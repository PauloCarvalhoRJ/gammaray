#include "gslibparstring.h"
#include <QTextStream>
#include "widgets/widgetgslibparstring.h"

GSLibParString::GSLibParString(const QString name, const QString label, const QString description) :
    GSLibParType(name, label, description)
{
}


void GSLibParString::save(QTextStream *out)
{
    *out << _value << '\n';
}

QWidget *GSLibParString::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParString();
    ((WidgetGSLibParString*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParString::update()
{
    ((WidgetGSLibParString*)this->_widget)->updateValue( this );
    return true;
}

GSLibParString *GSLibParString::clone()
{
    GSLibParString* new_par = new GSLibParString("","",_value);
    new_par->_value = _value;
    return new_par;
}
