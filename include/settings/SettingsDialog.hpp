// =============================================================================
//  SettingsDialog.hpp - Tabbed settings window covering every option group
//  requested: thème, résolution, langue, audio, contrôleurs, chemins,
//  options graphiques, mises à jour.
// =============================================================================
#pragma once

#include <QDialog>
#include <QListWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
class QSpinBox;
class QSlider;
class QListWidget;
class QLineEdit;
QT_END_NAMESPACE

namespace cwl::ui {

/// Presents the full settings surface described in the project spec as a
/// tabbed dialog. Reads from/writes to cwl::core::Settings; callers should
/// re-run any dependent subsystem refresh (e.g. GameLibrary rescanning)
/// after the dialog is accepted, since paths may have changed.
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void onAccept();
    void addGameDirectory();
    void removeSelectedGameDirectory();
    void browseSaveDirectory();
    void browseDolphinExecutable();
    void autoDetectDolphin();
    void checkForUpdatesNow();

private:
    QWidget* buildGeneralTab();
    QWidget* buildGraphicsTab();
    QWidget* buildAudioTab();
    QWidget* buildControllersTab();
    QWidget* buildPathsTab();
    QWidget* buildUpdatesTab();

    void loadFromSettings();

    // General
    QComboBox* m_themeCombo = nullptr;
    QComboBox* m_languageCombo = nullptr;

    // Graphics
    QCheckBox* m_fullscreenCheck = nullptr;
    QComboBox* m_resolutionCombo = nullptr;
    QCheckBox* m_vsyncCheck = nullptr;
    QSpinBox* m_fpsSpin = nullptr;

    // Audio
    QSlider* m_masterVolumeSlider = nullptr;
    QSlider* m_sfxVolumeSlider = nullptr;
    QCheckBox* m_muteCheck = nullptr;

    // Controllers (live status list, read-only)
    QListWidget* m_controllerList = nullptr;

    // Paths
    QListWidget* m_gameDirsList = nullptr;
    QLineEdit* m_saveDirEdit = nullptr;
    QLineEdit* m_dolphinPathEdit = nullptr;

    // Updates
    QCheckBox* m_autoUpdateCheck = nullptr;
};

} // namespace cwl::ui
