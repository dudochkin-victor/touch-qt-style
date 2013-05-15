#ifndef INTERNALSTYLEINTERFACE_H
#define INTERNALSTYLEINTERFACE_H

#include <QtPlugin>

class QStyle;

class InternalStyleInterface
{
  public:
    virtual ~InternalStyleInterface() {}
    virtual QStyle* createStyle() = 0;
};

Q_DECLARE_INTERFACE( InternalStyleInterface,
		     "com.meego.Plugin.InternalStyleInterface/1.0");

#endif
