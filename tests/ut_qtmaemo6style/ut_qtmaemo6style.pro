QT += testlib
QT += webkit

CONFIG += meegotouch

INCLUDEPATH += $${M_SOURCE_TREE}/style
INCLUDEPATH += $${M_SOURCE_TREE}/style/kineticscroller

SOURCES = \
    ut_qtmaemo6style.cpp \
    $${M_SOURCE_TREE}/style/qtmaemo6style.cpp \
    $${M_SOURCE_TREE}/style/qtmaemo6styleeventfilter.cpp \
    $${M_SOURCE_TREE}/style/qtmaemo6scrollbareventfilter.cpp \
    $${M_SOURCE_TREE}/style/qtmaemo6sliderpopup.cpp \
    $${M_SOURCE_TREE}/style/qtmaemo6titlebar.cpp \
    $${M_SOURCE_TREE}/style/qtmaemo6dialogdimmer.cpp \
    $${M_SOURCE_TREE}/style/qtmaemo6clicklabel.cpp \
    $${M_SOURCE_TREE}/style/qtmaemo6applicationwindow.cpp \
    $${M_SOURCE_TREE}/style/qtmaemo6comboboxpopup.cpp \
    $${M_SOURCE_TREE}/style/kineticscroller/qtflickgesture.cpp \
    $${M_SOURCE_TREE}/style/kineticscroller/qtscroller.cpp \
    $${M_SOURCE_TREE}/style/kineticscroller/qtscrollerfilter.cpp \
    $${M_SOURCE_TREE}/style/kineticscroller/qtscrollerproperties.cpp \
    $${M_SOURCE_TREE}/style/kineticscroller/qtscrollevent.cpp

HEADERS += \
    ut_qtmaemo6style.h \
    $${M_SOURCE_TREE}/style/qtmaemo6style.h \
    $${M_SOURCE_TREE}/style/qtmaemo6style_p.h \
    $${M_SOURCE_TREE}/style/qtmaemo6styleeventfilter.h \
    $${M_SOURCE_TREE}/style/qtmaemo6scrollbareventfilter.h \
    $${M_SOURCE_TREE}/style/qtmaemo6sliderpopup.h \
    $${M_SOURCE_TREE}/style/qtmaemo6titlebar.h \
    $${M_SOURCE_TREE}/style/qtmaemo6dialogdimmer.h \
    $${M_SOURCE_TREE}/style/qtmaemo6clicklabel.h \
    $${M_SOURCE_TREE}/style/qtmaemo6applicationwindow.h \
    $${M_SOURCE_TREE}/style/qtmaemo6comboboxpopup.h \
    $${M_SOURCE_TREE}/style/qtmaemo6applicationwindow.h \
    $${M_SOURCE_TREE}/style/kineticscroller/qtflickgesture_p.h \
    $${M_SOURCE_TREE}/style/kineticscroller/qtscroller.h \
    $${M_SOURCE_TREE}/style/kineticscroller/qtscroller_p.h \
    $${M_SOURCE_TREE}/style/kineticscroller/qtscrollerfilter_p.h \
    $${M_SOURCE_TREE}/style/kineticscroller/qtscrollerproperties.h \
    $${M_SOURCE_TREE}/style/kineticscroller/qtscrollerproperties_p.h \
    $${M_SOURCE_TREE}/style/kineticscroller/qtscrollevent.h \
    $${M_SOURCE_TREE}/style/kineticscroller/qtscrollevent_p.h


target.path = $$[QT_INSTALL_LIBS]/libmeegotouch-tests
INSTALLS += target
