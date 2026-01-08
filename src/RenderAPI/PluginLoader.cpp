#include "PluginLoader.h"
#include "../Logger.h"

PluginLoader::PluginLoader()
    : m_library(nullptr)
    , m_plugin(nullptr, PluginDeleter(nullptr))
{
}

PluginLoader::~PluginLoader()
{
    unloadPlugin();
}

bool PluginLoader::loadPlugin(const std::string& path)
{
    if (m_library)
    {
        LOG_ERROR("Plugin already loaded. Unload it first.");
        return false;
    }

#ifdef _WIN32
    m_library = LoadLibraryA(path.c_str());
    if (!m_library)
    {
        LOG_ERROR("Failed to load plugin: {}", path);
        LOG_ERROR("Error code: {}", GetLastError());
        return false;
    }

    CreatePluginFunc createFunc = reinterpret_cast<CreatePluginFunc>(GetProcAddress(m_library, "CreatePlugin"));
    DestroyPluginFunc destroyFunc = reinterpret_cast<DestroyPluginFunc>(GetProcAddress(m_library, "DestroyPlugin"));
#else
    m_library = dlopen(path.c_str(), RTLD_LAZY);
    if (!m_library)
    {
        LOG_ERROR("Failed to load plugin: {}", path);
        LOG_ERROR("Error: {}", dlerror());
        return false;
    }

    CreatePluginFunc createFunc = reinterpret_cast<CreatePluginFunc>(dlsym(m_library, "CreatePlugin"));
    DestroyPluginFunc destroyFunc = reinterpret_cast<DestroyPluginFunc>(dlsym(m_library, "DestroyPlugin"));
#endif

    if (!createFunc || !destroyFunc)
    {
        LOG_ERROR("Failed to find plugin functions");
        unloadPlugin();
        return false;
    }

    IRenderPlugin* pluginRaw = createFunc();
    if (!pluginRaw)
    {
        LOG_ERROR("Failed to create plugin instance");
        unloadPlugin();
        return false;
    }

    // Create smart pointer with custom deleter
    m_plugin = PluginPtr(pluginRaw, PluginDeleter(destroyFunc));

    LOG_INFO("Loaded plugin: {} v{}", m_plugin->getName(), m_plugin->getVersion());
    return true;
}

void PluginLoader::unloadPlugin()
{
    // Smart pointer will automatically call the deleter
    m_plugin.reset();

    if (m_library)
    {
#ifdef _WIN32
        FreeLibrary(m_library);
#else
        dlclose(m_library);
#endif
        m_library = nullptr;
    }
}

PluginPtr PluginLoader::releasePlugin()
{
    // Transfer ownership to caller
    return std::move(m_plugin);
}
