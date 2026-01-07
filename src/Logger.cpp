#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

Logger& Logger::getInstance()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_minLogLevel(LogLevel::Debug)
    , m_consoleEnabled(true)
    , m_fileEnabled(false)
{
}

Logger::~Logger()
{
    if (m_fileStream.is_open())
    {
        m_fileStream.close();
    }
}

void Logger::setLogLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minLogLevel = level;
}

void Logger::enableFileLogging(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_fileStream.is_open())
    {
        m_fileStream.close();
    }

    m_fileStream.open(filename, std::ios::out | std::ios::app);
    if (m_fileStream.is_open())
    {
        m_fileEnabled = true;
    }
}

void Logger::disableFileLogging()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_fileStream.is_open())
    {
        m_fileStream.close();
    }
    m_fileEnabled = false;
}

void Logger::enableConsoleLogging(bool enable)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_consoleEnabled = enable;
}

void Logger::log(LogLevel level, const std::string& message, const char* file, int line)
{
    if (level < m_minLogLevel)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string timestamp = getCurrentTime();
    std::string levelStr = levelToString(level);

    std::ostringstream logMessage;
    logMessage << "[" << timestamp << "] [" << levelStr << "] " << message;

    if (file && line >= 0)
    {
        logMessage << " (" << file << ":" << line << ")";
    }

    if (m_consoleEnabled)
    {
        const char* color = levelToColor(level);
        const char* reset = "\033[0m";

        if (level == LogLevel::Error)
        {
            std::cerr << color << logMessage.str() << reset << std::endl;
        }
        else
        {
            std::cout << color << logMessage.str() << reset << std::endl;
        }
    }

    if (m_fileEnabled && m_fileStream.is_open())
    {
        m_fileStream << logMessage.str() << std::endl;
        m_fileStream.flush();
    }
}

std::string Logger::formatString(const std::string& format)
{
    return format;
}

std::string Logger::getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &time);
#else
    localtime_r(&time, &timeInfo);
#endif

    std::ostringstream oss;
    oss << std::put_time(&timeInfo, "%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}

std::string Logger::levelToString(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

const char* Logger::levelToColor(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Debug:   return "\033[36m";  // Cyan
        case LogLevel::Info:    return "\033[32m";  // Green
        case LogLevel::Warning: return "\033[33m";  // Yellow
        case LogLevel::Error:   return "\033[31m";  // Red
        default:                return "\033[0m";   // Reset
    }
}
