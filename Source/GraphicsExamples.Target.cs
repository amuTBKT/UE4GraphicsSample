using UnrealBuildTool;
using System.Collections.Generic;

public class GraphicsExamplesTarget : TargetRules
{
	public GraphicsExamplesTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "GraphicsExamples" } );
	}
}
