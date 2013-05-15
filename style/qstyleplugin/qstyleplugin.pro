include( $$[QT_INSTALL_DATA]/mkspecs/features/meegotouch_common.prf )

TEMPLATE = lib
CONFIG += plugin

QMAKE_LFLAGS += "-Wl,--as-needed"

TARGET = meegotouchqtstyleplugin
target.path = $$[QT_INSTALL_PLUGINS]/styles

contains( M_BUILD_FEATURES, coverage ) {
    QMAKE_CXXFLAGS += -ftest-coverage -fprofile-arcs
    LIBS += -lgcov

    QMAKE_CLEAN += \
    *.gcda \
    *.gcno
}

HEADERS += \
  qtmaemo6styleplugin.h \
  mlibrarychecker.h \

SOURCES = \
  qtmaemo6styleplugin.cpp \
  mlibrarychecker.cpp \

INCLUDEPATH += ..

INSTALLS += target
