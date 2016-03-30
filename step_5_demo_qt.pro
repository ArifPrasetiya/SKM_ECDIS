#-------------------------------------------------
#
# Project created by QtCreator 2012-10-18T13:44:52
#
#-------------------------------------------------

QT       += core gui \
            opengl

TARGET = step_5_demo_qt

unix {

TEMPLATE = app

INCLUDEPATH += \
    . \
    ./../../../.. \
    /usr/include/navtor-sdk

QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-switch -Wno-unused-variable -Wno-unused-function -Wno-reorder

LIBS += \
    -lbase_library \
    -lX11 \
    -lGL
}

win32 {

TEMPLATE = vcapp

INCLUDEPATH += \
    . \
    ./../../../..

LIBS += \
  ../../../../base/out/win/x$(PlatformArchitecture)/$(Configuration)/base_library.lib
}

SOURCES += main.cpp\
        mainwindow.cpp \
    step_5_demo_widget.cpp \
    portrayalparametersdlg.cpp \
    featureinfodlg.cpp \
    enterhwiddlg.cpp \
    databaseupdatehistorydlg.cpp \
    s52_resource_manager.cpp \
    decoration_renderer.cpp \
    addbookmarkdlg.cpp \
    bookmarksdlg.cpp \
    coverage_renderer.cpp \
    markedfeaturerenderer.cpp \
    user_bmp_layer_renderer.cpp \
    glwidget.cpp

HEADERS  += mainwindow.h \
    step_5_demo_widget.h \
    portrayalparametersdlg.h \
    featureinfodlg.h \
    enterhwiddlg.h \
    databaseupdatehistorydlg.h \
    s52_resource_manager.h \
    decoration_renderer.h \
    addbookmarkdlg.h \
    bookmarksdlg.h \
    utils.h \
    coverage_renderer.h \
    mark_unmark_feature_interface.h \
    markedfeaturerenderer.h \
    user_bmp_layer_renderer.h \
    glwidget.h

FORMS    += mainwindow.ui \
    step_5_demo_widget.ui \
    portrayalparametersdlg.ui \
    featureinfodlg.ui \
    enterhwiddlg.ui \
    databaseupdatehistorydlg.ui \
    addbookmarkdlg.ui \
    bookmarksdlg.ui

RESOURCES += \
    step_5_demo.qrc \
