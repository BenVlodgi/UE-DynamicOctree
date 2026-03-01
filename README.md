<br/>
<p align="center">
  <a href="https://github.com/BenVlodgi/UE-DynamicOctree">
    <img src="https://github.com/BenVlodgi/UE-DynamicOctree/assets/1462374/952437e7-1917-4587-b6ff-f094352c332e" alt="Logo" width="640">
  </a>
</p>

# Dynamic Octree
This Unreal Engine plugin provides [`UDynamicOctree`](https://github.com/BenVlodgi/UE-DynamicOctree/blob/main/Source/DynamicOctreeRuntime/Public/DynamicOctree.h). An object which maintains a collection of (weakly referenced) spatial objects. The purpose of which is to enable easy **efficient** spatial queries for objects in the collection.
The collection uses the engine's [`FSparseDynamicOctree3`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/GeometryCore/Spatial/FSparseDynamicOctree3) to maintain the object organization.

Objects can be added/updated/removed at runtime.

Objects must either be [`AActor`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/GameFramework/AActor), [`USceneComponent`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/Components/USceneComponent), or implement [`UDynamicOctreeObjectInterface`](https://github.com/BenVlodgi/UE-DynamicOctree/blob/main/Source/DynamicOctreeRuntime/Public/DynamicOctreeObjectInterface.h)


<img src="https://github.com/user-attachments/assets/1e38c975-bbee-4d51-8b13-1b1cfd575926" alt="DynamicOctreeAPI" style="max-width:min(100%,100%);height:auto;">

## Installation
1. Copy the plugin into your project's `Plugins` folder.
2. Enable **DynamicOctree** in the Unreal Plugin Browser.
3. Recompile project.


## Configuration
The octree initializes automatically on construction. If you change these properties after construction, call `Rebuild()` to apply them (this preserves registered objects) or `Empty()` to apply them and clear all objects.

| Property | Default | Description |
|----------|---------|-------------|
| `RootDimensionSize` | `10000.0` | World-space size of the octree root cell. |
| `MaxExpandFactor` | `0.25` | Fractional cell expansion to fit objects. |
| `MaxTreeDepth` | `10` | Maximum subdivision depth. |

## Usage
### Blueprint
1. Create a `UDynamicOctree` object (e.g. via `Construct Object from Class`).
2. Call **Add or Update Object** to register actors or components.
3. Use **Get Objects In Area** or **Find Nearest Hit Object** to query.

### C++
```cpp
#include "DynamicOctree.h"

// Create
UDynamicOctree* Octree = NewObject<UDynamicOctree>(this);

// Add
Octree->AddOrUpdateObject(SomeActor);

// Range query
FBox QueryBox(Center - FVector(500), Center + FVector(500));
TArray<UObject*> NearbyObjects = Octree->GetObjectsInArea(QueryBox);

// Ray query
UObject* Hit = Octree->FindNearestHitObject(RayStart, RayDirection, RayLength);

// Remove
Octree->RemoveObject(SomeActor);
```

## Supported Object Types
Objects added to the octree must be one of:

| Type | Bounds source |
|------|---------------|
| `AActor` | `GetComponentsBoundingBox()` |
| `USceneComponent` | `Bounds.GetBox()` |
| Any `UObject` implementing `UDynamicOctreeObjectInterface` | `GetWorldBounds()` |

### Implementing `UDynamicOctreeObjectInterface`
For any `UObject` that isn't an `AActor` or `USceneComponent` which you want to track in the octree: implement the interface to provide custom bounds:
```cpp
// MyObject.h
#include "DynamicOctreeObjectInterface.h"

UCLASS()
class UMyObject : public UObject, public IDynamicOctreeObjectInterface
{
    GENERATED_BODY()
public:
    virtual void GetWorldBounds_Implementation(FBox& Bounds) const override;
};

// MyObject.cpp
void UMyObject::GetWorldBounds_Implementation(FBox& Bounds) const
{
    Bounds = FBox(MyMin, MyMax);
}
```


## Methods
| Method | Description |
|--------|-------------|
| `Rebuild()` | Reapply configuration and reinitialize the tree structure, keeping all registered objects. |
| `RemoveInvalidObjects()` | Remove any GC'd (null) weak object references. |
| `Empty()` | Clear all objects and reset the tree. |
| `AddOrUpdateObject(Object)` | Insert an object, or update its position if already present. |
| `RemoveObject(Object)` | Remove an object from the tree. |
| `ContainsObject(Object)` | Returns true if the object is currently in the tree. |
| `GetObjectsInArea(QueryBounds, bStrict, Class)` | Return all objects overlapping the given AABB (when `bStrict` is false, it may include a few other nearby things, assuming you might have a more explicit boundary check to do next). |
| `FindNearestHitObject(Start, Direction, MaxDistance, Class)` | Return the nearest object hit by a ray. |
| `GetAllObjects(Class)` | Return all valid registered objects. |
| `IsEmpty()` | Returns true if no valid objects remain. |
| `Length()` | Returns the raw entry count (may include stale entries before `RemoveInvalidObjects`). |

