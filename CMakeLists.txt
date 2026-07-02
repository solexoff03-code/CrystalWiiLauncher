# =============================================================================
#  Crystal Wii Launcher - Root CMake build script
#  A modern, open-source Wii-menu-inspired frontend for Dolphin Emulator.
# =============================================================================
cmake_minimum_required(VERSION 3.21)

project(CrystalWiiLauncher
    VERSION 0.1.0
    DESCRIPTION "A modern, open-source Wii-menu-inspired launcher/frontend for Dolphin Emulator"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# Global settings
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Windows is the primary target (Win32 disc I/O, Bluetooth Wiimote pairing,
# libusb GameCube adapter support). The project still configures on other
# platforms with those subsystems compiled out, for contributor convenience.
if(NOT WIN32)
    message(WARNING "Crystal Wii Launcher targets Windows. Some subsystems "
                     "(disc detection, Bluetooth Wiimote) will be stubbed out "
                     "on this platform.")
endif()

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Multimedia Svg)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBUSB QUIET libusb-1.0)
endif()

find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(json)
endif()

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------
file(GLOB_RECURSE CWL_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE CWL_HEADERS CONFIGURE_DEPENDS include/*.hpp)

add_executable(CrystalWiiLauncher WIN32
    ${CWL_SOURCES}
    ${CWL_HEADERS}
    assets/resources.qrc
)

target_include_directories(CrystalWiiLauncher PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(CrystalWiiLauncher PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::Svg
    SDL2::SDL2
    OpenGL::GL
    nlohmann_json::nlohmann_json
)

if(LIBUSB_FOUND)
    target_include_directories(CrystalWiiLauncher PRIVATE ${LIBUSB_INCLUDE_DIRS})
    target_link_libraries(CrystalWiiLauncher PRIVATE ${LIBUSB_LIBRARIES})
    target_compile_definitions(CrystalWiiLauncher PRIVATE CWL_HAVE_LIBUSB=1)
endif()

if(WIN32)
    target_link_libraries(CrystalWiiLauncher PRIVATE
        Bthprops.lib   # Bluetooth Windows API (Wiimote discovery/pairing)
        Setupapi.lib   # Device enumeration (disc drives, HID devices)
        winmm.lib
    )
    target_compile_definitions(CrystalWiiLauncher PRIVATE
        CWL_PLATFORM_WINDOWS=1
        UNICODE
        _UNICODE
    )
endif()

target_compile_definitions(CrystalWiiLauncher PRIVATE
    CWL_VERSION="${PROJECT_VERSION}"
)

# -----------------------------------------------------------------------------
# Install rules
# -----------------------------------------------------------------------------
install(TARGETS CrystalWiiLauncher RUNTIME DESTINATION .)
install(DIRECTORY themes/ DESTINATION themes OPTIONAL)
install(DIRECTORY assets/ DESTINATION assets)

# -----------------------------------------------------------------------------
# Testing (optional)
# -----------------------------------------------------------------------------
option(CWL_BUILD_TESTS "Build unit tests" OFF)
if(CWL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
