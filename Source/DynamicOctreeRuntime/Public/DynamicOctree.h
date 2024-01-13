// Copyright 2024 Dream Seed LLC.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DynamicOctreeObjectInterface.h"
#include "Spatial/SparseDynamicOctree3.h"


#include "DynamicOctree.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogDynamicOctree, Log, All);

/**
 * UDynamicOctree maintains a collection of weak objects. The collection is arranged
 * as a dynamic sparse octree of axis-aligned uniform grid cells.
 * This allows for the contained objects to be spatialy queried efficiently.
 * 
 * Objects can be added/updated/removed at runtime.
 */
UCLASS(Blueprintable)
class DYNAMICOCTREERUNTIME_API UDynamicOctree : public UObject
{
	GENERATED_BODY()

protected:
	/** Octree to store registered objects. */
	UE::Geometry::FSparseDynamicOctree3 Octree;

	/** Map of objects in the tree. */
	UPROPERTY()
	TMap<int32, TWeakObjectPtr<UObject>> ObjectIDToObjectMap;

	//** True once the Octree is initialized and shouldn't have its structure changed. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool bOctreeInitialized;

public:

	//** */
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	double RootDimensionSize = 10000.0;

	//** Fraction we expand the dimension of any cell, to allow extra space to fit objects. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	double MaxExpandFactor = 0.25;

	//** Objects will not be inserted more than this many levels deep from a Root cell. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	int MaxTreeDepth = 10;


public:

	UDynamicOctree();


	/**
	 * Clears the octree and initializes it with current parameters.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	void InitializeOctree(const bool bForce = false);


	/**
	 * Rebuilds the Octree Structure, maintaining the objects already added..
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	void Rebuild();


	/**
	 * Removes Invalid Objects from the collection.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	void RemoveInvalidObjects();


	/**
	 * Erases all data in this collection.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	void Empty();

	/**
	 * Returns true if there are no valid objects in the collection.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	UPARAM(DisplayName = "IsEmpty") bool IsEmpty() const;

	/**
	 * Inserts an object in the octree. If it already exists, update its location and bounds.
	 * 
	 * @param Object Object to add to tree.
	 * @return True if the object was successfully added or already in the tree.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	UPARAM(DisplayName = "InTree") bool AddOrUpdateObject(UObject* Object);

	/**
	 * Test if an object is stored in the tree.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	UPARAM(DisplayName = "InTree") bool ContainsObject(UObject* Object);

	/**
	 * Remove an object from the octree
	 * 
	 * @param Object Object to remove from tree.
	 * @return true if the object was in the tree and removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	UPARAM(DisplayName = "Removed") bool RemoveObject(UObject* Object);

	/**
	 * Query objects within the specified spatial region.
	 * 
	 * @param bStrict When true: only objects which overlap the Query Area will be returned.
	 *                When false: nearby objects in the same octree cell will be returned.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	UPARAM(DisplayName = "Objects") TArray<UObject*> GetObjectsInArea(const FBox& QueryBounds, const bool bStrict = true) const;


	/**
	 * Find nearest ray-hit point with objects in tree.
	 * 
	 * @param Start Start of the Ray
	 * @param Direction Direction of the Ray
	 * @param MaxDistance maximum hit distance. If Negative will not be limited.
	 * @return Hit object if any.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	UPARAM(DisplayName = "Object") UObject* FindNearestHitObject(const FVector Start, const FVector Direction, const double MaxDistance = -1) const;

	/**
	 * Get all valid objects added to the tree.
	 * 
	 * @return Array of all valid objects added to the tree.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	UPARAM(DisplayName = "Objects") TArray<UObject*> GetAllObjects() const;


protected:

	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	bool GetObjectBounds(const UObject* Object, FBox& Bounds) const;

	UObject* GetObjectFromID(const int ObjectID) const;
	UE::Geometry::FAxisAlignedBox3d BoxBoundsToAxisAlignedBounds(const FBox& Bounds) const;
	UE::Geometry::FAxisAlignedBox3d GetObjectIDAxisAlignedBounds(const int ObjectID) const;
	double GetObjectIDDistanceToRay(int ObjectID, const FRay3d& Ray) const;

};
