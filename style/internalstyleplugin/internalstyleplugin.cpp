#include "internalstyleplugin.h"

#include "qtmaemo6style.h"

QStyle* InternalStylePlugin::createStyle()
{
    return new QtMaemo6Style();
}

Q_EXPORT_PLUGIN2( internalstyleplugin, InternalStylePlugin );
