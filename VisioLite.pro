TRANSLATIONS += visualbuild_en.ts
QT       += core gui scxml network
QT += serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    State.cpp \
    ZoomController.cpp \
    backitem.cpp \
    bridgeitem.cpp \
    codeexecutor.cpp \
    codehighlighter.cpp \
    codepreviewdialog.cpp \
    commentitem.cpp \
    connect.cpp \
    cpptranslator.cpp \
    cpptranslator2.cpp \
    debuggerwindow.cpp \
    diagramitem.cpp \
    diagramview.cpp \
    dynamicclasstranslator.cpp \
    dynamicmicrotranslator.cpp \
    dynamicwificlasstranslator.cpp \
    dynamicwifitranslator.cpp \
    editabletextitem.cpp \
    graphmanager.cpp \
    groupitem.cpp \
    hardwarelistener.cpp \
    hardwarewifilistener.cpp \
    headerdialog.cpp \
    historyitem.cpp \
    historymanager.cpp \
    javatranslator.cpp \
    javatranslator2.cpp \
    librarymanagerdialog.cpp \
    logicstatemachine.cpp \
    main.cpp \
    mainwindow.cpp \
    microcontrollertranslator.cpp \
    microtranslator2.cpp \
    networkmanager.cpp \
    projectsidebar.cpp \
    propertiesdialog.cpp \
    pythontranslator.cpp \
    pythontranslator2.cpp \
    resizehandle.cpp \
    scxmlcompiler.cpp \
    scxmlhighlighter.cpp \
    statetable.cpp \
    syntaxregistry.cpp \
    transitiongroup.cpp \
    utils.cpp

HEADERS += \
    Grid.h \
    State.h \
    ZoomController.h \
    backitem.h \
    bridgeitem.h \
    codeexecutor.h \
    codehighlighter.h \
    codepreviewdialog.h \
    commentitem.h \
    connect.h \
    cpptranslator.h \
    cpptranslator2.h \
    debuggerwindow.h \
    diagramitem.h \
    diagramview.h \
    dynamicclasstranslator.h \
    dynamicmicrotranslator.h \
    dynamicwificlasstranslator.h \
    dynamicwifitranslator.h \
    editabletextitem.h \
    elementsregistry.h \
    graphmanager.h \
    groupitem.h \
    hardwarelistener.h \
    hardwarewifilistener.h \
    headerdialog.h \
    historyitem.h \
    historymanager.h \
    javatranslator.h \
    javatranslator2.h \
    librarymanagerdialog.h \
    logicstatemachine.h \
    mainwindow.h \
    microcontrollertranslator.h \
    microtranslator2.h \
    networkmanager.h \
    projectsidebar.h \
    propertiesdialog.h \
    pythontranslator.h \
    pythontranslator2.h \
    resizehandle.h \
    scxmlcompiler.h \
    scxmlhighlighter.h \
    statetable.h \
    syntaxregistry.h \
    transitiongroup.h \
    utils.h

FORMS += \
    DebuggerWindow.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

RC_ICONS = StateMachineicon.ico
