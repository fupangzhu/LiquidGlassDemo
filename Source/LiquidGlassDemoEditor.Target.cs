using UnrealBuildTool;
public class LiquidGlassDemoEditorTarget : TargetRules
{
    public LiquidGlassDemoEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("LiquidGlassDemo");
    }
}
