#include "gslibpardouble.h"
#include <QTextStream>
#include <QLineEdit>
#include "widgets/widgetgslibpardouble.h"

GSLibParDouble::GSLibParDouble(const QString name, const QString label, const QString description) :
    GSLibParType(name, label, description)
{
}

GSLibParDouble::~GSLibParDouble()
{
}


void GSLibParDouble::save(QTextStream *out)
{
    *out << _value << '\n';
}

QWidget *GSLibParDouble::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParDouble();
    ((WidgetGSLibParDouble*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParDouble::update()
{
    ((WidgetGSLibParDouble*)this->_widget)->updateValue( this );
    return true;
}

GSLibParDouble *GSLibParDouble::clone()
{
    GSLibParDouble *result = new GSLibParDouble("", "", "");
    result->_value = this->_value;
    return result;
}
