#define RENDER_PLUGIN_EXPORT
#include "../../src/RenderAPI/IRenderPlugin.h"
#include "../../src/OGL/Renderer.h"
#include "../../src/OGL/ShaderManager.h"
#include <memory>

class OGLPlugin : public IRenderPlugin
{
public:
    const char* getName() const override
    {
        return "OpenGL Renderer";
    }

    const char* getVersion() const override
    {
        return "1.0.0";
    }

    std::unique_ptr<IRenderer> createRenderer() override
    {
        return std::make_unique<OGL::Renderer>();
    }

    std::unique_ptr<IShaderManager> createShaderManager() override
    {
        return std::make_unique<OGL::ShaderManager>();
    }
};

extern "C" {
    PLUGIN_API IRenderPlugin* CreatePlugin()
    {
        return new OGLPlugin();
    }

    PLUGIN_API void DestroyPlugin(IRenderPlugin* plugin)
    {
        delete plugin;
    }
}
