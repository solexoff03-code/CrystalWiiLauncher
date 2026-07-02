#include "controllers/ControllerManager.hpp"
#include "core/Logger.hpp"

#include <SDL.h>

#ifdef CWL_PLATFORM_WINDOWS
#include <windows.h>
#include <bluetoothapis.h>
#endif

#ifdef CWL_HAVE_LIBUSB
#include <libusb.h>
#endif

namespace cwl::controllers {

ControllerManager::ControllerManager(QObject* parent)
    : QObject(parent)
{
    connect(&m_pollTimer, &QTimer::timeout, this, &ControllerManager::pollAllBackends);
    initializeSdl();
}

ControllerManager::~ControllerManager()
{
    if (m_sdlInitialized) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
    }
}

bool ControllerManager::initializeSdl()
{
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
        CWL_LOG_ERROR("Controllers", QStringLiteral("SDL init failed: %1").arg(SDL_GetError()));
        return false;
    }
    m_sdlInitialized = true;
    CWL_LOG_INFO("Controllers", "SDL2 gamepad subsystem initialized.");
    return true;
}

void ControllerManager::startMonitoring(int pollIntervalMs)
{
    m_pollTimer.start(pollIntervalMs);
    pollAllBackends();
}

void ControllerManager::stopMonitoring()
{
    m_pollTimer.stop();
}

ControllerType classifySdlController(const char* name)
{
    const QString n = QString::fromUtf8(name).toLower();
    if (n.contains("xbox")) return ControllerType::Xbox;
    if (n.contains("dualsense") || n.contains("ps5")) return ControllerType::DualSense;
    if (n.contains("dualshock") || n.contains("ps4") || n.contains("ps3")) return ControllerType::DualShock;
    return ControllerType::GenericGamepad;
}

QVector<ControllerStatus> ControllerManager::pollSdlGamepads()
{
    QVector<ControllerStatus> results;
    if (!m_sdlInitialized) return results;

    // Let SDL process its internal device-added/removed queue.
    SDL_GameControllerUpdate();

    const int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks; ++i) {
        if (!SDL_IsGameController(i)) continue;

        SDL_GameController* pad = SDL_GameControllerOpen(i);
        if (!pad) continue;

        const char* name = SDL_GameControllerName(pad);

        ControllerStatus status;
        status.type = classifySdlController(name ? name : "");
        status.displayName = QString::fromUtf8(name ? name : "Manette inconnue");
        status.state = ConnectionState::Connected;
        status.identifier = QStringLiteral("sdl-%1").arg(i);
        status.playerSlot = i;

#if SDL_VERSION_ATLEAST(2, 0, 14)
        const int battery = SDL_JoystickCurrentPowerLevel(SDL_GameControllerGetJoystick(pad));
        switch (battery) {
            case SDL_JOYSTICK_POWER_EMPTY:  status.batteryPercent = 5;  status.state = ConnectionState::LowBattery; break;
            case SDL_JOYSTICK_POWER_LOW:    status.batteryPercent = 20; status.state = ConnectionState::LowBattery; break;
            case SDL_JOYSTICK_POWER_MEDIUM: status.batteryPercent = 60; break;
            case SDL_JOYSTICK_POWER_FULL:   status.batteryPercent = 100; break;
            case SDL_JOYSTICK_POWER_WIRED:  status.batteryPercent = -1; break;
            default: status.batteryPercent = -1; break;
        }
#endif

        results.append(status);
        SDL_GameControllerClose(pad);
    }

    return results;
}

QVector<ControllerStatus> ControllerManager::pollWiimoteBackend()
{
    QVector<ControllerStatus> results;

#ifdef CWL_PLATFORM_WINDOWS
    // High-level discovery only: enumerate already-paired Bluetooth HID
    // devices and match against the Wiimote's registered device class.
    // Live HID report streaming (buttons/accelerometer/IR/extensions) is
    // implemented in the dedicated WiimoteBackend extension point.
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = {};
    searchParams.dwSize = sizeof(searchParams);
    searchParams.fReturnAuthenticated = TRUE;
    searchParams.fReturnRemembered = TRUE;
    searchParams.fReturnConnected = TRUE;
    searchParams.fReturnUnknown = FALSE;
    searchParams.fIssueInquiry = FALSE;
    searchParams.cTimeoutMultiplier = 1;

    BLUETOOTH_DEVICE_INFO deviceInfo = {};
    deviceInfo.dwSize = sizeof(deviceInfo);

    HBLUETOOTH_DEVICE_FIND findHandle = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    if (findHandle) {
        do {
            const QString deviceName = QString::fromWCharArray(deviceInfo.szName);
            if (deviceName.contains("Nintendo", Qt::CaseInsensitive) ||
                deviceName.contains("Wiimote", Qt::CaseInsensitive) ||
                deviceName.contains("RVL", Qt::CaseInsensitive)) {

                ControllerStatus status;
                status.type = ControllerType::Wiimote;
                status.displayName = deviceName.isEmpty() ? QStringLiteral("Wiimote") : deviceName;
                status.state = deviceInfo.fConnected ? ConnectionState::Connected : ConnectionState::Disconnected;
                status.identifier = QStringLiteral("bt-%1").arg(
                    QString::number(deviceInfo.Address.ullLong, 16));
                results.append(status);
            }
        } while (BluetoothFindNextDevice(findHandle, &deviceInfo));
        BluetoothFindDeviceClose(findHandle);
    }
#endif

    return results;
}

