#include "core/Settings.hpp"
#include "core/Logger.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <fstream>

namespace cwl::core {

Settings& Settings::instance()
{
    static Settings s_instance;
    return s_instance;
}

void Settings::addGameDirectory(const QString& dir)
{
    if (!m_gameDirectories.contains(dir, Qt::CaseInsensitive)) {
        m_gameDirectories.append(dir);
    }
}

void Settings::load(const QString& path)
{
    m_configPath = path;
    QFileInfo info(path);

    if (!info.exists()) {
        CWL_LOG_INFO("Settings", "No config.json found, creating defaults at " + path);
        QDir().mkpath(info.absolutePath());
        save();
        return;
    }

    std::ifstream in(path.toStdString());
    if (!in.is_open()) {
        CWL_LOG_ERROR("Settings", "Failed to open " + path + " for reading, using defaults.");
        return;
    }

    try {
        nlohmann::json j;
        in >> j;
        fromJson(j);
        CWL_LOG_INFO("Settings", "Loaded configuration from " + path);
    } catch (const std::exception& e) {
        CWL_LOG_ERROR("Settings", QStringLiteral("Failed to parse config.json: %1").arg(e.what()));
    }
}

void Settings::save()
{
    if (m_configPath.isEmpty()) {
        m_configPath = QStringLiteral("settings/config.json");
    }

    QFileInfo info(m_configPath);
    QDir().mkpath(info.absolutePath());

    std::ofstream out(m_configPath.toStdString());
    if (!out.is_open()) {
        CWL_LOG_ERROR("Settings", "Failed to open config.json for writing.");
        return;
    }

    out << toJson().dump(4);
    CWL_LOG_INFO("Settings", "Saved configuration to " + m_configPath);
}

nlohmann::json Settings::toJson() const
{
    nlohmann::json j;
    j["theme"] = m_theme.toStdString();
    j["language"] = m_language.toStdString();

    j["graphics"]["fullscreen"] = m_graphics.fullscreen;
    j["graphics"]["width"] = m_graphics.width;
    j["graphics"]["height"] = m_graphics.height;
    j["graphics"]["vsync"] = m_graphics.vsync;
    j["graphics"]["target_fps"] = m_graphics.targetFps;

    j["audio"]["master_volume"] = m_audio.masterVolume;
    j["audio"]["sfx_volume"] = m_audio.sfxVolume;
    j["audio"]["muted"] = m_audio.muted;

    std::vector<std::string> dirs;
    for (const auto& d : m_gameDirectories) {
        dirs.push_back(d.toStdString());
    }
    j["game_directories"] = dirs;
    j["save_directory"] = m_saveDirectory.toStdString();
    j["dolphin_path"] = m_dolphinPath.toStdString();
    j["auto_check_updates"] = m_autoCheckUpdates;

    return j;
}

void Settings::fromJson(const nlohmann::json& j)
{
    if (j.contains("theme")) m_theme = QString::fromStdString(j.at("theme").get<std::string>());
    if (j.contains("language")) m_language = QString::fromStdString(j.at("language").get<std::string>());

    if (j.contains("graphics")) {
        const auto& g = j.at("graphics");
        m_graphics.fullscreen = g.value("fullscreen", false);
        m_graphics.width = g.value("width", 1280);
        m_graphics.height = g.value("height", 720);
        m_graphics.vsync = g.value("vsync", true);
        m_graphics.targetFps = g.value("target_fps", 60);
    }

    if (j.contains("audio")) {
        const auto& a = j.at("audio");
        m_audio.masterVolume = a.value("master_volume", 80);
        m_audio.sfxVolume = a.value("sfx_volume", 100);
        m_audio.muted = a.value("muted", false);
    }

    if (j.contains("game_directories")) {
        m_gameDirectories.clear();
        for (const auto& d : j.at("game_directories")) {
            m_gameDirectories.append(QString::fromStdString(d.get<std::string>()));
        }
    }

    if (j.contains("save_directory")) {
        m_saveDirectory = QString::fromStdString(j.at("save_directory").get<std::string>());
    }
    if (j.contains("dolphin_path")) {
        m_dolphinPath = QString::fromStdString(j.at("dolphin_path").get<std::string>());
    }
    if (j.contains("auto_check_updates")) {
        m_autoCheckUpdates = j.at("auto_check_updates").get<bool>();
    }
}

} // namespace cwl::core
