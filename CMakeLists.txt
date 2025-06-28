cmake_minimum_required(VERSION 3.16)
project(Foccuss VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Sql Network)
find_package(X11 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

message(STATUS "Qt6_DIR: ${Qt6_DIR}")
message(STATUS "SQLITE3_INCLUDE_DIRS: ${SQLITE3_INCLUDE_DIRS}")
message(STATUS "SQLITE3_LIBRARIES: ${SQLITE3_LIBRARIES}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLITE3_INCLUDE_DIRS}
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6Network_INCLUDE_DIRS}
    ${Qt6Gui_INCLUDE_DIRS}
    ${X11_INCLUDE_DIR}
)

add_definitions(-DLINUX_BUILD)

set(SOURCES
    src/main.cpp
    src/core/appdetector.cpp
    src/core/appmonitor.cpp
    src/data/appmodel.cpp
    src/data/database.cpp
    src/data/blockTimeSettingsModel.cpp
    src/service/apiservice.cpp
    src/service/linuxservice.cpp
    src/ui/mainwindow.cpp
    src/ui/blockoverlay.cpp
    src/ui/applistmodel.cpp
)

set(HEADERS
    include/Common.h
    include/ForwardDeclarations.h
    src/core/appdetector.h
    src/core/appmonitor.h
    src/data/appmodel.h
    src/data/database.h
    src/data/blockTimeSettingsModel.h
    src/service/apiservice.h
    src/service/linuxservice.h
    src/ui/mainwindow.h
    src/ui/blockoverlay.h
    src/ui/applistmodel.h
)

set(RESOURCES
    resources/resources.qrc
)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.png")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.png" "DUMMY")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/block_icon.png")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/block_icon.png" "DUMMY")
endif()

add_executable(Foccuss ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(Foccuss PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Sql
    Qt6::Network
    ${X11_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    pthread
)

target_compile_definitions(Foccuss PRIVATE
    QT_WIDGETS_LIB
    QT_CORE_LIB
    QT_GUI_LIB
    QT_SQL_LIB
    QT_NETWORK_LIB
)

install(TARGETS Foccuss
    RUNTIME DESTINATION bin
)

install(FILES resources/foccuss.desktop
    DESTINATION share/applications
)

install(FILES resources/foccuss.service
    DESTINATION lib/systemd/user
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/foccuss.png
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/foccuss.svg
    DESTINATION share/icons/hicolor/256x256/apps
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Foccuss PRIVATE -Wall -Wextra)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(Foccuss PRIVATE -Werror)
    endif()
endif()
