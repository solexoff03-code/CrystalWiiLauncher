#include "updater/UpdateManager.hpp"
#include "core/Logger.hpp"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>

namespace cwl::updater {

UpdateManager::UpdateManager(QObject* parent)
    : QObject(parent)
{
}

void UpdateManager::setRepository(const QString& owner, const QString& repo)
{
    m_owner = owner;
    m_repo = repo;
}

bool UpdateManager::isVersionNewer(const QString& remote, const QString& current)
{
    auto parse = [](const QString& v) {
        QString cleaned = v;
        if (cleaned.startsWith('v', Qt::CaseInsensitive)) cleaned.remove(0, 1);
        const QStringList parts = cleaned.split('.');
        int major = parts.size() > 0 ? parts[0].toInt() : 0;
        int minor = parts.size() > 1 ? parts[1].toInt() : 0;
        int patch = parts.size() > 2 ? parts[2].toInt() : 0;
        return std::make_tuple(major, minor, patch);
    };
    return parse(remote) > parse(current);
}

void UpdateManager::checkForUpdatesAsync()
{
    const QString url = QStringLiteral("https://api.github.com/repos/%1/%2/releases/latest")
        .arg(m_owner, m_repo);

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "CrystalWiiLauncher-UpdateChecker");

    CWL_LOG_INFO("Updater", "Checking for updates: " + url);

    QNetworkReply* reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            CWL_LOG_ERROR("Updater", "Update check failed: " + reply->errorString());
            emit checkFailed(reply->errorString());
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit checkFailed(QStringLiteral("Réponse GitHub invalide."));
            return;
        }

        const QJsonObject obj = doc.object();
        ReleaseInfo info;
        info.version = obj.value("tag_name").toString();
        info.htmlUrl = obj.value("html_url").toString();
        info.changelog = obj.value("body").toString();

        const QJsonArray assets = obj.value("assets").toArray();
        for (const auto& assetVal : assets) {
            const QJsonObject asset = assetVal.toObject();
            const QString name = asset.value("name").toString();
            if (name.endsWith(".exe") || name.endsWith(".zip") || name.endsWith(".msi")) {
                info.downloadUrl = asset.value("browser_download_url").toString();
                break;
            }
        }

        info.isNewer = isVersionNewer(info.version, QStringLiteral(CWL_VERSION));

        if (info.isNewer) {
            CWL_LOG_INFO("Updater", QStringLiteral("Update available: %1").arg(info.version));
            emit updateAvailable(info);
        } else {
            CWL_LOG_INFO("Updater", "Already up to date.");
            emit upToDate();
        }
    });
}

void UpdateManager::downloadAndInstall(const ReleaseInfo& release)
{
    if (release.downloadUrl.isEmpty()) {
        emit checkFailed(QStringLiteral("Aucun fichier de mise à jour disponible pour cette version."));
        return;
    }

    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString localPath = QDir(tempDir).filePath(QStringLiteral("CrystalWiiLauncher-update-%1").arg(release.version)
        + QFileInfo(release.downloadUrl).suffix().prepend('.'));

    QNetworkRequest request{QUrl(release.downloadUrl)};
    QNetworkReply* reply = m_network.get(request);

    auto* file = new QFile(localPath, this);
    if (!file->open(QIODevice::WriteOnly)) {
        emit checkFailed(QStringLiteral("Impossible d'écrire le fichier de mise à jour."));
        delete file;
        return;
    }

    connect(reply, &QNetworkReply::downloadProgress, this, &UpdateManager::downloadProgress);
    connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
        file->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, localPath]() {
        file->close();
        file->deleteLater();
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit checkFailed(reply->errorString());
            return;
        }

        CWL_LOG_INFO("Updater", "Downloaded update to " + localPath);
        emit downloadFinished(localPath);

        // Installers (.exe/.msi) can be launched directly; the running app
        // should exit shortly after to let the installer replace files.
        if (localPath.endsWith(".exe") || localPath.endsWith(".msi")) {
            QProcess::startDetached(localPath, {});
        }
    });
}

} // namespace cwl::updater
