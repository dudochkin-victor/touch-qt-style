include( $$[QT_INSTALL_DATA]/mkspecs/features/meegotouch_common.prf )

system(pkg-config --exists xdamage) {
    DEFINES += HAVE_XDAMAGE
} 

system(pkg-config --exists contextsubscriber-1.0) {
    DEFINES += HAVE_CONTEXTSUBSCRIBER
}

contains(DEFINES, HAVE_MEEGOGRAPHICSSYSTEM) {
    message("Using meegographicssystemhelper")
    QT += meegographicssystemhelper
}

TEMPLATE = lib
CONFIG += plugin meegotouch link_pkgconfig

QMAKE_LFLAGS += "-Wl,--as-needed"

# for M_QT_STYLE_EXPORT
DEFINES += M_BUILD_QT_STYLE

TARGET = meegointernalqtstyleplugin
target.path = $$[QT_INSTALL_PLUGINS]/meegotouchstyles

contains( M_BUILD_FEATURES, coverage ) {
    QMAKE_CXXFLAGS += -ftest-coverage -fprofile-arcs
    LIBS += -lgcov

    QMAKE_CLEAN += \
    *.gcda \
    *.gcno
}

exists( $$[QMAKE_MKSPECS]/features/contentmanager.prf ) {
    message("Using the Native File Dialog")
    DEFINES += HAVE_NATIVE_FILEDIALOG
    CONFIG += contentmanager
    CONFIG += qtsparql
} else {
    message("Build-Dependencies for Native File Dialog are missing! " )
    message("Please ensure that this package is installed:" )
    message("libcontentmanager-dev )")
}

PUBLIC_HEADERS += \
  qtmaemo6style.h

PRIVATE_HEADERS += \
  qtmaemo6style_p.h \
  qtmaemo6styleeventfilter.h \
  qtmaemo6scrollbareventfilter.h \
  qtmaemo6sliderpopup.h \
  internalstyleplugin/internalstyleplugin.h \
  qtmaemo6titlebar.h \
  qtmaemo6dialogdimmer.h \
  qtmaemo6clicklabel.h \
  qtmaemo6comboboxpopup.h \
  qtmaemo6applicationwindow.h

SOURCES += \
  qtmaemo6style.cpp \
  qtmaemo6styleeventfilter.cpp \
  qtmaemo6scrollbareventfilter.cpp \
  qtmaemo6sliderpopup.cpp \
  internalstyleplugin/internalstyleplugin.cpp \
  qtmaemo6titlebar.cpp \
  qtmaemo6dialogdimmer.cpp \
  qtmaemo6clicklabel.cpp \
  qtmaemo6comboboxpopup.cpp \
  qtmaemo6applicationwindow.cpp

#scroller module
include(kineticscroller/kineticscroller.pri)

contains( DEFINES, HAVE_NATIVE_FILEDIALOG ) {
    PRIVATE_HEADERS += \
        qtmeegofilepicker.h
    SOURCES += \
        qtmeegofilepicker.cpp \
        qtmeegofilepickerhooks.cpp \
}

RESOURCES += style.qrc

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

install_headers.path  = $$M_INSTALL_HEADERS
install_headers.files = $$PUBLIC_HEADERS

INSTALLS += target \
  install_headers

