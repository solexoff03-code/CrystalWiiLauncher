// =============================================================================
//  GameLibrary.hpp - Scans configured directories for ISO/WBFS/RVZ/GCZ files,
//  parses header metadata asynchronously, caches cover art, and exposes a
//  searchable/sortable model of the user's game collection.
// =============================================================================
#pragma once

#include "games/GameEntry.hpp"

#include <QObject>
#include <QStringList>
#include <QVector>
#include <QFutureWatcher>

namespace cwl::games {

enum class SortMode {
    TitleAscending,
    TitleDescending,
    MostRecentlyPlayed,
    MostPlayed,
    SizeDescending
};

/// Owns the in-memory game collection. Scanning runs on a background thread
/// pool (via QtConcurrent) so the UI never blocks; results stream back via
/// the gameDiscovered() signal and scanFinished() when the pass completes.
class GameLibrary : public QObject {
    Q_OBJECT

public:
    explicit GameLibrary(QObject* parent = nullptr);

    /// Kicks off an asynchronous recursive scan of all configured
    /// directories. Safe to call again while a scan is running -- the
    /// previous scan is cancelled first.
    void scanDirectoriesAsync(const QStringList& directories);

    /// Returns the current in-memory library (thread-safe snapshot).
    QVector<GameEntry> allGames() const;

    /// Returns games matching a case-insensitive title substring search.
    QVector<GameEntry> search(const QString& query) const;

    /// Returns a sorted copy of the current library.
    QVector<GameEntry> sorted(SortMode mode) const;

    void toggleFavorite(const QString& gameId);
    void recordPlaySession(const QString& gameId, qint64 secondsPlayed);

    static bool isSupportedExtension(const QString& suffix);
    static GameFormat formatFromExtension(const QString& suffix);

signals:
    void scanStarted();
    void gameDiscovered(const cwl::games::GameEntry& entry);
    void scanProgress(int scanned, int totalEstimate);
    void scanFinished(int totalGames);

private:
    /// Parses a single disc image's header to extract the Game ID, title and
    /// region. Runs on a worker thread. Returns a partially-filled GameEntry.
    static GameEntry parseGameFile(const QString& filePath);

    QVector<GameEntry> m_games;
    QFutureWatcher<QVector<GameEntry>>* m_watcher = nullptr;
};

} // namespace cwl::games
