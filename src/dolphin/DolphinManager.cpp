#include "dolphin/DolphinManager.hpp"
#include "core/Logger.hpp"

#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>

#ifdef CWL_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace cwl::dolphin {

DolphinManager::DolphinManager(QObject* parent)
    : QObject(parent)
{
    m_process = new QProcess(this);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
        CWL_LOG_INFO("Dolphin", QStringLiteral("Dolphin exited with code %1").arg(exitCode));
        emit gameStopped(exitCode);
    });

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        CWL_LOG_ERROR("Dolphin", QStringLiteral("Process error: %1").arg(static_cast<int>(error)));
        emit launchFailed(QStringLiteral("Impossible de démarrer Dolphin (code d'erreur %1)").arg(static_cast<int>(error)));
    });
}

QString DolphinManager::autoDetectExecutable()
{
    QStringList candidates = {
        QStringLiteral("C:/Program Files/Dolphin-x64/Dolphin.exe"),
        QStringLiteral("C:/Program Files/Dolphin/Dolphin.exe"),
        QStringLiteral("C:/Program Files (x86)/Dolphin-x64/Dolphin.exe"),
        QStringLiteral("C:/Program Files (x86)/Dolphin/Dolphin.exe"),
    };

    // Also check the user's local install and Desktop shortcuts folder,
    // common with the portable .zip distribution of Dolphin.
    const QString userProfile = qEnvironmentVariable("USERPROFILE");
    if (!userProfile.isEmpty()) {
        candidates << userProfile + QStringLiteral("/Dolphin/Dolphin.exe");
        candidates << userProfile + QStringLiteral("/AppData/Local/Dolphin-Emu/Dolphin.exe");
    }

    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            CWL_LOG_INFO("Dolphin", "Auto-detected Dolphin at " + path);
            return path;
        }
    }

    // Fall back to searching the system PATH.
    const QString fromPath = QStandardPaths::findExecutable(QStringLiteral("Dolphin.exe"));
    if (!fromPath.isEmpty()) {
        CWL_LOG_INFO("Dolphin", "Found Dolphin via PATH at " + fromPath);
        return fromPath;
    }

    CWL_LOG_WARNING("Dolphin", "Could not auto-detect a Dolphin installation.");
    return {};
}

bool DolphinManager::isConfigured() const
{
    return !m_executablePath.isEmpty() && QFileInfo::exists(m_executablePath);
}

bool DolphinManager::launchGame(const QString& filePath, bool batchMode)
{
    if (!isConfigured()) {
        emit launchFailed(QStringLiteral("Le chemin de l'exécutable Dolphin n'est pas configuré."));
        return false;
    }

    if (!QFileInfo::exists(filePath)) {
        emit launchFailed(QStringLiteral("Le fichier du jeu est introuvable : %1").arg(filePath));
        return false;
    }

    if (isGameRunning()) {
        CWL_LOG_WARNING("Dolphin", "A game is already running; stopping it first.");
        stopGame();
        m_process->waitForFinished(3000);
    }

    QStringList args;
    if (batchMode) {
        args << QStringLiteral("-b"); // batch mode: close Dolphin when the game stops
    }
    args << QStringLiteral("-e") << QDir::toNativeSeparators(filePath);

    CWL_LOG_INFO("Dolphin", QStringLiteral("Launching: %1 %2").arg(m_executablePath, args.join(' ')));
    m_process->start(m_executablePath, args);

    if (!m_process->waitForStarted(5000)) {
        emit launchFailed(QStringLiteral("Dolphin n'a pas pu démarrer."));
        return false;
    }

    emit gameStarted(filePath);
    return true;
}

void DolphinManager::stopGame()
{
    if (isGameRunning()) {
        CWL_LOG_INFO("Dolphin", "Stopping running game/Dolphin process.");
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
    }
}

bool DolphinManager::openDolphinSettings()
{
    if (!isConfigured()) {
        emit launchFailed(QStringLiteral("Le chemin de l'exécutable Dolphin n'est pas configuré."));
        return false;
    }
    // Launching Dolphin with no arguments opens its own main window, from
    // which the user can reach Graphics/Controllers/Config directly.
    return QProcess::startDetached(m_executablePath, {});
}

bool DolphinManager::isGameRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

} // namespace cwl::dolphin
