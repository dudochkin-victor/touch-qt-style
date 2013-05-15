
INCLUDEPATH += kineticscroller

#QT += webkit

DEFINES += NOWEBKIT

HEADERS += kineticscroller/qtflickgesture_p.h \
           kineticscroller/qtscroller.h \
           kineticscroller/qtscroller_p.h \
           kineticscroller/qtscrollerfilter_p.h \
           kineticscroller/qtscrollerproperties.h \
           kineticscroller/qtscrollerproperties_p.h \
           kineticscroller/qtscrollevent.h \
           kineticscroller/qtscrollevent_p.h
SOURCES += kineticscroller/qtflickgesture.cpp \
           kineticscroller/qtscroller.cpp \
           kineticscroller/qtscrollerfilter.cpp \
           kineticscroller/qtscrollerproperties.cpp \
           kineticscroller/qtscrollevent.cpp
