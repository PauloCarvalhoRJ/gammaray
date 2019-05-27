#include "projectroot.h"


ProjectRoot::ProjectRoot() : ProjectComponent ()
{
}

QString ProjectRoot::getName() const
{
    return "PROJECT ROOT";
}

QIcon ProjectRoot::getIcon()
{
    return QIcon();
}


bool ProjectRoot::isFile() const
{
    return false;
}

bool ProjectRoot::isAttribute()
{
    return false;
}

QString ProjectRoot::getObjectLocator()
{
    return "/" + getName();
}
