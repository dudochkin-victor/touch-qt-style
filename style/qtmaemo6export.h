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

#ifndef QTMAEMO6EXPORT_H
#define QTMAEMO6EXPORT_H

#include <QtCore/QtGlobal>

#ifdef M_BUILD_QT_STYLE
#  define M_QT_STYLE_EXPORT Q_DECL_EXPORT
#else
#  define M_QT_STYLE_EXPORT Q_DECL_IMPORT
#endif

#if defined(Q_OS_WIN) && defined (Q_CC_MSVC)
# ifndef __func__
#  define __func__ __FUNCTION__
# endif
# ifndef __PRETTY_FUNCTION__
#  define __PRETTY_FUNCTION__ __FUNCTION__
# endif
#endif

#endif // Header guard
