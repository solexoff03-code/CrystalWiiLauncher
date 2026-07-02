// =============================================================================
//  DiscScanner.hpp - Polls Windows optical drives for inserted GameCube/Wii
//  discs, using Win32 (GetLogicalDrives + DeviceIoControl / GetDriveType).
// =============================================================================
#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <QVector>

namespace cwl::discs {

struct OpticalDrive {
    QString letter;      // e.g. "D:"
    bool hasDisc = false;
    QString volumeLabel;
};

/// Periodically polls all optical (CD/DVD/Blu-ray) drives on the system.
/// When a disc is inserted, attempts to read its GameCube/Wii header the
/// same way GameLibrary does for ISO files, and emits discInserted() so the
/// UI can play the insertion animation and offer to launch it via Dolphin.
class DiscScanner : public QObject {
    Q_OBJECT

public:
    explicit DiscScanner(QObject* parent = nullptr);

    /// Starts polling every `intervalMs` milliseconds (default 2s -- disc
    /// drive polling is cheap and doesn't need to be faster than that).
    void startPolling(int intervalMs = 2000);
    void stopPolling();

    QVector<OpticalDrive> currentDrives() const { return m_lastKnownState; }

signals:
    void discInserted(const QString& driveLetter, const QString& gameTitle, const QString& gameId);
    void discRemoved(const QString& driveLetter);
    void drivesChanged(const QVector<cwl::discs::OpticalDrive>& drives);

private slots:
    void pollDrives();

private:
    /// Enumerates all drive letters on the system that Windows reports as
    /// DRIVE_CDROM. Returns e.g. {"D:", "E:"}.
    static QStringList enumerateOpticalDriveLetters();

    /// Attempts to read the disc header from the raw device path
    /// (e.g. "\\.\D:") to extract a title/id, mirroring the ISO parser.
    static bool tryReadDiscHeader(const QString& driveLetter, QString& outTitle, QString& outId);

    QTimer m_pollTimer;
    QVector<OpticalDrive> m_lastKnownState;
};

} // namespace cwl::discs
