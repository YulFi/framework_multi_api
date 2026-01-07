#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <memory>

enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

class Logger
{
public:
    static Logger& getInstance();

    void setLogLevel(LogLevel level);
    void enableFileLogging(const std::string& filename);
    void disableFileLogging();
    void enableConsoleLogging(bool enable);

    void log(LogLevel level, const std::string& message, const char* file = nullptr, int line = -1);

    template<typename... Args>
    void debug(const std::string& format, Args... args)
    {
        log(LogLevel::Debug, formatString(format, args...));
    }

    template<typename... Args>
    void info(const std::string& format, Args... args)
    {
        log(LogLevel::Info, formatString(format, args...));
    }

    template<typename... Args>
    void warning(const std::string& format, Args... args)
    {
        log(LogLevel::Warning, formatString(format, args...));
    }

    template<typename... Args>
    void error(const std::string& format, Args... args)
    {
        log(LogLevel::Error, formatString(format, args...));
    }

    // Simple overloads for single string messages
    void debug(const std::string& message) { log(LogLevel::Debug, message); }
    void info(const std::string& message) { log(LogLevel::Info, message); }
    void warning(const std::string& message) { log(LogLevel::Warning, message); }
    void error(const std::string& message) { log(LogLevel::Error, message); }

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string formatString(const std::string& format);

    template<typename T, typename... Args>
    std::string formatString(const std::string& format, T value, Args... args)
    {
        std::string result;
        size_t pos = 0;
        bool replaced = false;

        for (size_t i = 0; i < format.length(); ++i)
        {
            if (format[i] == '{' && i + 1 < format.length() && format[i + 1] == '}' && !replaced)
            {
                result += format.substr(pos, i - pos);
                std::ostringstream oss;
                oss << value;
                result += oss.str();
                pos = i + 2;
                replaced = true;
                break;
            }
        }

        if (replaced)
        {
            result += formatString(format.substr(pos), args...);
        }
        else
        {
            result += format.substr(pos);
        }

        return result;
    }

    std::string getCurrentTime();
    std::string levelToString(LogLevel level);
    const char* levelToColor(LogLevel level);

    LogLevel m_minLogLevel;
    bool m_consoleEnabled;
    bool m_fileEnabled;
    std::ofstream m_fileStream;
    std::mutex m_mutex;
};

// Convenience macros
#define LOG_DEBUG(...) Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARNING(...) Logger::getInstance().warning(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().error(__VA_ARGS__)
