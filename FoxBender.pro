QT += widgets

CONFIG += c++17

TARGET = FoxBender
TEMPLATE = app

INCLUDEPATH += .

SOURCES += \
    main.cpp \
    src/app/appsettings.cpp \
    src/core/dxfdocument.cpp \
    src/core/dxfentity.cpp \
    src/core/projectdocument.cpp \
    src/core/toolstation.cpp \
    src/io/foxdxfreader.cpp \
    src/io/jsonstores.cpp \
    src/ui/mainwindow.cpp \
    src/ui/widgets/dxfviewwidget.cpp \
    third_party/libdxfrw/src/intern/drw_dbg.cpp \
    third_party/libdxfrw/src/intern/drw_textcodec.cpp \
    third_party/libdxfrw/src/intern/dwgbuffer.cpp \
    third_party/libdxfrw/src/intern/dwgreader.cpp \
    third_party/libdxfrw/src/intern/dwgreader15.cpp \
    third_party/libdxfrw/src/intern/dwgreader18.cpp \
    third_party/libdxfrw/src/intern/dwgreader21.cpp \
    third_party/libdxfrw/src/intern/dwgreader24.cpp \
    third_party/libdxfrw/src/intern/dwgreader27.cpp \
    third_party/libdxfrw/src/intern/dwgutil.cpp \
    third_party/libdxfrw/src/intern/dxfreader.cpp \
    third_party/libdxfrw/src/intern/dxfwriter.cpp \
    third_party/libdxfrw/src/intern/rscodec.cpp \
    third_party/libdxfrw/src/drw_base.cpp \
    third_party/libdxfrw/src/drw_classes.cpp \
    third_party/libdxfrw/src/drw_entities.cpp \
    third_party/libdxfrw/src/drw_header.cpp \
    third_party/libdxfrw/src/drw_objects.cpp \
    third_party/libdxfrw/src/libdwgr.cpp \
    third_party/libdxfrw/src/libdxfrw.cpp

HEADERS += \
    src/app/appsettings.h \
    src/core/dxfdocument.h \
    src/core/dxfentity.h \
    src/core/geometrymodel.h \
    src/core/layerdefinition.h \
    src/core/projectdocument.h \
    src/core/toolstation.h \
    src/io/foxdxfreader.h \
    src/io/jsonstores.h \
    src/ui/mainwindow.h \
    src/ui/widgets/dxfviewwidget.h \
    third_party/libdxfrw/src/drw_base.h \
    third_party/libdxfrw/src/drw_classes.h \
    third_party/libdxfrw/src/drw_entities.h \
    third_party/libdxfrw/src/drw_header.h \
    third_party/libdxfrw/src/drw_interface.h \
    third_party/libdxfrw/src/drw_objects.h \
    third_party/libdxfrw/src/libdwgr.h \
    third_party/libdxfrw/src/libdxfrw.h

FORMS += \
    src/ui/mainwindow.ui

INCLUDEPATH += \
    third_party/libdxfrw/src \
    third_party/libdxfrw/src/intern

DISTFILES += \
    codex_ohje_teralinja_qt_cpp_v_1.md
