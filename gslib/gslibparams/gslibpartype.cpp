#include "gslibpartype.h"
#include <QLayout>
#include <QWidget>

GSLibParType::GSLibParType(const QString name, const QString label, const QString description) : _widget(nullptr)
{
    this->_name = name;
    this->_label = label;
    this->_description = description;
}

GSLibParType::~GSLibParType()
{
    if( _widget )
        delete _widget;
}

bool GSLibParType::isNamed(const QString name)
{
    return this->_name == name;
}

QWidget *GSLibParType::getWidget()
{
    return this->_widget;
}

bool GSLibParType::update()
{
    return false;
}

bool GSLibParType::isRepeat()
{
    return false;
}

GSLibParType *GSLibParType::clone()
{
    return nullptr;
}

void GSLibParType::detachFromGUI(QLayout *parentLayout)
{
    if( _widget ){
        parentLayout->removeWidget( this->_widget );
        this->_widget->setParent( nullptr );
    }
}
