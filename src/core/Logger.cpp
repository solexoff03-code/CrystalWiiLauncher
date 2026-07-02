#include "core/Logger.hpp"

#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QCoreApplication>

namespace cwl::core {

Logger& Logger::instance()
{
    static Logger s_instance;
    return s_instance;
}

Logger::~Logger()
{
    if (m_stream) {
        m_stream->flush();
    }
}

void Logger::initialize(const QString& logDirectory)
{
    QMutexLocker locker(&m_mutex);
    if (m_initialized) {
        return;
    }

    QDir dir(logDirectory);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    const QString fileName = QStringLiteral("crystal-%1.log")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd")));

    m_file = std::make_unique<QFile>(dir.filePath(fileName));
    if (m_file->open(QIODevice::Append | QIODevice::Text)) {
        m_stream = std::make_unique<QTextStream>(m_file.get());
    } else {
        qWarning() << "Logger: unable to open log file at" << m_file->fileName();
        m_file.reset();
    }

    m_initialized = true;
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:    return QStringLiteral("DEBUG");
        case LogLevel::Info:     return QStringLiteral("INFO");
        case LogLevel::Warning:  return QStringLiteral("WARN");
        case LogLevel::Error:    return QStringLiteral("ERROR");
        case LogLevel::Critical: return QStringLiteral("CRIT");
    }
    return QStringLiteral("?");
}

void Logger::log(LogLevel level, const QString& category, const QString& message)
{
    if (level < m_minimumLevel) {
        return;
    }

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    const QString line = QStringLiteral("[%1] [%2] [%3] %4")
        .arg(timestamp, levelToString(level), category, message);

    QMutexLocker locker(&m_mutex);

    // Always mirror to the console for interactive debugging.
    switch (level) {
        case LogLevel::Debug:
        case LogLevel::Info:
            qInfo().noquote() << line;
            break;
        case LogLevel::Warning:
            qWarning().noquote() << line;
            break;
        case LogLevel::Error:
        case LogLevel::Critical:
            qCritical().noquote() << line;
            break;
    }

    if (m_stream) {
        (*m_stream) << line << Qt::endl;
    }
}

} // namespace cwl::core
