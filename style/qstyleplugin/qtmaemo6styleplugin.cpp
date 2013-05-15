/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (directui@nokia.com)
**
** This file is part of libmeegotouch.
**
** If you have questions regarding the use of this file, please contact
** Nokia at directui@nokia.com.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#include "qtmaemo6styleplugin.h"

#include <QDir>
#include <QPluginLoader>
#include <QDebug>

#include "internalstyleinterface.h"
#include "mlibrarychecker.h"

QStringList QtMaemo6StylePlugin::keys() const
{
    return QStringList() << "meegotouch";
}

QStyle *QtMaemo6StylePlugin::getStylePointer()
{
    QDir pluginsDir( "/usr/lib/qt4/plugins/meegotouchstyles" );

    foreach (const QString& fileName, pluginsDir.entryList(QDir::Files))
    {
        //qWarning() << "trying to load plugin:" << fileName;

        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if ( plugin )
        {
            InternalStyleInterface *interface
                         = qobject_cast<InternalStyleInterface *>(plugin);
            if ( interface )
            {
                return interface->createStyle();
            }
        }
        else
        {
            qWarning() << "no plugin found in" << fileName;
        }
    }

    return 0;
}

QStyle *QtMaemo6StylePlugin::create(const QString &key)
{
    /*
     Q: why do we have two plugins for the style ?
     A: to be able to detect if the style is loaded for
     a qt-only or a meegotouch application.

     Long version:
     We use dl_iterate_phdr to run a query over all
     loaded shared objects (loaded libraries). If
     libmeegotouchcore is in that list, we found that
     the running app is a meegotouch app. Then we do not
     want to load the meegotouch-qt-style. Because the
     style itself needs to link against libmeegotouchcore,
     this detection only works when the style is not loaded
     yet. For this reason we have to put the meegotouch-qt-style
     into an own internal plugin that is then loaded and used
     to create the style for qt.
    */


    QString lkey = key.toLower();

    if (lkey == "meegotouch")
    {
        // here we do not load the meegotouch qt style if
        // the running application links against
        // libmeegotouchcore, i.e. it is a MeeGoTouch
        // application.
        MLibraryChecker libChecker;
        if ( libChecker.linksAgainstMeegoTouch() )
        {
            qWarning( "Not loading meegotouch-qt-style for meegotouch app." );
            return 0;
        }

        qWarning( "Loading meegotouch-qt-style for plain qt app." );

        QStyle *style = getStylePointer();
        if ( ! style )
        {
            qWarning( "meegotouch-qt-style loading failed." );
        }
        else
        {
            qWarning( "meegotouch-qt-style loading success." );
        }

	return style;
    }

    return 0;
}

Q_EXPORT_PLUGIN2(pnp_qtmaemo6styleplugin, QtMaemo6StylePlugin)
