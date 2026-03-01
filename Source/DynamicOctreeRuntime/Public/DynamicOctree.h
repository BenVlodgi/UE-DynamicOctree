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

	/** Map of objects in the tree.
	 * 
	 * TODO: Use a reverse lookup, and create own ID's incrememntally.
	 * Right now GetUniqueID() is uint which we convert to int.
	 * int: -1 is considered invalud in the ray intersection, which
	 * will clash if the UniqueID is 4294967295.
	 */
	UPROPERTY()
	TMap<int32, TWeakObjectPtr<UObject>> ObjectIDToObjectMap;

	/** True once the Octree is initialized and shouldn't have its structure changed. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Dynamic Octree")
	bool bOctreeInitialized;

public:

	/** World-space size of the octree root cell. Call Rebuild() or Empty() after changing. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Dynamic Octree")
	double RootDimensionSize = 10000.0;

	/** Fraction we expand the dimension of any cell, to allow extra space to fit objects. Call Rebuild() or Empty() after changing. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Dynamic Octree")
	double MaxExpandFactor = 0.25;

	/** Objects will not be inserted more than this many levels deep from a Root cell. Call Rebuild() or Empty() after changing. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Dynamic Octree")
	int MaxTreeDepth = 10;


public:

	UDynamicOctree();

	/**
	 * @deprecated Use Rebuild() to reapply configuration while preserving objects.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree", meta = (DeprecatedFunction, DeprecationMessage = "Use Rebuild() to reapply configuration while preserving objects."))
	void InitializeOctree();

	/**
	 * Rebuilds the Octree Structure, maintaining the objects already added.
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
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree", meta = (Keywords = "remove, clear"))
	void Empty();

	/**
	 * Returns true if there are no valid objects in the collection.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	UPARAM(DisplayName = "IsEmpty") bool IsEmpty() const;

	/**
	 * Returns count of registered objects even if they became null.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree", meta = (keywords = "number, size, count"))
	UPARAM(DisplayName = "Length") int Length() const;

	/**
	 * Inserts an object in the octree. If it already exists, update its location and bounds.
	 * 
	 * @param Object Object to add to tree.
	 * @return True if the object was successfully added or already in the tree.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree", meta = (keywords = "add object"))
	UPARAM(DisplayName = "InTree") bool AddOrUpdateObject(UObject* Object);

	/**
	 * Test if an object is stored in the tree.
	 */
	UFUNCTION(BlueprintPure, Category = "Dynamic Octree")
	[[nodiscard]] UPARAM(DisplayName = "InTree") bool ContainsObject(UObject* Object) const;

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
	 * @param bStrict Whether or not to use an AABB test to ensure results are within exact given bounds. Set to false, if you will be doing your own simple distance/range check on the results.
	 *				  When true: only objects which overlap the Query Area will be returned.
	 *                When false: nearby objects in the same octree cell may be returned.
	 * @param Class   Optional Class filter.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = "false", Category = "Dynamic Octree", meta = (DeterminesOutputType = "Class", DynamicOutput = "Objects", keywords = "query"))
	[[nodiscard]] UPARAM(DisplayName = "Objects") TArray<UObject*> GetObjectsInArea(const FBox& QueryBounds, const bool bStrict = true, const TSubclassOf<UObject> Class = nullptr) const;

	/**
	 * Find nearest ray-hit point with objects in tree.
	 * 
	 * @param Start			Start of the Ray
	 * @param Direction		Direction of the Ray
	 * @param MaxDistance	maximum hit distance. If Negative will not be limited.
	 * @param Class			Optional Class filter. If specified, only objects of this class (or subclasses) will be considered.
	 * @return Hit object if any.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = "false", Category = "Dynamic Octree")
	[[nodiscard]] UPARAM(DisplayName = "Object") UObject* FindNearestHitObject(const FVector Start, const FVector Direction, const double MaxDistance = -1, const TSubclassOf<UObject> Class = nullptr) const;

	/**
	 * Get all valid objects added to the tree.
	 * @param Class   Optional Class filter.
	 * 
	 * @return Array of all valid objects added to the tree.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = "false", Category = "Dynamic Octree", meta = (DeterminesOutputType = "Class"))
	[[nodiscard]] UPARAM(DisplayName = "Objects") TArray<UObject*> GetAllObjects(const TSubclassOf<UObject> Class = nullptr) const;


protected:

	/**
	 * Resolves the world-space AABB for an object.
	 * Supports AActor, USceneComponent, and objects implementing UDynamicOctreeObjectInterface.
	 *
	 * @param Object  The object to query.
	 * @param Bounds  Out parameter filled with the object's bounding box.
	 * @return True if bounds were successfully retrieved.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Octree")
	bool GetObjectBounds(const UObject* Object, FBox& Bounds) const;

	[[nodiscard]] UObject* GetObjectFromID(const int ObjectID) const;
	[[nodiscard]] UE::Geometry::FAxisAlignedBox3d BoxBoundsToAxisAlignedBounds(const FBox& Bounds) const;
	[[nodiscard]] UE::Geometry::FAxisAlignedBox3d GetObjectIDAxisAlignedBounds(const int ObjectID) const;
	[[nodiscard]] double GetObjectIDRayHitDistance(const int ObjectID, const FRay3d& Ray) const;
	[[nodiscard]] double GetObjectIDRayHitDistanceForClass(const int ObjectID, const FRay3d& Ray, const TSubclassOf<UObject> Class) const;

};
