# Dynamic Octree

This plugin provides `UDynamicOctree`. An object which maintains a collection of (weakly referenced) spatial objects. The purpose of which is to enable easy spatial queries for objects in the collection.
The collection uses `FSparseDynamicOctree3` to maintain the object organization.

Objects can be added/updated/removed at runtime.
