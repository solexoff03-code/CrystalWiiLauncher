// =============================================================================
//  ControllerManager.hpp - Unified controller detection and live status.
//  Backends:
//    - SDL2            : Xbox, DualSense, DualShock, generic HID gamepads
//    - Windows Bluetooth: Wiimote pairing/discovery (+ Nunchuk/Classic/
//                         MotionPlus/Balance Board extension identification)
//    - libusb           : Official/compatible GameCube USB adapters
//    - Qt input events   : keyboard/mouse "controller"
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QTimer>

namespace cwl::controllers {

enum class ControllerType {
    Wiimote,
    Nunchuk,
    ClassicController,
    MotionPlus,
    BalanceBoard,
    GameCubeAdapter,
    Xbox,
    DualSense,
    DualShock,
    GenericGamepad,
    KeyboardMouse
};

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    LowBattery
};

struct ControllerStatus {
    ControllerType type;
    QString displayName;
    ConnectionState state = ConnectionState::Disconnected;
    int batteryPercent = -1;    // -1 = unknown/not applicable (e.g. wired pads, KB/M)
    int playerSlot = -1;        // assigned player slot, -1 if unassigned
    QString identifier;         // backend-specific id (Bluetooth address, SDL instance id, ...)
};

/// Aggregates every supported input backend into one polled list of
/// ControllerStatus, refreshed on a timer and exposed via controllersChanged().
///
/// NOTE ON SCOPE: full low-level Wiimote HID report parsing (accelerometer /
/// IR camera / MotionPlus gyro streaming) is a substantial subsystem in its
/// own right and lives in WiimoteBackend (see controllers/WiimoteBackend.*,
/// an extension point stubbed here). This manager focuses on discovery,
/// pairing state and battery/connection status, which is what the launcher
/// UI needs to render the "controllers" screen.
class ControllerManager : public QObject {
    Q_OBJECT

public:
    explicit ControllerManager(QObject* parent = nullptr);
    ~ControllerManager() override;

    void startMonitoring(int pollIntervalMs = 1000);
    void stopMonitoring();

    QVector<ControllerStatus> currentControllers() const { return m_controllers; }

    /// Starts a Windows Bluetooth discovery + pairing flow for Wiimotes
    /// (device class matches Nintendo's registered Wiimote Bluetooth
    /// identifiers). Pairing UI/consent is handled by Windows itself.
    void beginWiimotePairing();

signals:
    void controllersChanged(const QVector<cwl::controllers::ControllerStatus>& controllers);
    void controllerConnected(const cwl::controllers::ControllerStatus& controller);
    void controllerDisconnected(const QString& identifier);
    void wiimotePairingFinished(bool success, const QString& message);

private slots:
    void pollAllBackends();

private:
    QVector<ControllerStatus> pollSdlGamepads();      // Xbox / DualSense / DualShock / generic
    QVector<ControllerStatus> pollWiimoteBackend();   // Bluetooth Wiimote family
    QVector<ControllerStatus> pollGameCubeAdapter();  // libusb
    ControllerStatus keyboardMouseStatus() const;     // always "connected"

    bool initializeSdl();

    QTimer m_pollTimer;
    QVector<ControllerStatus> m_controllers;
    bool m_sdlInitialized = false;
};

} // namespace cwl::controllers
