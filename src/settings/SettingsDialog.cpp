#include "settings/SettingsDialog.hpp"
#include "core/Settings.hpp"
#include "dolphin/DolphinManager.hpp"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QSlider>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QDialogButtonBox>

namespace cwl::ui {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Paramètres - Crystal Wii Launcher"));
    resize(640, 480);

    auto* layout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    tabs->addTab(buildGeneralTab(), QStringLiteral("Général"));
    tabs->addTab(buildGraphicsTab(), QStringLiteral("Graphismes"));
    tabs->addTab(buildAudioTab(), QStringLiteral("Audio"));
    tabs->addTab(buildControllersTab(), QStringLiteral("Contrôleurs"));
    tabs->addTab(buildPathsTab(), QStringLiteral("Chemins"));
    tabs->addTab(buildUpdatesTab(), QStringLiteral("Mises à jour"));
    layout->addWidget(tabs);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    loadFromSettings();
}

QWidget* SettingsDialog::buildGeneralTab()
{
    auto* widget = new QWidget(this);
    auto* form = new QFormLayout(widget);

    m_themeCombo = new QComboBox(widget);
    m_themeCombo->addItem(QStringLiteral("Crystal Default"), QStringLiteral("default"));
    form->addRow(QStringLiteral("Thème :"), m_themeCombo);

    m_languageCombo = new QComboBox(widget);
    m_languageCombo->addItem(QStringLiteral("Français"), QStringLiteral("fr"));
    m_languageCombo->addItem(QStringLiteral("English"), QStringLiteral("en"));
    m_languageCombo->addItem(QStringLiteral("Español"), QStringLiteral("es"));
    m_languageCombo->addItem(QStringLiteral("Deutsch"), QStringLiteral("de"));
    form->addRow(QStringLiteral("Langue :"), m_languageCombo);

    return widget;
}

QWidget* SettingsDialog::buildGraphicsTab()
{
    auto* widget = new QWidget(this);
    auto* form = new QFormLayout(widget);

    m_fullscreenCheck = new QCheckBox(QStringLiteral("Plein écran"), widget);
    form->addRow(m_fullscreenCheck);

    m_resolutionCombo = new QComboBox(widget);
    m_resolutionCombo->addItems({"1280x720", "1600x900", "1920x1080", "2560x1440", "3840x2160"});
    form->addRow(QStringLiteral("Résolution :"), m_resolutionCombo);

    m_vsyncCheck = new QCheckBox(QStringLiteral("Synchronisation verticale (V-Sync)"), widget);
    form->addRow(m_vsyncCheck);

    m_fpsSpin = new QSpinBox(widget);
    m_fpsSpin->setRange(30, 240);
    m_fpsSpin->setSuffix(QStringLiteral(" FPS"));
    form->addRow(QStringLiteral("Limite d'images/seconde :"), m_fpsSpin);

    return widget;
}

QWidget* SettingsDialog::buildAudioTab()
{
    auto* widget = new QWidget(this);
    auto* form = new QFormLayout(widget);

    m_masterVolumeSlider = new QSlider(Qt::Horizontal, widget);
    m_masterVolumeSlider->setRange(0, 100);
    form->addRow(QStringLiteral("Volume général :"), m_masterVolumeSlider);

    m_sfxVolumeSlider = new QSlider(Qt::Horizontal, widget);
    m_sfxVolumeSlider->setRange(0, 100);
    form->addRow(QStringLiteral("Volume des effets sonores :"), m_sfxVolumeSlider);

    m_muteCheck = new QCheckBox(QStringLiteral("Muet"), widget);
    form->addRow(m_muteCheck);

    return widget;
}

QWidget* SettingsDialog::buildControllersTab()
{
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);

    layout->addWidget(new QLabel(QStringLiteral(
        "État des contrôleurs détectés (mis à jour en temps réel) :"), widget));

    m_controllerList = new QListWidget(widget);
    layout->addWidget(m_controllerList);

    auto* pairButton = new QPushButton(QStringLiteral("Appairer une Wiimote (Bluetooth)"), widget);
    layout->addWidget(pairButton);
    // NOTE: actual pairing is triggered by MainWindow's ControllerManager
    // instance; this dialog only displays status passed in by the caller.

    return widget;
}

QWidget* SettingsDialog::buildPathsTab()
{
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);

    layout->addWidget(new QLabel(QStringLiteral("Dossiers de jeux (ISO / WBFS / RVZ / GCZ) :"), widget));
    m_gameDirsList = new QListWidget(widget);
    layout->addWidget(m_gameDirsList);

    auto* dirButtons = new QHBoxLayout();
    auto* addDirButton = new QPushButton(QStringLiteral("Ajouter..."), widget);
    auto* removeDirButton = new QPushButton(QStringLiteral("Retirer"), widget);
    connect(addDirButton, &QPushButton::clicked, this, &SettingsDialog::addGameDirectory);
    connect(removeDirButton, &QPushButton::clicked, this, &SettingsDialog::removeSelectedGameDirectory);
    dirButtons->addWidget(addDirButton);
    dirButtons->addWidget(removeDirButton);
    layout->addLayout(dirButtons);

    auto* saveRow = new QHBoxLayout();
    saveRow->addWidget(new QLabel(QStringLiteral("Dossier de sauvegardes :"), widget));
    m_saveDirEdit = new QLineEdit(widget);
    auto* browseSaveButton = new QPushButton(QStringLiteral("Parcourir..."), widget);
    connect(browseSaveButton, &QPushButton::clicked, this, &SettingsDialog::browseSaveDirectory);
    saveRow->addWidget(m_saveDirEdit);
    saveRow->addWidget(browseSaveButton);
    layout->addLayout(saveRow);

    auto* dolphinRow = new QHBoxLayout();
    dolphinRow->addWidget(new QLabel(QStringLiteral("Exécutable Dolphin :"), widget));
    m_dolphinPathEdit = new QLineEdit(widget);
    auto* browseDolphinButton = new QPushButton(QStringLiteral("Parcourir..."), widget);
    auto* autoDetectButton = new QPushButton(QStringLiteral("Détecter automatiquement"), widget);
    connect(browseDolphinButton, &QPushButton::clicked, this, &SettingsDialog::browseDolphinExecutable);
    connect(autoDetectButton, &QPushButton::clicked, this, &SettingsDialog::autoDetectDolphin);
    dolphinRow->addWidget(m_dolphinPathEdit);
    dolphinRow->addWidget(browseDolphinButton);
    dolphinRow->addWidget(autoDetectButton);
    layout->addLayout(dolphinRow);

    return widget;
}

