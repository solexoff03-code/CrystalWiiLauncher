// =============================================================================
//  Logger.hpp - Thread-safe, file + console logging system for Crystal Wii
//  Launcher. Writes rotated daily log files into logs/.
// =============================================================================
#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <memory>

namespace cwl::core {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

/// Singleton, thread-safe logger. All subsystems funnel through here so we
/// get a single, consistent logs/crystal-YYYY-MM-DD.log file.
class Logger {
public:
    static Logger& instance();

    /// Must be called once at startup, before any log() calls, with the
    /// directory where log files should be written (defaults to "logs/").
    void initialize(const QString& logDirectory = QStringLiteral("logs"));

    void log(LogLevel level, const QString& category, const QString& message);

    void debug(const QString& category, const QString& message)    { log(LogLevel::Debug, category, message); }
    void info(const QString& category, const QString& message)     { log(LogLevel::Info, category, message); }
    void warning(const QString& category, const QString& message)  { log(LogLevel::Warning, category, message); }
    void error(const QString& category, const QString& message)    { log(LogLevel::Error, category, message); }
    void critical(const QString& category, const QString& message) { log(LogLevel::Critical, category, message); }

    /// Minimum level that gets written; defaults to Debug in debug builds,
    /// Info in release builds.
    void setMinimumLevel(LogLevel level) { m_minimumLevel = level; }

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static QString levelToString(LogLevel level);

    QMutex m_mutex;
    std::unique_ptr<QFile> m_file;
    std::unique_ptr<QTextStream> m_stream;
    LogLevel m_minimumLevel = LogLevel::Debug;
    bool m_initialized = false;
};

} // namespace cwl::core

// Convenience macros so call sites read: CWL_LOG_INFO("Dolphin", "Found exe at ...");
#define CWL_LOG_DEBUG(category, message)    cwl::core::Logger::instance().debug(category, message)
#define CWL_LOG_INFO(category, message)     cwl::core::Logger::instance().info(category, message)
#define CWL_LOG_WARNING(category, message)  cwl::core::Logger::instance().warning(category, message)
#define CWL_LOG_ERROR(category, message)    cwl::core::Logger::instance().error(category, message)
#define CWL_LOG_CRITICAL(category, message) cwl::core::Logger::instance().critical(category, message)
