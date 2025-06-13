<br/>
<p align="center">
  <a href="https://github.com/BenVlodgi/UE-DynamicOctree">
    <img src="https://github.com/BenVlodgi/UE-DynamicOctree/assets/1462374/952437e7-1917-4587-b6ff-f094352c332e" alt="Logo" width="640" height="320">
  </a>
</p>

# Dynamic Octree
This Unreal Engine plugin provides [`UDynamicOctree`](https://github.com/BenVlodgi/UE-DynamicOctree/blob/main/Source/DynamicOctreeRuntime/Public/DynamicOctree.h). An object which maintains a collection of (weakly referenced) spatial objects. The purpose of which is to enable easy spatial queries for objects in the collection.
The collection uses [`FSparseDynamicOctree3`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/GeometryCore/Spatial/FSparseDynamicOctree3) to maintain the object organization.

Objects can be added/updated/removed at runtime.

Objects must either be [`AActor`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/GameFramework/AActor), [`USceneComponent`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/Components/USceneComponent), or implement [`UDynamicOctreeObjectInterface`](https://github.com/BenVlodgi/UE-DynamicOctree/blob/main/Source/DynamicOctreeRuntime/Public/DynamicOctreeObjectInterface.h)

![DynamicOctreeAPI](https://github.com/user-attachments/assets/1e38c975-bbee-4d51-8b13-1b1cfd575926)
