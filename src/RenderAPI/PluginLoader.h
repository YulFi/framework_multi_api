#pragma once

#include "IRenderPlugin.h"
#include <string>
#include <memory>

#ifdef _WIN32
    #include <windows.h>
    typedef HMODULE LibraryHandle;
#else
    #include <dlfcn.h>
    typedef void* LibraryHandle;
#endif

/**
 * Custom deleter for plugin smart pointer
 * Ensures proper plugin destruction through the plugin's own DestroyPlugin function
 */
struct PluginDeleter
{
    DestroyPluginFunc destroyFunc;

    PluginDeleter(DestroyPluginFunc func = nullptr) : destroyFunc(func) {}

    void operator()(IRenderPlugin* plugin) const
    {
        if (plugin && destroyFunc)
        {
            destroyFunc(plugin);
        }
    }
};

using PluginPtr = std::unique_ptr<IRenderPlugin, PluginDeleter>;

class PluginLoader
{
public:
    PluginLoader();
    ~PluginLoader();

    bool loadPlugin(const std::string& path);
    void unloadPlugin();

    // Get plugin as smart pointer with custom deleter
    PluginPtr releasePlugin();

    // Check if plugin is loaded (deprecated - for backwards compatibility)
    IRenderPlugin* getPlugin() const { return m_plugin.get(); }
    bool isLoaded() const { return m_plugin != nullptr; }

private:
    LibraryHandle m_library;
    PluginPtr m_plugin;
};
