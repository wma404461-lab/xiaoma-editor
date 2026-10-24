using UnrealBuildTool;

public class PonyEditRuntime : ModuleRules
{
	public PonyEditRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings"
		});
	}
}
