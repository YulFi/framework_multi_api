#include "MyApp.h"
#include "Logger.h"
#include <string>
#include <filesystem>

void printUsage()
{
    LOG_INFO("Usage: OpenGLProject [--plugin <path>]");
    LOG_INFO("Options:");
    LOG_INFO("  --plugin <path>  Path to renderer plugin DLL (default: plugins/OGLRenderer.dll)");
    LOG_INFO("  --help           Show this help message");
    LOG_INFO("");
    LOG_INFO("Available plugins:");
    LOG_INFO("  plugins/OGLRenderer.dll - OpenGL renderer");
    LOG_INFO("  plugins/VKRenderer.dll  - Vulkan renderer");
}

int main(int argc, char* argv[])
{
    // Initialize logger
    Logger::getInstance().enableFileLogging("renderer.log");
    Logger::getInstance().setLogLevel(LogLevel::Debug);

    std::string pluginPath = "plugins/OGLRenderer.dll";
    //std::string pluginPath = "plugins/VKRenderer.dll";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            printUsage();
            return 0;
        }
        else if (arg == "--plugin" && i + 1 < argc)
        {
            pluginPath = argv[++i];
        }
        else
        {
            LOG_ERROR("Unknown argument: {}", arg);
            printUsage();
            return -1;
        }
    }

    LOG_INFO("========================================");
    LOG_INFO("Loading renderer plugin: {}", pluginPath);
    LOG_INFO("========================================");

    MyApp app(pluginPath);

    if (!app.initialize())
    {
        LOG_ERROR("Failed to initialize application");
        return -1;
    }

    app.run();
    app.shutdown();

    return 0;
}
