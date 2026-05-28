cmake_minimum_required(VERSION 3.16)

project(FoxBender VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(LIBDXFRW_SOURCES
        third_party/libdxfrw/src/intern/drw_dbg.cpp
        third_party/libdxfrw/src/intern/drw_textcodec.cpp
        third_party/libdxfrw/src/intern/dwgbuffer.cpp
        third_party/libdxfrw/src/intern/dwgreader.cpp
        third_party/libdxfrw/src/intern/dwgreader15.cpp
        third_party/libdxfrw/src/intern/dwgreader18.cpp
        third_party/libdxfrw/src/intern/dwgreader21.cpp
        third_party/libdxfrw/src/intern/dwgreader24.cpp
        third_party/libdxfrw/src/intern/dwgreader27.cpp
        third_party/libdxfrw/src/intern/dwgutil.cpp
        third_party/libdxfrw/src/intern/dxfreader.cpp
        third_party/libdxfrw/src/intern/dxfwriter.cpp
        third_party/libdxfrw/src/intern/rscodec.cpp
        third_party/libdxfrw/src/drw_base.cpp
        third_party/libdxfrw/src/drw_classes.cpp
        third_party/libdxfrw/src/drw_entities.cpp
        third_party/libdxfrw/src/drw_header.cpp
        third_party/libdxfrw/src/drw_objects.cpp
        third_party/libdxfrw/src/libdwgr.cpp
        third_party/libdxfrw/src/libdxfrw.cpp
)

add_library(dxfrw_vendor STATIC ${LIBDXFRW_SOURCES})
target_include_directories(dxfrw_vendor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libdxfrw/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libdxfrw/src/intern
)
target_compile_features(dxfrw_vendor PUBLIC cxx_std_14)

set(PROJECT_SOURCES
        main.cpp
        src/app/appsettings.cpp
        src/app/appsettings.h
        src/core/projectdocument.cpp
        src/core/projectdocument.h
        src/core/dxfdocument.cpp
        src/core/dxfdocument.h
        src/core/dxfentity.cpp
        src/core/dxfentity.h
        src/core/geometrymodel.h
        src/core/layerdefinition.h
        src/core/toolstation.cpp
        src/core/toolstation.h
        src/io/jsonstores.cpp
        src/io/jsonstores.h
        src/io/foxdxfreader.cpp
        src/io/foxdxfreader.h
        src/ui/mainwindow.cpp
        src/ui/mainwindow.h
        src/ui/mainwindow.ui
        src/ui/widgets/dxfviewwidget.cpp
        src/ui/widgets/dxfviewwidget.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(FoxBender
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET FoxBender APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(FoxBender SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(FoxBender
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(FoxBender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(FoxBender PRIVATE Qt${QT_VERSION_MAJOR}::Widgets dxfrw_vendor)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.FoxBender)
endif()
set_target_properties(FoxBender PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS FoxBender
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(FoxBender)
endif()
