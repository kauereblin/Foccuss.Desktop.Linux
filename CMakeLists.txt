cmake_minimum_required(VERSION 3.16)
project(Foccuss VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add our custom CMake modules path
#set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Qt
find_package(Qt6 COMPONENTS Widgets Core Sql REQUIRED)

# SQLite
find_package(SQLite3 REQUIRED)

# Display paths for debugging
message(STATUS "SQLite3_INCLUDE_DIRS: ${SQLite3_INCLUDE_DIRS}")
message(STATUS "SQLite3_LIBRARIES: ${SQLite3_LIBRARIES}")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLite3_INCLUDE_DIRS}
)

# Define source files
set(SOURCES
    src/main.cpp
    
    # UI components
    src/ui/mainwindow.cpp
    src/ui/blockoverlay.cpp
    src/ui/applistmodel.cpp
    
    # Core functionality
    src/core/appdetector.cpp
    src/core/appmonitor.cpp
    
    # Service functionality
    src/service/linuxservice.cpp
    
    # Data management
    src/data/database.cpp
    src/data/appmodel.cpp
    src/data/blockTimeSettingsModel.cpp
)

# Define header files
set(HEADERS
    src/ui/mainwindow.h
    src/ui/blockoverlay.h
    src/ui/applistmodel.h
    src/core/appdetector.h
    src/core/appmonitor.h
    src/service/linuxservice.h
    src/data/database.h
    src/data/appmodel.h
    src/data/blockTimeSettingsModel.h
    include/Common.h
    include/ForwardDeclarations.h
    include/QtVersionCheck.h
)

# Resource files
set(RESOURCES
    resources/resources.qrc
)

# Create dummy icon files if they don't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.png")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.png" "DUMMY")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/block_icon.png")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/block_icon.png" "DUMMY")
endif()

# Create executable
add_executable(Foccuss ${SOURCES} ${HEADERS} ${RESOURCES})

# Link libraries
target_link_libraries(Foccuss PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Sql
    ${SQLite3_LIBRARIES}
)

# Install rules
install(TARGETS Foccuss
    RUNTIME DESTINATION bin
)

# Install desktop file
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/foccuss.desktop
    DESTINATION share/applications
)

# Install icons
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.png
    DESTINATION share/icons/hicolor/128x128/apps
    RENAME foccuss.png
)

# Create desktop file if it doesn't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/foccuss.desktop")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/resources/foccuss.desktop"
"[Desktop Entry]
Type=Application
Name=Foccuss
Comment=Application blocking tool to help you focus
Exec=Foccuss
Icon=foccuss
Categories=Utility;
"
    )
endif()

# Include subdirectories if needed
# add_subdirectory(tests) 