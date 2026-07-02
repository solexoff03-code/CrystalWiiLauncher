// =============================================================================
//  main.cpp - Crystal Wii Launcher entry point.
//  Initializes logging, loads settings, and shows the main window.
// =============================================================================
#include "core/Logger.hpp"
#include "core/Settings.hpp"
#include "ui/MainWindow.hpp"

#include <QApplication>
#include <QSurfaceFormat>
#include <QDir>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Crystal Wii Launcher"));
    QApplication::setApplicationVersion(QStringLiteral(CWL_VERSION));
    QApplication::setOrganizationName(QStringLiteral("Crystal Wii Launcher Community"));

    // Target 60 FPS rendering with vsync for the animated channel grid /
    // pointer overlay, as requested.
    QSurfaceFormat format;
    format.setSwapInterval(1);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);

    // Ensure the runtime folder layout exists next to the executable so a
    // fresh checkout/build produces a self-contained portable app.
    for (const QString& dir : {"logs", "cache", "saves", "settings", "themes"}) {
        QDir().mkpath(dir);
    }

    cwl::core::Logger::instance().initialize(QStringLiteral("logs"));
    CWL_LOG_INFO("App", QStringLiteral("Crystal Wii Launcher v%1 starting up").arg(CWL_VERSION));

    cwl::core::Settings::instance().load(QStringLiteral("settings/config.json"));

    cwl::ui::MainWindow window;
    if (cwl::core::Settings::instance().graphics().fullscreen) {
        window.showFullScreen();
    } else {
        window.show();
    }

    const int result = app.exec();
    CWL_LOG_INFO("App", "Shutting down.");
    return result;
}
