#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

class FLiquidGlassUIModule final : public IModuleInterface
{
public:
    void StartupModule() override
    {
        const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("LiquidGlassUI"));
        check(Plugin.IsValid());
        AddShaderSourceDirectoryMapping(TEXT("/Plugin/LiquidGlassUI"), FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders")));
    }
};
IMPLEMENT_MODULE(FLiquidGlassUIModule, LiquidGlassUI)
