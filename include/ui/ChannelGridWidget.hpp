// =============================================================================
//  ChannelGridWidget.hpp - Custom-painted, animated grid of "channels"
//  (games + system tiles), inspired by the Wii Menu's layout without reusing
//  any of its original artwork.
// =============================================================================
#pragma once

#include "games/GameEntry.hpp"

#include <QWidget>
#include <QVector>
#include <QPropertyAnimation>
#include <QPixmap>

namespace cwl::ui {

/// A single visual tile in the grid: either bound to a GameEntry or a
/// system action (e.g. "Paramètres", "Disque"). Rendered with rounded
/// corners, a soft drop shadow, and a scale/glow animation on hover,
/// matching the "channel" metaphor requested without copying Nintendo art.
struct ChannelTile {
    QString label;
    QPixmap artwork;              // procedurally generated or loaded cover art
    bool isSystemTile = false;    // true for "Paramètres" / "Disque" / etc.
    cwl::games::GameEntry game;   // valid only when !isSystemTile
    qreal currentScale = 1.0;     // animated
};

class ChannelGridWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChannelGridWidget(QWidget* parent = nullptr);

    void setGames(const QVector<cwl::games::GameEntry>& games);
    void addSystemTile(const QString& label, const QPixmap& artwork);

    int columns() const { return m_columns; }
    void setColumns(int columns);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

signals:
    void gameLaunchRequested(const cwl::games::GameEntry& game);
    void systemTileActivated(const QString& label);

private:
    struct TileGeometry { QRect rect; int index; };

    QVector<TileGeometry> layoutTiles() const;
    int tileIndexAt(const QPoint& pos) const;
    void setHoveredIndex(int index);

    QVector<ChannelTile> m_tiles;
    int m_columns = 4;
    int m_spacing = 24;
    int m_hoveredIndex = -1;

    QPropertyAnimation* m_hoverAnimation = nullptr;
};

} // namespace cwl::ui
