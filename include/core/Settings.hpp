// =============================================================================
//  Settings.hpp - Persistent JSON-backed application settings.
//  Stored at settings/config.json. Covers theme, resolution, language, audio,
//  controller bindings, game/save paths, graphics options and update checks.
// =============================================================================
#pragma once

#include <QString>
#include <QStringList>
#include <nlohmann/json.hpp>

namespace cwl::core {

struct GraphicsOptions {
    bool fullscreen = false;
    int width = 1280;
    int height = 720;
    bool vsync = true;
    int targetFps = 60;
};

struct AudioOptions {
    int masterVolume = 80;   // 0-100
    int sfxVolume = 100;     // 0-100
    bool muted = false;
};

/// Central, thread-safe-on-main-thread settings object. Loaded once at
/// startup and saved whenever the user changes something in the Settings UI.
class Settings {
public:
    static Settings& instance();

    /// Loads settings/config.json, creating it with defaults if missing.
    void load(const QString& path = QStringLiteral("settings/config.json"));
    void save();

    // --- General -------------------------------------------------------
    QString theme() const { return m_theme; }
    void setTheme(const QString& theme) { m_theme = theme; }

    QString language() const { return m_language; }
    void setLanguage(const QString& lang) { m_language = lang; }

    // --- Graphics --------------------------------------------------------
    GraphicsOptions& graphics() { return m_graphics; }
    const GraphicsOptions& graphics() const { return m_graphics; }

    // --- Audio -------------------------------------------------------------
    AudioOptions& audio() { return m_audio; }
    const AudioOptions& audio() const { return m_audio; }

    // --- Paths -------------------------------------------------------------
    QStringList gameDirectories() const { return m_gameDirectories; }
    void setGameDirectories(const QStringList& dirs) { m_gameDirectories = dirs; }
    void addGameDirectory(const QString& dir);

    QString saveDirectory() const { return m_saveDirectory; }
    void setSaveDirectory(const QString& dir) { m_saveDirectory = dir; }

    QString dolphinExecutablePath() const { return m_dolphinPath; }
    void setDolphinExecutablePath(const QString& path) { m_dolphinPath = path; }

    // --- Updates -------------------------------------------------------------
    bool autoCheckUpdates() const { return m_autoCheckUpdates; }
    void setAutoCheckUpdates(bool enabled) { m_autoCheckUpdates = enabled; }

private:
    Settings() = default;

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

    QString m_configPath;

    QString m_theme = QStringLiteral("default");
    QString m_language = QStringLiteral("fr");

    GraphicsOptions m_graphics;
    AudioOptions m_audio;

    QStringList m_gameDirectories;
    QString m_saveDirectory = QStringLiteral("saves");
    QString m_dolphinPath;

    bool m_autoCheckUpdates = true;
};

} // namespace cwl::core
