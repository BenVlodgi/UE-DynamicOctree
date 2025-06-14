// Copyright 2024 Dream Seed LLC.

using UnrealBuildTool;

public class DynamicOctreeRuntime : ModuleRules
{
    public DynamicOctreeRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "GeometryCore",
        });
    }
}
