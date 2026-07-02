// =============================================================================
//  DolphinManager.hpp - Detects an installed Dolphin Emulator, launches games
//  through it, and forwards its settings/config UI.
// =============================================================================
#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

namespace cwl::dolphin {

/// Wraps a QProcess pointed at the user's Dolphin installation. Crystal Wii
/// Launcher never bundles or redistributes Dolphin -- it only locates and
/// drives an existing local install, as permitted by Dolphin's own license.
class DolphinManager : public QObject {
    Q_OBJECT

public:
    explicit DolphinManager(QObject* parent = nullptr);

    /// Searches common install locations (Program Files, Program Files (x86),
    /// the Microsoft Store package folder, and the user's PATH) for
    /// Dolphin.exe. Returns an empty string if not found.
    static QString autoDetectExecutable();

    /// Explicitly set/get the configured path (persisted via Settings by the
    /// caller).
    void setExecutablePath(const QString& path) { m_executablePath = path; }
    QString executablePath() const { return m_executablePath; }
    bool isConfigured() const;

    /// Launches Dolphin with the given ISO/WBFS/RVZ/GCZ file, optionally in
    /// batch mode (skips the Dolphin UI and goes straight to the game, then
    /// closes Dolphin when the game exits).
    bool launchGame(const QString& filePath, bool batchMode = true);

    /// Closes the currently-running game/Dolphin instance, if any.
    void stopGame();

    /// Opens Dolphin's own configuration window (dolphin.exe with no
    /// arguments) so the user can tweak graphics backend, controller
    /// profiles, etc. directly.
    bool openDolphinSettings();

    bool isGameRunning() const;

signals:
    void gameStarted(const QString& filePath);
    void gameStopped(int exitCode);
    void launchFailed(const QString& reason);

private:
    QString m_executablePath;
    QProcess* m_process = nullptr;
};

} // namespace cwl::dolphin
