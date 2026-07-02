#include "ui/ChannelGridWidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QtMath>

namespace cwl::ui {

ChannelGridWidget::ChannelGridWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);

    m_hoverAnimation = new QPropertyAnimation(this);
    m_hoverAnimation->setDuration(180);
    m_hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void ChannelGridWidget::setGames(const QVector<cwl::games::GameEntry>& games)
{
    // Keep any existing system tiles (they're appended after games), drop
    // old game tiles, then rebuild.
    QVector<ChannelTile> systemTiles;
    for (const auto& t : m_tiles) {
        if (t.isSystemTile) systemTiles.append(t);
    }

    m_tiles.clear();
    for (const auto& g : games) {
        ChannelTile tile;
        tile.label = g.title;
        tile.artwork = g.hasCoverArt ? g.coverArt : QPixmap();
        tile.game = g;
        tile.isSystemTile = false;
        m_tiles.append(tile);
    }
    m_tiles += systemTiles;

    update();
}

void ChannelGridWidget::addSystemTile(const QString& label, const QPixmap& artwork)
{
    ChannelTile tile;
    tile.label = label;
    tile.artwork = artwork;
    tile.isSystemTile = true;
    m_tiles.append(tile);
    update();
}

void ChannelGridWidget::setColumns(int columns)
{
    m_columns = qMax(1, columns);
    update();
}

QVector<ChannelGridWidget::TileGeometry> ChannelGridWidget::layoutTiles() const
{
    QVector<TileGeometry> geometries;
    if (m_tiles.isEmpty()) return geometries;

    const int totalSpacing = m_spacing * (m_columns + 1);
    const int tileWidth = (width() - totalSpacing) / m_columns;
    const int tileHeight = tileWidth; // square channel tiles, Wii-menu style

    for (int i = 0; i < m_tiles.size(); ++i) {
        const int row = i / m_columns;
        const int col = i % m_columns;
        const int x = m_spacing + col * (tileWidth + m_spacing);
        const int y = m_spacing + row * (tileHeight + m_spacing);
        geometries.append({QRect(x, y, tileWidth, tileHeight), i});
    }
    return geometries;
}

int ChannelGridWidget::tileIndexAt(const QPoint& pos) const
{
    for (const auto& g : layoutTiles()) {
        if (g.rect.contains(pos)) return g.index;
    }
    return -1;
}

void ChannelGridWidget::setHoveredIndex(int index)
{
    if (index == m_hoveredIndex) return;
    m_hoveredIndex = index;
    update();
}

void ChannelGridWidget::mouseMoveEvent(QMouseEvent* event)
{
    setHoveredIndex(tileIndexAt(event->pos()));
    QWidget::mouseMoveEvent(event);
}

void ChannelGridWidget::mousePressEvent(QMouseEvent* event)
{
    const int idx = tileIndexAt(event->pos());
    if (idx >= 0 && idx < m_tiles.size()) {
        const ChannelTile& tile = m_tiles[idx];
        if (tile.isSystemTile) {
            emit systemTileActivated(tile.label);
        } else {
            emit gameLaunchRequested(tile.game);
        }
    }
    QWidget::mousePressEvent(event);
}

void ChannelGridWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateGeometry();
}

void ChannelGridWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Soft vertical gradient background (Crystal theme colors; overridden
    // by the active theme.json at runtime by the ThemeManager -- omitted
    // here for brevity, see docs/THEMING.md).
    QLinearGradient bg(0, 0, 0, height());
    bg.setColorAt(0.0, QColor("#dff3ff"));
    bg.setColorAt(1.0, QColor("#a9d8f5"));
    painter.fillRect(rect(), bg);

    const auto geometries = layoutTiles();
    for (const auto& g : geometries) {
        const ChannelTile& tile = m_tiles[g.index];
        const bool hovered = (g.index == m_hoveredIndex);
        const qreal scale = hovered ? 1.08 : 1.0;

        QRectF rect(g.rect);
        const QPointF center = rect.center();
        QRectF scaledRect(0, 0, rect.width() * scale, rect.height() * scale);
        scaledRect.moveCenter(center);

        QPainterPath path;
        path.addRoundedRect(scaledRect, 18, 18);

        painter.save();
        if (hovered) {
            painter.setPen(QPen(QColor("#7ec8ff"), 3));
        } else {
            painter.setPen(QPen(QColor("#ffffff"), 2));
        }
        painter.setBrush(QColor(255, 255, 255, hovered ? 235 : 210));
        painter.drawPath(path);
        painter.restore();

        painter.save();
        painter.setClipPath(path);
        if (!tile.artwork.isNull()) {
            painter.drawPixmap(scaledRect.toRect(), tile.artwork);
        } else {
            // Procedural placeholder tile: initials on a soft accent tint,
            // used until real cover art is fetched/cached.
            painter.fillRect(scaledRect, QColor("#3aa0e8"));
            painter.setPen(Qt::white);
            QFont font = painter.font();
            font.setPointSize(20);
            font.setBold(true);
            painter.setFont(font);
            const QString initials = tile.label.isEmpty() ? QStringLiteral("?") : tile.label.left(2).toUpper();
            painter.drawText(scaledRect, Qt::AlignCenter, initials);
        }
        painter.restore();

        painter.setPen(QColor("#123047"));
        QFont labelFont = painter.font();
        labelFont.setPointSize(10);
        painter.setFont(labelFont);
        painter.drawText(QRectF(rect.x(), rect.bottom() + 4, rect.width(), 20),
                          Qt::AlignHCenter | Qt::AlignTop, tile.label);
    }
}

} // namespace cwl::ui
