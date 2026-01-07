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

class PluginLoader
{
public:
    PluginLoader();
    ~PluginLoader();

    bool loadPlugin(const std::string& path);
    void unloadPlugin();

    IRenderPlugin* getPlugin() const { return m_plugin; }
    bool isLoaded() const { return m_plugin != nullptr; }

private:
    LibraryHandle m_library;
    IRenderPlugin* m_plugin;
    DestroyPluginFunc m_destroyFunc;
};
