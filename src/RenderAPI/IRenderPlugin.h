#pragma once

#include "IRenderer.h"
#include "IShaderManager.h"
#include <memory>
#include <string>

// Export macro for plugins
#ifdef _WIN32
    #ifdef RENDER_PLUGIN_EXPORT
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __declspec(dllimport)
    #endif
#else
    #define PLUGIN_API
#endif

// Plugin interface
class IRenderPlugin
{
public:
    virtual ~IRenderPlugin() = default;

    virtual const char* getName() const = 0;
    virtual const char* getVersion() const = 0;

    virtual std::unique_ptr<IRenderer> createRenderer() = 0;
    virtual std::unique_ptr<IShaderManager> createShaderManager() = 0;
};

// Plugin creation function signature
typedef IRenderPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(IRenderPlugin*);

// Export functions that each plugin must implement
extern "C" {
    PLUGIN_API IRenderPlugin* CreatePlugin();
    PLUGIN_API void DestroyPlugin(IRenderPlugin* plugin);
}
