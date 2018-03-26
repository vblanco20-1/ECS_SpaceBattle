// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ECSTesting : ModuleRules
{
	public ECSTesting(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
       // UEBuildConfiguration.bForceEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
        bEnableExceptions = true;
        PublicIncludePaths.AddRange(
        new string[] {

                    "ECSTesting/ThirdParty",
                    "ECSTesting/ECS_Base",
                    "ECSTesting/ECS_SpaceBattle"
            // ... add public include paths required here ...
        }
        );
    }
}
