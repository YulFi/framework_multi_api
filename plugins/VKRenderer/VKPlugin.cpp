#define RENDER_PLUGIN_EXPORT
#include "../../src/RenderAPI/IRenderPlugin.h"
#include "../../src/VK/Renderer.h"
#include "../../src/VK/ShaderManager.h"
#include <memory>

class VKPlugin : public IRenderPlugin
{
public:
    VKPlugin() : m_renderer(nullptr), m_shaderManager(nullptr) {}

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
        auto renderer = std::make_unique<VK::Renderer>();
        m_renderer = renderer.get();

        // Connect shader manager to renderer if it was already created
        if (m_shaderManager && m_renderer)
        {
            m_renderer->setShaderManager(m_shaderManager);
        }

        return renderer;
    }

    std::unique_ptr<IShaderManager> createShaderManager() override
    {
        auto shaderManager = std::make_unique<VK::ShaderManager>();
        m_shaderManager = shaderManager.get();

        // Connect shader manager to renderer if it was already created
        if (m_shaderManager && m_renderer)
        {
            m_renderer->setShaderManager(m_shaderManager);
        }

        return shaderManager;
    }

private:
    VK::Renderer* m_renderer;
    VK::ShaderManager* m_shaderManager;
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
