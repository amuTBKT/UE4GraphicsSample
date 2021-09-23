namespace UnrealBuildTool.Rules
{
	public class GraphicsPluginEditor : ModuleRules
	{
		public GraphicsPluginEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

			PublicDependencyModuleNames.AddRange(
	            new string[]
	            {
	                "Core",
	                "CoreUObject",
	                "Engine",
					"ApplicationCore",
	            }
	        );

        	PrivateDependencyModuleNames.AddRange(
        		new string[]
				{
					"UnrealEd",
					"UMG",
					"Slate",
					"SlateCore",
					"LevelEditor",
					"PropertyEditor",
				}
			);
		}
	}
}
