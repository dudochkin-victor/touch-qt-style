#ifndef INTERNALSTYLEPLUGIN_H
#define INTERNALSTYLEPLUGIN_H

#include <QObject>

#include "internalstyleinterface.h"

class InternalStylePlugin : public QObject, InternalStyleInterface
{
    Q_OBJECT
    Q_INTERFACES( InternalStyleInterface )

public:
  virtual QStyle* createStyle();
};

#endif
