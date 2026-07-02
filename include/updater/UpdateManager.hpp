// =============================================================================
//  UpdateManager.hpp - Checks GitHub Releases for newer Crystal Wii Launcher
//  versions and offers to download/apply them.
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

namespace cwl::updater {

struct ReleaseInfo {
    QString version;         // e.g. "0.2.0"
    QString htmlUrl;         // GitHub release page
    QString downloadUrl;     // direct asset download (installer/zip)
    QString changelog;
    bool isNewer = false;
};

/// Talks to the public GitHub REST API
/// (GET /repos/<owner>/<repo>/releases/latest) — no authentication needed
/// for public repos, respects GitHub's rate limits.
class UpdateManager : public QObject {
    Q_OBJECT

public:
    explicit UpdateManager(QObject* parent = nullptr);

    void setRepository(const QString& owner, const QString& repo);

    /// Fires off an async check; results arrive via updateAvailable() /
    /// upToDate() / checkFailed().
    void checkForUpdatesAsync();

    /// Downloads the release asset to a temp file and, on completion,
    /// launches it (installer) or extracts it (zip) then restarts the app.
    void downloadAndInstall(const ReleaseInfo& release);

signals:
    void updateAvailable(const cwl::updater::ReleaseInfo& release);
    void upToDate();
    void checkFailed(const QString& reason);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString& localFilePath);

private:
    static bool isVersionNewer(const QString& remote, const QString& current);

    QNetworkAccessManager m_network;
    QString m_owner = QStringLiteral("crystal-wii-launcher");
    QString m_repo = QStringLiteral("CrystalWiiLauncher");
};

} // namespace cwl::updater
