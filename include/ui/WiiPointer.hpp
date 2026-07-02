// =============================================================================
//  WiiPointer.hpp - Draws an animated on-screen hand/pointer cursor overlay,
//  positioned either by the real mouse or by Wiimote IR pointer data.
// =============================================================================
#pragma once

#include <QWidget>
#include <QPoint>

namespace cwl::ui {

/// Transparent, click-through overlay widget stacked above the rest of the
/// UI that renders a smooth, slightly-trailing pointer cursor -- evoking the
/// Wii Menu's pointer without reusing its original glove/hand artwork.
class WiiPointer : public QWidget {
    Q_OBJECT

public:
    explicit WiiPointer(QWidget* parent = nullptr);

    /// Called continuously (mouseMoveEvent forwarding from MainWindow, or a
    /// Wiimote IR update) to move the pointer's target position. The
    /// rendered position eases toward the target for a smooth trailing feel.
    void setTargetPosition(const QPoint& pos);

    void setSourceIsWiimote(bool isWiimote) { m_sourceIsWiimote = isWiimote; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void tick(); // advances the eased position, called from a QTimer

    QPointF m_currentPos;
    QPoint m_targetPos;
    bool m_sourceIsWiimote = false;
};

} // namespace cwl::ui
