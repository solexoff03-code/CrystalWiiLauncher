#include "ui/MainWindow.hpp"
#include "ui/ChannelGridWidget.hpp"
#include "ui/WiiPointer.hpp"
#include "settings/SettingsDialog.hpp"
#include "core/Settings.hpp"
#include "core/Logger.hpp"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QVBoxLayout>

namespace cwl::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Crystal Wii Launcher"));
    resize(1280, 720);
    setMouseTracking(true);

    buildUi();
    wireUpSubsystems();

    // Kick off background systems.
    auto& settings = cwl::core::Settings::instance();
    if (settings.dolphinExecutablePath().isEmpty()) {
        const QString detected = cwl::dolphin::DolphinManager::autoDetectExecutable();
        if (!detected.isEmpty()) {
            settings.setDolphinExecutablePath(detected);
            settings.save();
        }
    }
    m_dolphinManager.setExecutablePath(settings.dolphinExecutablePath());

    m_discScanner.startPolling();
    m_controllerManager.startMonitoring();
    refreshLibrary();

    if (settings.autoCheckUpdates()) {
        m_updateManager.checkForUpdatesAsync();
    }
}

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    m_channelGrid = new ChannelGridWidget(central);
    m_channelGrid->addSystemTile(QStringLiteral("Paramètres"), QPixmap());
    m_channelGrid->addSystemTile(QStringLiteral("Disque"), QPixmap());
    layout->addWidget(m_channelGrid);

    setCentralWidget(central);

    // The pointer overlay sits above everything else and ignores mouse
    // events itself so clicks pass through to the grid beneath it.
    m_pointer = new WiiPointer(this);
    m_pointer->setGeometry(rect());
    m_pointer->raise();
    m_pointer->show();

    m_settingsDialog = new SettingsDialog(this);
}

void MainWindow::wireUpSubsystems()
{
    connect(m_channelGrid, &ChannelGridWidget::gameLaunchRequested, this, &MainWindow::onGameLaunchRequested);
    connect(m_channelGrid, &ChannelGridWidget::systemTileActivated, this, &MainWindow::onSystemTileActivated);

    connect(&m_gameLibrary, &cwl::games::GameLibrary::scanFinished, this, [this](int total) {
        CWL_LOG_INFO("UI", QStringLiteral("Library scan finished: %1 games").arg(total));
        m_channelGrid->setGames(m_gameLibrary.allGames());
    });

    connect(&m_discScanner, &cwl::discs::DiscScanner::discInserted, this, &MainWindow::onDiscInserted);

    connect(&m_controllerManager, &cwl::controllers::ControllerManager::controllersChanged,
            this, &MainWindow::onControllersChanged);

    connect(&m_dolphinManager, &cwl::dolphin::DolphinManager::launchFailed, this, [this](const QString& reason) {
        QMessageBox::warning(this, QStringLiteral("Dolphin"), reason);
    });

    connect(&m_updateManager, &cwl::updater::UpdateManager::updateAvailable, this, &MainWindow::onUpdateAvailable);
}

void MainWindow::refreshLibrary()
{
    const auto dirs = cwl::core::Settings::instance().gameDirectories();
    if (!dirs.isEmpty()) {
        m_gameLibrary.scanDirectoriesAsync(dirs);
    }
}

void MainWindow::onGameLaunchRequested(const cwl::games::GameEntry& game)
{
    CWL_LOG_INFO("UI", "Launch requested: " + game.title);
    m_dolphinManager.launchGame(game.filePath, /*batchMode=*/true);
}

void MainWindow::onSystemTileActivated(const QString& label)
{
    if (label == QStringLiteral("Paramètres")) {
        m_settingsDialog->exec();
        refreshLibrary(); // paths may have changed
    } else if (label == QStringLiteral("Disque")) {
        // Surface the current optical drive state; DiscScanner already
        // polls continuously, this just prompts an immediate UI refresh.
        QMessageBox::information(this, QStringLiteral("Disque"),
            QStringLiteral("Insérez un disque GameCube ou Wii compatible pour le lancer automatiquement."));
    }
}

void MainWindow::onDiscInserted(const QString& driveLetter, const QString& title, const QString& id)
{
    const QString displayTitle = title.isEmpty() ? QStringLiteral("Disque inconnu") : title;
    CWL_LOG_INFO("UI", QStringLiteral("Disc inserted: %1 (%2) on %3").arg(displayTitle, id, driveLetter));

    const auto result = QMessageBox::question(this, QStringLiteral("Disque détecté"),
        QStringLiteral("« %1 » a été détecté dans %2.\nLancer le jeu avec Dolphin ?").arg(displayTitle, driveLetter),
        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_dolphinManager.launchGame(driveLetter + QStringLiteral("\\"), /*batchMode=*/true);
    }
}

void MainWindow::onControllersChanged(const QVector<cwl::controllers::ControllerStatus>& controllers)
{
    // The Settings dialog's "Contrôleurs" tab subscribes to live status;
    // MainWindow just keeps a log trail here for diagnostics.
    CWL_LOG_DEBUG("UI", QStringLiteral("%1 controller(s) currently detected").arg(controllers.size()));
}

void MainWindow::onUpdateAvailable(const cwl::updater::ReleaseInfo& release)
{
    const auto result = QMessageBox::information(this, QStringLiteral("Mise à jour disponible"),
        QStringLiteral("Crystal Wii Launcher %1 est disponible.\nInstaller maintenant ?").arg(release.version),
        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_updateManager.downloadAndInstall(release);
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (m_pointer) {
        m_pointer->setSourceIsWiimote(false);
        m_pointer->setTargetPosition(event->pos());
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_F11) {
        toggleFullscreen();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::toggleFullscreen()
{
    m_isFullscreen = !m_isFullscreen;
    if (m_isFullscreen) {
        showFullScreen();
    } else {
        showNormal();
    }
    cwl::core::Settings::instance().graphics().fullscreen = m_isFullscreen;
}

} // namespace cwl::ui
