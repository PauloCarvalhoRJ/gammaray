#include "section.h"

Section::Section(QString path) : File(path)
{

}

QIcon Section::getIcon()
{
    return QIcon(":icons32/section32");
}

QString Section::getObjectLocator()
{
    return _parent->getObjectLocator() + '/' + getName();
}
