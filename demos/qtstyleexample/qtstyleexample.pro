include( $$[QT_INSTALL_DATA]/mkspecs/features/meegotouch_common.prf )

target.path = $$M_INSTALL_BIN

# Input
SOURCES += main.cpp \
    mainwindow.cpp
HEADERS += mainwindow.h
FORMS += mainwindow.ui

RESOURCES += res.qrc

# UI style definition
style_sheet.path = $$M_THEME_DIR/../qtstyleexample/themes/style
style_sheet.files = style/qtstyleexample.css

view_configuration.path = $$M_THEME_DIR/../qtstyleexample/themes
view_configuration.files = style/qtstyleexample.conf

style_images.path = $$M_THEME_DIR/../qtstyleexample/themes/images
style_images.files = \
    images/*.png \
    images/*.jpg

style_svg.path = $$M_THEME_DIR/../qtstyleexample/themes/svg
style_svg.files = images/*.svg

desktop_entry.path = $$M_INSTALL_DATA/applications
desktop_entry.files = \
    qtstyleexample.desktop \
    qtstyleexample-windows-style.desktop

myname = com.nokia.qtstyleexample

services.CONFIG += no_check_exist
services.target = $${myname}.service
services.commands = cat $$IN_PWD/$${myname}.service.in \
                 | sed -e "s:@M_INSTALL_BIN@:$${M_INSTALL_BIN}:g" \
                 > $${myname}.service
services.files = $$OUT_PWD/$${myname}.service
services.path = $$M_DBUS_SERVICES_DIR

# Install instructions
INSTALLS += target \
    style_sheet \
    style_images \
    style_svg \
    desktop_entry

!win32:!macx {
    INSTALLS += services
}
