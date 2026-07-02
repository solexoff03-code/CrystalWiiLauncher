// =============================================================================
//  MainWindow.hpp - Top-level window: hosts the Wii-menu-style channel grid,
//  wires up GameLibrary/DiscScanner/DolphinManager/ControllerManager, and
//  owns the WiiPointer overlay + fullscreen/windowed toggling.
// =============================================================================
#pragma once

#include "games/GameLibrary.hpp"
#include "discs/DiscScanner.hpp"
#include "dolphin/DolphinManager.hpp"
#include "controllers/ControllerManager.hpp"
#include "updater/UpdateManager.hpp"

#include <QMainWindow>
#include <QStackedWidget>

namespace cwl::ui {

class ChannelGridWidget;
class WiiPointer;
class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onGameLaunchRequested(const cwl::games::GameEntry& game);
    void onSystemTileActivated(const QString& label);
    void onDiscInserted(const QString& driveLetter, const QString& title, const QString& id);
    void onControllersChanged(const QVector<cwl::controllers::ControllerStatus>& controllers);
    void onUpdateAvailable(const cwl::updater::ReleaseInfo& release);

    void refreshLibrary();
    void toggleFullscreen();

private:
    void buildUi();
    void wireUpSubsystems();

    ChannelGridWidget* m_channelGrid = nullptr;
    WiiPointer* m_pointer = nullptr;
    SettingsDialog* m_settingsDialog = nullptr;

    cwl::games::GameLibrary m_gameLibrary;
    cwl::discs::DiscScanner m_discScanner;
    cwl::dolphin::DolphinManager m_dolphinManager;
    cwl::controllers::ControllerManager m_controllerManager;
    cwl::updater::UpdateManager m_updateManager;

    bool m_isFullscreen = false;
};

} // namespace cwl::ui
