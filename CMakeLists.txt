cmake_minimum_required(VERSION 3.10)
project(Task3_FeatureStudio VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Qt5 Configuration ──────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

# ─── OpenCV & CURL ──────────────────────────────────────────────────
find_package(OpenCV REQUIRED)
find_package(CURL REQUIRED)

# ─── Include Directories ────────────────────────────────────────────
include_directories(
    src
    ${Qt5Widgets_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

# ─── Source Files ───────────────────────────────────────────────────
file(GLOB_RECURSE FRONTEND_SOURCES
    "src/frontend/*.cpp"
    "src/frontend/*.h"
    "src/frontend/ui/*.ui"
)
file(GLOB_RECURSE BACKEND_SOURCES
    "src/backend/*.cpp"
    "src/backend/*.h"
)

# ─── Executable & Linking ───────────────────────────────────────────
add_executable(launch src/main.cpp ${FRONTEND_SOURCES} ${BACKEND_SOURCES})

target_link_libraries(launch PRIVATE
    Qt5::Widgets
    ${OpenCV_LIBS}
    ${CURL_LIBRARIES}
    -lcurl
)