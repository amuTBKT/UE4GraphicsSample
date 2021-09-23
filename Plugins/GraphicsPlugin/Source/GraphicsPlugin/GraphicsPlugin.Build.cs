namespace UnrealBuildTool.Rules
{
	public class GraphicsPlugin : ModuleRules
	{
		public GraphicsPlugin(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

			PublicDependencyModuleNames.AddRange(
	            new string[]
	            {
	                "Core",
	                "CoreUObject",
	                "Projects",
	                "Engine",
	            }
	        );

			// these modules are needed to use rendering features
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"RHI",
					"RenderCore",
					"Renderer",
				});

			// these modules are needed to use Niagara
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Niagara",
					"VectorVM",
					"NiagaraCore",
				});

			PublicIncludePaths.AddRange(
        		new string[]
        		{
        			"../Shaders/Shared"
    			}
    		);
		}
	}
}
