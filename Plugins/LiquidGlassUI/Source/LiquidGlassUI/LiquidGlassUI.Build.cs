using UnrealBuildTool;

public class LiquidGlassUI : ModuleRules
{
    public LiquidGlassUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Slate", "SlateCore", "UMG" });
        PrivateDependencyModuleNames.AddRange(new[] { "Projects", "RenderCore", "RHI" });
    }
}
