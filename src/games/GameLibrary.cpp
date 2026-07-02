#include "games/GameLibrary.hpp"
#include "core/Logger.hpp"

#include <QDirIterator>
#include <QtConcurrent/QtConcurrent>
#include <QFile>
#include <QFileInfo>
#include <algorithm>

namespace cwl::games {

namespace {

// GameCube/Wii disc headers begin with a 6-byte Game ID (e.g. "RMCE01")
// followed by a 2-byte maker code, then a 0x60-byte ASCII title at offset
// 0x20. This layout is documented publicly (WiiBrew / GC-Forever wikis) and
// is common to raw ISO images and the payload wrapped inside GCZ files.
constexpr qint64 kHeaderGameIdOffset = 0x00;
constexpr qint64 kHeaderGameIdLength = 6;
constexpr qint64 kHeaderTitleOffset = 0x20;
constexpr qint64 kHeaderTitleLength = 0x60;

GameRegion regionFromGameId(const QString& id)
{
    if (id.isEmpty()) return GameRegion::Unknown;
    const QChar regionChar = id.at(id.length() - 3 >= 0 ? 3 : 0);
    switch (regionChar.toLatin1()) {
        case 'J': return GameRegion::NTSC_J;
        case 'E': return GameRegion::NTSC_U;
        case 'P': return GameRegion::PAL;
        default:  return GameRegion::Unknown;
    }
}

} // namespace

GameLibrary::GameLibrary(QObject* parent)
    : QObject(parent)
{
}

bool GameLibrary::isSupportedExtension(const QString& suffix)
{
    const QString lower = suffix.toLower();
    return lower == QStringLiteral("iso") ||
           lower == QStringLiteral("wbfs") ||
           lower == QStringLiteral("rvz") ||
           lower == QStringLiteral("gcz");
}

GameFormat GameLibrary::formatFromExtension(const QString& suffix)
{
    const QString lower = suffix.toLower();
    if (lower == QStringLiteral("iso"))  return GameFormat::ISO;
    if (lower == QStringLiteral("wbfs")) return GameFormat::WBFS;
    if (lower == QStringLiteral("rvz"))  return GameFormat::RVZ;
    if (lower == QStringLiteral("gcz"))  return GameFormat::GCZ;
    return GameFormat::Unknown;
}

GameEntry GameLibrary::parseGameFile(const QString& filePath)
{
    GameEntry entry;
    QFileInfo info(filePath);

    entry.filePath = filePath;
    entry.format = formatFromExtension(info.suffix());
    entry.fileSizeBytes = info.size();
    entry.dateAdded = info.birthTime().isValid() ? info.birthTime() : QDateTime::currentDateTime();
    entry.title = info.completeBaseName(); // sensible fallback if header parsing fails

    // Only ISO and GCZ expose the raw disc header at a fixed offset in a
    // straightforward way. WBFS wraps partitions behind its own directory
    // table and RVZ uses Dolphin's own compressed container format; both
    // require dedicated parsers (left as extension points -- see
    // docs/FORMATS.md) and are intentionally not decoded byte-for-byte here.
    if (entry.format == GameFormat::ISO) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            file.seek(kHeaderGameIdOffset);
            const QByteArray idBytes = file.read(kHeaderGameIdLength);
            entry.id = QString::fromLatin1(idBytes).trimmed();

            file.seek(kHeaderTitleOffset);
            const QByteArray titleBytes = file.read(kHeaderTitleLength);
            QString parsedTitle = QString::fromLatin1(titleBytes);
            parsedTitle = parsedTitle.left(parsedTitle.indexOf(QChar('\0')) < 0
                                                ? parsedTitle.length()
                                                : parsedTitle.indexOf(QChar('\0')));
            parsedTitle = parsedTitle.trimmed();
            if (!parsedTitle.isEmpty()) {
                entry.title = parsedTitle;
            }
            entry.region = regionFromGameId(entry.id);
        }
    }

    return entry;
}

void GameLibrary::scanDirectoriesAsync(const QStringList& directories)
{
    if (m_watcher && m_watcher->isRunning()) {
        m_watcher->cancel();
        m_watcher->waitForFinished();
    }

    emit scanStarted();

    // Phase 1 (fast, on this thread): walk the filesystem to build the file
    // list. Phase 2 (slow, on the thread pool): parse each file's header.
    QStringList files;
    for (const QString& dir : directories) {
        QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            if (isSupportedExtension(QFileInfo(path).suffix())) {
                files.append(path);
            }
        }
    }

    CWL_LOG_INFO("GameLibrary", QStringLiteral("Found %1 candidate game files, parsing headers...").arg(files.size()));

    m_watcher = new QFutureWatcher<QVector<GameEntry>>(this);

    connect(m_watcher, &QFutureWatcher<QVector<GameEntry>>::finished, this, [this]() {
        m_games = m_watcher->result();
        std::sort(m_games.begin(), m_games.end(), [](const GameEntry& a, const GameEntry& b) {
            return a.title.compare(b.title, Qt::CaseInsensitive) < 0;
        });
        for (const auto& g : m_games) {
            emit gameDiscovered(g);
        }
        emit scanFinished(m_games.size());
        m_watcher->deleteLater();
    });

    QFuture<QVector<GameEntry>> future = QtConcurrent::run([files]() {
        QVector<GameEntry> results;
        results.reserve(files.size());
        for (const QString& path : files) {
            results.append(parseGameFile(path));
        }
        return results;
    });

    m_watcher->setFuture(future);
}

QVector<GameEntry> GameLibrary::allGames() const
{
    return m_games;
}

QVector<GameEntry> GameLibrary::search(const QString& query) const
{
    if (query.isEmpty()) {
        return m_games;
    }
    QVector<GameEntry> results;
    for (const auto& g : m_games) {
        if (g.title.contains(query, Qt::CaseInsensitive)) {
            results.append(g);
        }
    }
    return results;
}

QVector<GameEntry> GameLibrary::sorted(SortMode mode) const
{
    QVector<GameEntry> copy = m_games;
    switch (mode) {
        case SortMode::TitleAscending:
            std::sort(copy.begin(), copy.end(), [](auto& a, auto& b) { return a.title.compare(b.title, Qt::CaseInsensitive) < 0; });
            break;
        case SortMode::TitleDescending:
            std::sort(copy.begin(), copy.end(), [](auto& a, auto& b) { return a.title.compare(b.title, Qt::CaseInsensitive) > 0; });
            break;
        case SortMode::MostRecentlyPlayed:
            std::sort(copy.begin(), copy.end(), [](auto& a, auto& b) { return a.lastPlayed > b.lastPlayed; });
            break;
        case SortMode::MostPlayed:
            std::sort(copy.begin(), copy.end(), [](auto& a, auto& b) { return a.playTimeSeconds > b.playTimeSeconds; });
            break;
        case SortMode::SizeDescending:
            std::sort(copy.begin(), copy.end(), [](auto& a, auto& b) { return a.fileSizeBytes > b.fileSizeBytes; });
            break;
    }
    return copy;
}

void GameLibrary::toggleFavorite(const QString& gameId)
{
    for (auto& g : m_games) {
        if (g.id == gameId) {
            g.isFavorite = !g.isFavorite;
            break;
        }
    }
}

void GameLibrary::recordPlaySession(const QString& gameId, qint64 secondsPlayed)
{
    for (auto& g : m_games) {
        if (g.id == gameId) {
            g.playTimeSeconds += secondsPlayed;
            g.lastPlayed = QDateTime::currentDateTime();
            break;
        }
    }
}

} // namespace cwl::games
