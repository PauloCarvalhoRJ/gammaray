#include "objectgroup.h"
#include <QTextStream>

ObjectGroup::ObjectGroup(const QString name, const QIcon icon, const QString internal_name)
{
    this->_name = name;
    this->_icon = icon;
    this->_internal_name = internal_name;
}

QString ObjectGroup::getName() const
{
    return this->_name;
}

QIcon ObjectGroup::getIcon()
{
    return this->_icon;
}

void ObjectGroup::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->_internal_name << '\n';
    for (std::vector<ProjectComponent*>::iterator it = this->_children.begin(); it != this->_children.end(); ++it)
      (*it)->save( txt_stream );
}


bool ObjectGroup::isFile() const
{
    return false;
}


bool ObjectGroup::isAttribute()
{
    return false;
}

QString ObjectGroup::getObjectLocator()
{
    return _parent->getObjectLocator() + '/' + getName();
}