QVector<ControllerStatus> ControllerManager::pollGameCubeAdapter()
{
    QVector<ControllerStatus> results;

#ifdef CWL_HAVE_LIBUSB
    // Official/compatible GameCube USB adapters (Nintendo/Mayflash) expose a
    // well-known VID/PID pair (057E:0337 for the official Nintendo adapter).
    static libusb_context* ctx = nullptr;
    if (!ctx) {
        libusb_init(&ctx);
    }

    libusb_device** deviceList = nullptr;
    const ssize_t count = libusb_get_device_list(ctx, &deviceList);

    for (ssize_t i = 0; i < count; ++i) {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(deviceList[i], &desc) != 0) continue;

        const bool isOfficialAdapter = (desc.idVendor == 0x057E && desc.idProduct == 0x0337);
        const bool isMayflashAdapter = (desc.idVendor == 0x0079 && desc.idProduct == 0x1843);

        if (isOfficialAdapter || isMayflashAdapter) {
            ControllerStatus status;
            status.type = ControllerType::GameCubeAdapter;
            status.displayName = QStringLiteral("Adaptateur manette GameCube USB");
            status.state = ConnectionState::Connected;
            status.identifier = QStringLiteral("usb-%1-%2").arg(desc.idVendor).arg(desc.idProduct);
            results.append(status);
        }
    }

    if (deviceList) {
        libusb_free_device_list(deviceList, 1);
    }
#endif

    return results;
}

ControllerStatus ControllerManager::keyboardMouseStatus() const
{
    ControllerStatus status;
    status.type = ControllerType::KeyboardMouse;
    status.displayName = QStringLiteral("Clavier / Souris");
    status.state = ConnectionState::Connected; // always available
    status.identifier = QStringLiteral("kbm-0");
    return status;
}

void ControllerManager::pollAllBackends()
{
    QVector<ControllerStatus> updated;
    updated += pollSdlGamepads();
    updated += pollWiimoteBackend();
    updated += pollGameCubeAdapter();
    updated += keyboardMouseStatus();

    // Diff against the previous snapshot to emit granular connect/disconnect
    // signals, in addition to the bulk controllersChanged() signal.
    for (const auto& c : updated) {
        const bool wasKnown = std::any_of(m_controllers.begin(), m_controllers.end(),
            [&](const ControllerStatus& old) { return old.identifier == c.identifier; });
        if (!wasKnown) {
            emit controllerConnected(c);
        }
    }
    for (const auto& old : m_controllers) {
        const bool stillPresent = std::any_of(updated.begin(), updated.end(),
            [&](const ControllerStatus& c) { return c.identifier == old.identifier; });
        if (!stillPresent) {
            emit controllerDisconnected(old.identifier);
        }
    }

    m_controllers = updated;
    emit controllersChanged(m_controllers);
}

void ControllerManager::beginWiimotePairing()
{
#ifdef CWL_PLATFORM_WINDOWS
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = {};
    searchParams.dwSize = sizeof(searchParams);
    searchParams.fReturnAuthenticated = TRUE;
    searchParams.fReturnRemembered = FALSE;
    searchParams.fReturnConnected = FALSE;
    searchParams.fReturnUnknown = TRUE;
    searchParams.fIssueInquiry = TRUE;   // actively scan for new devices
    searchParams.cTimeoutMultiplier = 8; // ~10 seconds

    BLUETOOTH_DEVICE_INFO deviceInfo = {};
    deviceInfo.dwSize = sizeof(deviceInfo);

    bool found = false;
    HBLUETOOTH_DEVICE_FIND findHandle = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    if (findHandle) {
        do {
            const QString deviceName = QString::fromWCharArray(deviceInfo.szName);
            if (deviceName.contains("Nintendo", Qt::CaseInsensitive) ||
                deviceName.contains("RVL", Qt::CaseInsensitive)) {
                // Press 1+2 on the Wiimote during discovery to make it
                // pairable; Windows handles the actual bonding handshake.
                found = true;
                break;
            }
        } while (BluetoothFindNextDevice(findHandle, &deviceInfo));
        BluetoothFindDeviceClose(findHandle);
    }

    emit wiimotePairingFinished(found,
        found ? QStringLiteral("Wiimote détectée, appairage en cours...")
              : QStringLiteral("Aucune Wiimote trouvée. Appuyez sur les boutons 1+2 et réessayez."));
#else
    emit wiimotePairingFinished(false, QStringLiteral("L'appairage Bluetooth n'est disponible que sous Windows."));
#endif
}

} // namespace cwl::controllers
