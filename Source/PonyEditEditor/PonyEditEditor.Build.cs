using UnrealBuildTool;

public class PonyEditEditor : ModuleRules
{
	public PonyEditEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Slate",
			"SlateCore",
			"InputCore",
			"ToolMenus",
			"ToolWidgets",
			"EditorWidgets",
			"EditorFramework",
			"DeveloperSettings",
			"Projects",
			"MeshDescription",
			"Json",
			"PonyEditRuntime"
		});
	}
}
