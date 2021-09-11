using UnrealBuildTool;

public class GraphicsExamples : ModuleRules
{
	public GraphicsExamples(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
			});

		// these modules are needed to use rendering features
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"RHI",
				"RenderCore",
				"Renderer",
			});

		// Graphics plugin to reference the shaders
		PublicDependencyModuleNames.Add("GraphicsPlugin");
	}
}
