#include "PluginLoader.h"
#include "../Logger.h"

PluginLoader::PluginLoader()
    : m_library(nullptr)
    , m_plugin(nullptr)
    , m_destroyFunc(nullptr)
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

    CreatePluginFunc createFunc = (CreatePluginFunc)GetProcAddress(m_library, "CreatePlugin");
    m_destroyFunc = (DestroyPluginFunc)GetProcAddress(m_library, "DestroyPlugin");
#else
    m_library = dlopen(path.c_str(), RTLD_LAZY);
    if (!m_library)
    {
        LOG_ERROR("Failed to load plugin: {}", path);
        LOG_ERROR("Error: {}", dlerror());
        return false;
    }

    CreatePluginFunc createFunc = (CreatePluginFunc)dlsym(m_library, "CreatePlugin");
    m_destroyFunc = (DestroyPluginFunc)dlsym(m_library, "DestroyPlugin");
#endif

    if (!createFunc || !m_destroyFunc)
    {
        LOG_ERROR("Failed to find plugin functions");
        unloadPlugin();
        return false;
    }

    m_plugin = createFunc();
    if (!m_plugin)
    {
        LOG_ERROR("Failed to create plugin instance");
        unloadPlugin();
        return false;
    }

    LOG_INFO("Loaded plugin: {} v{}", m_plugin->getName(), m_plugin->getVersion());
    return true;
}

void PluginLoader::unloadPlugin()
{
    if (m_plugin && m_destroyFunc)
    {
        m_destroyFunc(m_plugin);
        m_plugin = nullptr;
        m_destroyFunc = nullptr;
    }

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
