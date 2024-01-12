// Copyright 2024 Dream Seed LLC.


#include "DynamicOctree.h"
#include "Math/NumericLimits.h"

#include "DynamicOctreeObjectInterface.h"



UDynamicOctree::UDynamicOctree()
{
	InitializeOctree();
}


void UDynamicOctree::InitializeOctree(const bool bForce)
{
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
	// Clears Octree.
	InitializeOctree(true);

	// Remove any invalid objects.
	ObjectIDToObjectMap.FilterByPredicate(
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
			UE_LOG(LogTemp, Warning, TEXT("UDynamicOctree::Rebuild - Failed to get Object bounds. This shouldn't happen, as the object was added before rebuild."));
		}
	}
}


bool UDynamicOctree::AddOrUpdateObject(UObject* Object)
{
	if (!Object)
	{
		UE_LOG(LogTemp, Warning, TEXT("UDynamicOctree::RegisterObject - Attempted to register a null object."));
		return false;
	}

	FBox ObjectBoundsBox;
	if (!GetObjectBounds(Object, ObjectBoundsBox))
	{
		UE_LOG(LogTemp, Warning, TEXT("UDynamicOctree::RegisterObject - Failed to get object bounds."));
		return false;
	}

	// Register or update the object in the octree
	const int ObjectID = Object->GetUniqueID();
	const UE::Geometry::FAxisAlignedBox3d AxisAlignedBox3d = BoxBoundsToAxisAlignedBounds(ObjectBoundsBox);
	
	uint32 SuggestedCellID = -1;

	if (!Octree.ContainsObject(ObjectID))
	{
		Octree.InsertObject(ObjectID, AxisAlignedBox3d);
	}
	else if (Octree.CheckIfObjectNeedsReinsert(ObjectID, AxisAlignedBox3d, SuggestedCellID))
	{
		Octree.ReinsertObject(ObjectID, AxisAlignedBox3d, SuggestedCellID);
	}

	return true;
}


bool UDynamicOctree::ContainsObject(UObject* Object)
{
	if (!Object)
	{
		UE_LOG(LogTemp, Warning, TEXT("UDynamicOctree::ContainsObject - Failed to check null object."));
		return false;
	}

	return Octree.ContainsObject(Object->GetUniqueID());
}


bool UDynamicOctree::RemoveObject(UObject* Object)
{
	if (!Object)
	{
		UE_LOG(LogTemp, Warning, TEXT("UDynamicOctree::UnregisterObject - Attempted to unregister a null object."));
		return false;
	}

	// Unregister the object from the octree
	return Octree.RemoveObject(Object->GetUniqueID());
}


TArray<UObject*> UDynamicOctree::GetObjectsInArea(const FBox& QueryBounds, const bool bStrict) const
{
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

			}

			ResultObjects.Add(Object);
		}
	}
	ResultObjects.Shrink();


	return ResultObjects;
}


UObject* UDynamicOctree::FindNearestHitObject(const FVector Start, const FVector Direction, const double MaxDistance) const
{
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
	if (!Object)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetObjectBounds - Null object provided."));
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
		UE_LOG(LogTemp, Warning, TEXT("GetObjectBounds - Object type not supported or does not implement DynamicOctreeObjectInterface."));
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
