// =============================================================================
//  GameEntry.hpp - Metadata describing a single scanned game (ISO/WBFS/RVZ/GCZ).
// =============================================================================
#pragma once

#include <QString>
#include <QDateTime>
#include <QPixmap>

namespace cwl::games {

enum class GameFormat {
    ISO,
    WBFS,
    RVZ,
    GCZ,
    Unknown
};

enum class GameRegion {
    NTSC_J,   // Japan
    NTSC_U,   // North America
    PAL,      // Europe / Australia
    Unknown
};

/// A single library entry. Cheap to copy (QString/QPixmap are implicitly
/// shared), so it's safe to pass by value across threads/signals.
struct GameEntry {
    QString id;                 // 6-char Game ID parsed from the disc header, when available
    QString title;
    QString filePath;
    GameFormat format = GameFormat::Unknown;
    GameRegion region = GameRegion::Unknown;
    qint64 fileSizeBytes = 0;

    QPixmap coverArt;            // Empty if no cover art was found/downloaded
    bool hasCoverArt = false;

    bool isFavorite = false;
    qint64 playTimeSeconds = 0;
    QDateTime lastPlayed;
    QDateTime dateAdded;

    QString formatLabel() const
    {
        switch (format) {
            case GameFormat::ISO:  return QStringLiteral("ISO");
            case GameFormat::WBFS: return QStringLiteral("WBFS");
            case GameFormat::RVZ:  return QStringLiteral("RVZ");
            case GameFormat::GCZ:  return QStringLiteral("GCZ");
            default:               return QStringLiteral("?");
        }
    }

    QString regionLabel() const
    {
        switch (region) {
            case GameRegion::NTSC_J: return QStringLiteral("NTSC-J");
            case GameRegion::NTSC_U: return QStringLiteral("NTSC-U");
            case GameRegion::PAL:    return QStringLiteral("PAL");
            default:                 return QStringLiteral("?");
        }
    }
};

} // namespace cwl::games
