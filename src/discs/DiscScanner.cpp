#include "discs/DiscScanner.hpp"
#include "core/Logger.hpp"

#ifdef CWL_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace cwl::discs {

DiscScanner::DiscScanner(QObject* parent)
    : QObject(parent)
{
    connect(&m_pollTimer, &QTimer::timeout, this, &DiscScanner::pollDrives);
}

void DiscScanner::startPolling(int intervalMs)
{
    CWL_LOG_INFO("DiscScanner", QStringLiteral("Starting optical drive polling every %1ms").arg(intervalMs));
    m_pollTimer.start(intervalMs);
    pollDrives(); // do an immediate first pass
}

void DiscScanner::stopPolling()
{
    m_pollTimer.stop();
}

QStringList DiscScanner::enumerateOpticalDriveLetters()
{
    QStringList letters;

#ifdef CWL_PLATFORM_WINDOWS
    const DWORD mask = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (!(mask & (1 << i))) continue;

        const QString letter = QStringLiteral("%1:").arg(QChar('A' + i));
        const QString rootPath = letter + QStringLiteral("\\");
        const UINT type = GetDriveTypeW(reinterpret_cast<LPCWSTR>(rootPath.utf16()));
        if (type == DRIVE_CDROM) {
            letters.append(letter);
        }
    }
#else
    // Non-Windows builds (contributor convenience only): optical drive
    // enumeration is not implemented. Crystal Wii Launcher targets Windows.
#endif

    return letters;
}

bool DiscScanner::tryReadDiscHeader(const QString& driveLetter, QString& outTitle, QString& outId)
{
#ifdef CWL_PLATFORM_WINDOWS
    // Raw device access requires the \\.\D: form and read access only; no
    // write/format capability is ever requested.
    const QString devicePath = QStringLiteral("\\\\.\\") + driveLetter;

    HANDLE handle = CreateFileW(
        reinterpret_cast<LPCWSTR>(devicePath.utf16()),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    constexpr DWORD kHeaderSize = 0x80; // covers Game ID (0x00) + Title (0x20..0x80)
    BYTE buffer[kHeaderSize] = {};
    DWORD bytesRead = 0;

    const bool ok = ReadFile(handle, buffer, kHeaderSize, &bytesRead, nullptr) && bytesRead == kHeaderSize;
    CloseHandle(handle);

    if (!ok) {
        return false;
    }

    outId = QString::fromLatin1(reinterpret_cast<const char*>(buffer), 6).trimmed();

    QString title = QString::fromLatin1(reinterpret_cast<const char*>(buffer) + 0x20, kHeaderSize - 0x20);
    const int nullPos = title.indexOf(QChar('\0'));
    if (nullPos >= 0) title = title.left(nullPos);
    outTitle = title.trimmed();

    return !outId.isEmpty();
#else
    Q_UNUSED(driveLetter);
    Q_UNUSED(outTitle);
    Q_UNUSED(outId);
    return false;
#endif
}

void DiscScanner::pollDrives()
{
    QVector<OpticalDrive> newState;

    for (const QString& letter : enumerateOpticalDriveLetters()) {
        OpticalDrive drive;
        drive.letter = letter;

        QString title, id;
        drive.hasDisc = tryReadDiscHeader(letter, title, id);
        drive.volumeLabel = title;

        newState.append(drive);

        // Compare against the previous known state for this letter to fire
        // insertion/removal edge-triggered signals.
        const auto previousIt = std::find_if(m_lastKnownState.begin(), m_lastKnownState.end(),
            [&](const OpticalDrive& d) { return d.letter == letter; });

        const bool wasPresent = previousIt != m_lastKnownState.end() && previousIt->hasDisc;

        if (drive.hasDisc && !wasPresent) {
            CWL_LOG_INFO("DiscScanner", QStringLiteral("Disc inserted in %1: \"%2\" (%3)").arg(letter, title, id));
            emit discInserted(letter, title, id);
        } else if (!drive.hasDisc && wasPresent) {
            CWL_LOG_INFO("DiscScanner", QStringLiteral("Disc removed from %1").arg(letter));
            emit discRemoved(letter);
        }
    }

    m_lastKnownState = newState;
    emit drivesChanged(m_lastKnownState);
}

} // namespace cwl::discs
