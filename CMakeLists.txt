cmake_minimum_required(VERSION 3.21)

project(BigA VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(BigA WIN32
    src/main.cpp
    src/App.cpp
    src/App.h
    src/AppPaths.cpp
    src/AppPaths.h
    src/ConfigManager.cpp
    src/ConfigManager.h
    src/Constants.h
    src/KeyBinding.cpp
    src/KeyBinding.h
    src/Log.cpp
    src/Log.h
    src/MainOverlayWindow.cpp
    src/MainOverlayWindow.h
    src/RawInputManager.cpp
    src/RawInputManager.h
    src/RuntimeState.h
    src/SettingsWindow.cpp
    src/SettingsWindow.h
    src/TrayController.cpp
    src/TrayController.h
    src/UserConfig.cpp
    src/UserConfig.h
    src/widgets/CapacityEdit.cpp
    src/widgets/CapacityEdit.h
    src/widgets/FramelessTitleBar.cpp
    src/widgets/FramelessTitleBar.h
    src/widgets/KeyBindingEdit.cpp
    src/widgets/KeyBindingEdit.h
    src/widgets/ParameterEdit.cpp
    src/widgets/ParameterEdit.h
    resources/app.rc
    resources/resources.qrc
)

target_include_directories(BigA PRIVATE src)

target_link_libraries(BigA PRIVATE
    Qt6::Widgets
    user32
)

target_compile_definitions(BigA PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(NOT QT6_IS_SHARED_LIBS_BUILD)
    # Static Qt: use the static MSVC runtime (/MT) and embed the platform and
    # image-format plugins required by the application into the executable.
    set_property(TARGET BigA PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    set(_biga_static_plugins
        Qt6::QWindowsIntegrationPlugin
        Qt6::QModernWindowsStylePlugin
        Qt6::QGifPlugin
        Qt6::QICOPlugin
        Qt6::QJpegPlugin
    )
    set(_biga_import_plugins)
    foreach(_plugin IN LISTS _biga_static_plugins)
        if(TARGET ${_plugin})
            list(APPEND _biga_import_plugins ${_plugin})
        else()
            message(WARNING "Static Qt plugin target not found: ${_plugin}")
        endif()
    endforeach()
    if(_biga_import_plugins)
        qt_import_plugins(BigA INCLUDE ${_biga_import_plugins})
    endif()
endif()

if(MSVC)
    target_compile_options(BigA PRIVATE /utf-8 /W4 /permissive-)
else()
    target_compile_options(BigA PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(BigA PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "BigA"
)