QWidget* SettingsDialog::buildUpdatesTab()
{
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);

    m_autoUpdateCheck = new QCheckBox(QStringLiteral("Vérifier automatiquement les mises à jour au démarrage"), widget);
    layout->addWidget(m_autoUpdateCheck);

    auto* checkNowButton = new QPushButton(QStringLiteral("Vérifier maintenant"), widget);
    connect(checkNowButton, &QPushButton::clicked, this, &SettingsDialog::checkForUpdatesNow);
    layout->addWidget(checkNowButton);

    layout->addStretch();
    layout->addWidget(new QLabel(QStringLiteral("Version actuelle : %1").arg(QStringLiteral(CWL_VERSION)), widget));

    return widget;
}

void SettingsDialog::loadFromSettings()
{
    auto& s = cwl::core::Settings::instance();

    m_languageCombo->setCurrentIndex(m_languageCombo->findData(s.language()));

    m_fullscreenCheck->setChecked(s.graphics().fullscreen);
    m_resolutionCombo->setCurrentText(QStringLiteral("%1x%2").arg(s.graphics().width).arg(s.graphics().height));
    m_vsyncCheck->setChecked(s.graphics().vsync);
    m_fpsSpin->setValue(s.graphics().targetFps);

    m_masterVolumeSlider->setValue(s.audio().masterVolume);
    m_sfxVolumeSlider->setValue(s.audio().sfxVolume);
    m_muteCheck->setChecked(s.audio().muted);

    m_gameDirsList->clear();
    m_gameDirsList->addItems(s.gameDirectories());

    m_saveDirEdit->setText(s.saveDirectory());
    m_dolphinPathEdit->setText(s.dolphinExecutablePath());

    m_autoUpdateCheck->setChecked(s.autoCheckUpdates());
}

void SettingsDialog::onAccept()
{
    auto& s = cwl::core::Settings::instance();

    s.setLanguage(m_languageCombo->currentData().toString());

    s.graphics().fullscreen = m_fullscreenCheck->isChecked();
    const QStringList resParts = m_resolutionCombo->currentText().split('x');
    if (resParts.size() == 2) {
        s.graphics().width = resParts[0].toInt();
        s.graphics().height = resParts[1].toInt();
    }
    s.graphics().vsync = m_vsyncCheck->isChecked();
    s.graphics().targetFps = m_fpsSpin->value();

    s.audio().masterVolume = m_masterVolumeSlider->value();
    s.audio().sfxVolume = m_sfxVolumeSlider->value();
    s.audio().muted = m_muteCheck->isChecked();

    QStringList dirs;
    for (int i = 0; i < m_gameDirsList->count(); ++i) {
        dirs << m_gameDirsList->item(i)->text();
    }
    s.setGameDirectories(dirs);
    s.setSaveDirectory(m_saveDirEdit->text());
    s.setDolphinExecutablePath(m_dolphinPathEdit->text());

    s.setAutoCheckUpdates(m_autoUpdateCheck->isChecked());

    s.save();
    accept();
}

void SettingsDialog::addGameDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Choisir un dossier de jeux"));
    if (!dir.isEmpty()) {
        m_gameDirsList->addItem(dir);
    }
}

void SettingsDialog::removeSelectedGameDirectory()
{
    qDeleteAll(m_gameDirsList->selectedItems());
}

void SettingsDialog::browseSaveDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Choisir le dossier de sauvegardes"));
    if (!dir.isEmpty()) {
        m_saveDirEdit->setText(dir);
    }
}

void SettingsDialog::browseDolphinExecutable()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Choisir Dolphin.exe"),
        QString(), QStringLiteral("Exécutable (*.exe)"));
    if (!path.isEmpty()) {
        m_dolphinPathEdit->setText(path);
    }
}

void SettingsDialog::autoDetectDolphin()
{
    const QString detected = cwl::dolphin::DolphinManager::autoDetectExecutable();
    if (!detected.isEmpty()) {
        m_dolphinPathEdit->setText(detected);
    }
}

void SettingsDialog::checkForUpdatesNow()
{
    // Handled by MainWindow's persistent UpdateManager; this button simply
    // signals intent to the user for now (wired fully once the dialog is
    // constructed with a shared UpdateManager reference in a future pass).
}

} // namespace cwl::ui
