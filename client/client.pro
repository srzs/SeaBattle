QT += widgets websockets

TARGET = seabattle
TEMPLATE = app
CONFIG += c++14

HEADERS += \
    mainwindow.h \
    gameconfig/gameconfigdialog.h \
    gameconfig/gameconfigmodel.h \
    game/gamewidget.h \
    game/gamecreatedialog.h \
    game/gamemodel.h \
    game/gameclient.h \
    gamebroadcaster.h \
    game/gamejoindialog.h \
    gamelistener.h

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    gameconfig/gameconfigdialog.cpp \
    gameconfig/gameconfigmodel.cpp \
    game/gamewidget.cpp \
    game/gamecreatedialog.cpp \
    game/gamemodel.cpp \
    game/gameclient.cpp \
    gamebroadcaster.cpp \
    game/gamejoindialog.cpp \
    gamelistener.cpp

FORMS += \
    mainwindow.ui \
    gameconfig/gameconfigdialog.ui \
    gameconfig/gameconfigeditdialog.ui \
    game/gamepreparewidget.ui \
    game/gamecreatedialog.ui \
    game/gameconnectwidget.ui \
    game/gamemainwidget.ui \
    game/gamejoindialog.ui

RESOURCES += seabattle.qrc
TRANSLATIONS = seabattle_de.ts

# Always use lrelease if it exists
exists($$[QT_INSTALL_BINS]/lrelease) {
    QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}

# Choose lrelease.exe on Windows
isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}

# Automatically compile translations
TSQM.name = lrelease ${QMAKE_FILE_IN}
TSQM.input = TRANSLATIONS
TSQM.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN}
TSQM.CONFIG = no_link
QMAKE_EXTRA_COMPILERS += TSQM
PRE_TARGETDEPS += compiler_TSQM_make_all

# Common project dependency
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../common/release/ -lcommon
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../common/debug/ -lcommon
else:unix: LIBS += -L$$OUT_PWD/../common/ -lcommon

INCLUDEPATH += $$PWD/../common
DEPENDPATH += $$PWD/../common

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../common/release/libcommon.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../common/debug/libcommon.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../common/release/common.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../common/debug/common.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../common/libcommon.a
