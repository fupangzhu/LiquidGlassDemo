using UnrealBuildTool;
public class LiquidGlassDemo : ModuleRules
{
    public LiquidGlassDemo(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "UMG", "LiquidGlassUI" });
    }
}
