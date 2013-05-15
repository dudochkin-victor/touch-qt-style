include( $$[QT_INSTALL_DATA]/mkspecs/features/meegotouch_common.prf )

target.path = $$M_INSTALL_BIN

# Input
SOURCES += main.cpp \
    mainwindow.cpp
HEADERS += mainwindow.h
RESOURCES += res.qrc

desktop_entry.path = $$M_INSTALL_DATA/applications
desktop_entry.files = \
    qtstyleorientation.desktop

myname = com.nokia.qtstyleorientation

services.CONFIG += no_check_exist
services.target = $${myname}.service
services.commands = cat $$IN_PWD/$${myname}.service.in \
                 | sed -e "s:@M_INSTALL_BIN@:$${M_INSTALL_BIN}:g" \
                 > $${myname}.service
services.files = $$OUT_PWD/$${myname}.service
services.path = $$M_DBUS_SERVICES_DIR

# Install instructions
INSTALLS += target \
    desktop_entry

!win32:!macx {
    INSTALLS += services
}
