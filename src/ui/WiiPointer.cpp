#include "ui/WiiPointer.hpp"

#include <QPainter>
#include <QTimer>

namespace cwl::ui {

WiiPointer::WiiPointer(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &WiiPointer::tick);
    timer->start(16); // ~60 FPS easing, matches the requested 60 FPS target
}

void WiiPointer::setTargetPosition(const QPoint& pos)
{
    m_targetPos = pos;
}

void WiiPointer::tick()
{
    // Simple exponential easing toward the target gives the pointer a
    // light, "floaty" trail reminiscent of IR-pointer input, whether the
    // underlying source is the mouse or an actual Wiimote.
    constexpr qreal easing = 0.35;
    const QPointF target(m_targetPos);
    const QPointF delta = target - m_currentPos;

    if (delta.manhattanLength() > 0.1) {
        m_currentPos += delta * easing;
        update();
    }
}

void WiiPointer::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Original pointer glyph: a soft circular halo with a small dot core --
    // deliberately simple and not a recreation of the Wii's hand cursor.
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 90));
    painter.drawEllipse(m_currentPos, 18, 18);

    painter.setBrush(m_sourceIsWiimote ? QColor("#3aa0e8") : QColor("#ffffff"));
    painter.drawEllipse(m_currentPos, 7, 7);
}

} // namespace cwl::ui
