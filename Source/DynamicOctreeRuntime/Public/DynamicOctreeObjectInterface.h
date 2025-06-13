// Copyright 2024 Dream Seed LLC.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DynamicOctreeObjectInterface.generated.h"

/**
 * Interface for objects that can be registered in DynamicOctree
 */
UINTERFACE(BlueprintType)
class UDynamicOctreeObjectInterface : public UInterface
{
	GENERATED_BODY()
};

class DYNAMICOCTREERUNTIME_API IDynamicOctreeObjectInterface
{
	GENERATED_BODY()

public:
	/**
	 * Get the world bounds of the object.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dynamic Octree")
	void GetWorldBounds(FBox& Bounds) const;
	virtual void GetWorldBounds_Implementation(FBox& Bounds) const;
};
