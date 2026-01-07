#define RENDER_PLUGIN_EXPORT
#include "../../src/RenderAPI/IRenderPlugin.h"
#include "../../src/VK/Renderer.h"
#include "../../src/VK/ShaderManager.h"
#include <memory>

class VKPlugin : public IRenderPlugin
{
public:
    const char* getName() const override
    {
        return "Vulkan Renderer";
    }

    const char* getVersion() const override
    {
        return "1.0.0";
    }

    std::unique_ptr<IRenderer> createRenderer() override
    {
        return std::make_unique<VK::Renderer>();
    }

    std::unique_ptr<IShaderManager> createShaderManager() override
    {
        return std::make_unique<VK::ShaderManager>();
    }
};

extern "C" {
    PLUGIN_API IRenderPlugin* CreatePlugin()
    {
        return new VKPlugin();
    }

    PLUGIN_API void DestroyPlugin(IRenderPlugin* plugin)
    {
        delete plugin;
    }
}
