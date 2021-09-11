using UnrealBuildTool;
using System.Collections.Generic;

public class GraphicsExamplesEditorTarget : TargetRules
{
	public GraphicsExamplesEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "GraphicsExamples" } );
	}
}
