// Copyright 2024 Dream Seed LLC.


#include "DynamicOctree.h"
#include "GameFramework/Actor.h"
#include "Math/NumericLimits.h"

#include "DynamicOctreeObjectInterface.h"

DEFINE_LOG_CATEGORY(LogDynamicOctree);


UDynamicOctree::UDynamicOctree()
{
	InitializeOctree();
}


void UDynamicOctree::InitializeOctree(const bool bForce)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(InitializeOctree);

	if (!bOctreeInitialized || bForce)
	{
		Octree = UE::Geometry::FSparseDynamicOctree3();

		Octree.RootDimension = RootDimensionSize;
		Octree.MaxExpandFactor = MaxExpandFactor;
		Octree.SetMaxTreeDepth(MaxTreeDepth);

		bOctreeInitialized = true;
	}
}


void UDynamicOctree::Rebuild()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Rebuild);

	// Clears Octree.
	InitializeOctree(true);

	// Remove any invalid objects.
	ObjectIDToObjectMap = ObjectIDToObjectMap.FilterByPredicate(
		[](const TPair<int32, TWeakObjectPtr<UObject>>& IDToObject)
		{
			return IDToObject.Value.IsValid();
		});

	// Add the objects back to the tree.
	for (const auto& IDToObject : ObjectIDToObjectMap)
	{
		FBox ObjectBoundsBox;
		if (GetObjectBounds(IDToObject.Value.Get(), ObjectBoundsBox))
		{
			Octree.InsertObject(IDToObject.Key, BoxBoundsToAxisAlignedBounds(ObjectBoundsBox));
		}
		else
		{
			UE_LOG(LogDynamicOctree, Warning, TEXT("UDynamicOctree::Rebuild - Failed to get Object bounds. This shouldn't happen, as the object was added before rebuild."));
		}
	}
}


void UDynamicOctree::RemoveInvalidObjects()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RemoveInvalidObjects);

	TArray<int32> IDsToRemove;
	for (const auto& ObjectIDToObject : ObjectIDToObjectMap)
	{
		if (!ObjectIDToObject.Value.IsValid())
		{
			IDsToRemove.Add(ObjectIDToObject.Key);
		}
	}

	for (const int32& ObjectID : IDsToRemove)
	{
		ObjectIDToObjectMap.Remove(ObjectID);
		Octree.RemoveObject(ObjectID);
	}
}


void UDynamicOctree::Empty()
{
	ObjectIDToObjectMap.Empty();
	InitializeOctree(true);
}


bool UDynamicOctree::IsEmpty() const
{
	for (const auto& ObjectIDToObject : ObjectIDToObjectMap)
	{
		if (ObjectIDToObject.Value.IsValid())
		{
			return false;
		}
	}

	return true;
}

int UDynamicOctree::Length() const
{
	return ObjectIDToObjectMap.Num();
}


bool UDynamicOctree::AddOrUpdateObject(UObject* Object)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AddOrUpdateObject);

	if (!Object)
	{
		UE_LOG(LogDynamicOctree, Warning, TEXT("UDynamicOctree::RegisterObject - Attempted to register a null object."));
		return false;
	}

	FBox ObjectBoundsBox;
	if (!GetObjectBounds(Object, ObjectBoundsBox))
	{
		UE_LOG(LogDynamicOctree, Warning, TEXT("UDynamicOctree::RegisterObject - Failed to get object bounds."));
		return false;
	}

	// Register or update the object in the octree
	const int ObjectID = Object->GetUniqueID();
	const UE::Geometry::FAxisAlignedBox3d AxisAlignedBox3dBounds = BoxBoundsToAxisAlignedBounds(ObjectBoundsBox);
	
	uint32 SuggestedCellID = -1;

	if (!Octree.ContainsObject(ObjectID))
	{
		Octree.InsertObject(ObjectID, AxisAlignedBox3dBounds);
		ObjectIDToObjectMap.Add(ObjectID, Object);
	}
	else if (Octree.CheckIfObjectNeedsReinsert(ObjectID, AxisAlignedBox3dBounds, SuggestedCellID))
	{
		Octree.ReinsertObject(ObjectID, AxisAlignedBox3dBounds, SuggestedCellID);
		ObjectIDToObjectMap.Add(ObjectID, Object);
	}

	return true;
}


bool UDynamicOctree::ContainsObject(UObject* Object)
{
	if (!Object)
	{
		UE_LOG(LogDynamicOctree, Warning, TEXT("UDynamicOctree::ContainsObject - Failed to check null object."));
		return false;
	}

	return Octree.ContainsObject(Object->GetUniqueID());
}


bool UDynamicOctree::RemoveObject(UObject* Object)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RemoveObject);

	if (!Object)
	{
		UE_LOG(LogDynamicOctree, Warning, TEXT("UDynamicOctree::UnregisterObject - Attempted to unregister a null object."));
		return false;
	}

	// Unregister the object from the octree
	return Octree.RemoveObject(Object->GetUniqueID());
}


TArray<UObject*> UDynamicOctree::GetObjectsInArea(const FBox& QueryBounds, const bool bStrict) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetObjectsInArea);

	const UE::Geometry::FAxisAlignedBox3d AxisAlignedBox3d = BoxBoundsToAxisAlignedBounds(QueryBounds);

	// Query objects within the specified spatial region
	TArray<int32> ObjectIDs;
	Octree.RangeQuery(AxisAlignedBox3d, ObjectIDs);


	TArray<UObject*> ResultObjects;
	ResultObjects.Reserve(ObjectIDs.Num());
	for (const int& ObjectID : ObjectIDs)
	{
		TWeakObjectPtr<UObject> ObjectPtr = ObjectIDToObjectMap.FindRef(ObjectID);
		if (ObjectPtr.IsValid())
		{
			UObject* Object = ObjectPtr.Get();
			if (bStrict)
			{
				// Strictly test AABB
				FBox ObjectBounds;
				GetObjectBounds(Object, ObjectBounds);

				// AABB test object bounds to query bounds
				if (!QueryBounds.Intersect(ObjectBounds))
				{
					continue;
				}
			}

			ResultObjects.Add(Object);
		}
	}
	ResultObjects.Shrink();

	return ResultObjects;
}

UObject* UDynamicOctree::FindNearestHitObject(const FVector Start, const FVector Direction, const double MaxDistance) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FindNearestHitObject);

	const FRay Ray = FRay(Start, Direction);
	const double UseMaxDistance = MaxDistance >= 0 ? MaxDistance : TNumericLimits<double>::Max();

	const int32 HitObjectID = Octree.FindNearestHitObject(
		Ray,
		[this](const int ID){ return GetObjectIDAxisAlignedBounds(ID); },
		[this](const int ID, const FRay3d& Ray){ return GetObjectIDDistanceToRay(ID, Ray); },
		UseMaxDistance
	);

	return HitObjectID >= 0 ? GetObjectFromID(HitObjectID) : nullptr;
}


TArray<UObject*> UDynamicOctree::GetAllObjects() const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetAllObjects);

	TArray<TWeakObjectPtr<UObject>> AllWeakObjects;
	ObjectIDToObjectMap.GenerateValueArray(AllWeakObjects);

	TArray<UObject*> AllObjects;
	for (const auto& WeakObject : AllWeakObjects)
	{
		if (WeakObject.IsValid())
		{
			AllObjects.Add(WeakObject.Get());
		}
	}
	return AllObjects;
}


bool UDynamicOctree::GetObjectBounds(const UObject* Object, FBox& Bounds) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetObjectBounds);

	if (!Object)
	{
		UE_LOG(LogDynamicOctree, Warning, TEXT("GetObjectBounds - Null object provided."));
		return false;
	}

	// Use the interface if implemented.
	if (Object->GetClass()->ImplementsInterface(UDynamicOctreeObjectInterface::StaticClass()))
	{
		IDynamicOctreeObjectInterface::Execute_GetWorldBounds(Object, Bounds);
		return true;
	}
	else if (const AActor* Actor = Cast<AActor>(Object))
	{
		Bounds = Actor->GetComponentsBoundingBox();
		return true;
	}
	else if (const USceneComponent* SceneComponent = Cast<USceneComponent>(Object))
	{
		Bounds = SceneComponent->Bounds.GetBox();
		return true;
	}
	else
	{
		UE_LOG(LogDynamicOctree, Warning, TEXT("GetObjectBounds - Object type not supported or does not implement DynamicOctreeObjectInterface."));
		return false;
	}
}


UObject* UDynamicOctree::GetObjectFromID(const int ObjectID) const
{
	const TWeakObjectPtr<UObject> WeakObject = ObjectIDToObjectMap.FindRef(ObjectID);
	return WeakObject.IsValid() ? WeakObject.Get() : nullptr;
}

UE::Geometry::FAxisAlignedBox3d UDynamicOctree::BoxBoundsToAxisAlignedBounds(const FBox& Bounds) const
{
	return UE::Geometry::FAxisAlignedBox3d(Bounds.Min, Bounds.Max);
}


UE::Geometry::FAxisAlignedBox3d UDynamicOctree::GetObjectIDAxisAlignedBounds(const int ObjectID) const
{
	const UObject* Object = GetObjectFromID(ObjectID);
	if (IsValid(Object))
	{
		FBox ObjectBoundsBox;
		if (GetObjectBounds(Object, ObjectBoundsBox))
		{
			return BoxBoundsToAxisAlignedBounds(ObjectBoundsBox);
		}
	}

	return UE::Geometry::FAxisAlignedBox3d();
}


double UDynamicOctree::GetObjectIDDistanceToRay(int ObjectID, const FRay3d& Ray) const
{
	const UObject* Object = GetObjectFromID(ObjectID);
	if (IsValid(Object))
	{
		FBox ObjectBoundsBox;
		if (GetObjectBounds(Object, ObjectBoundsBox))
		{
			return Ray.Dist(ObjectBoundsBox.GetCenter());
		}
	}

	return TNumericLimits<double>::Max();
}
