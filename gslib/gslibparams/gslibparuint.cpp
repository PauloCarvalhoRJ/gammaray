#include "gslibparuint.h"
#include <QTextStream>
#include "widgets/widgetgslibparuint.h"

GSLibParUInt::GSLibParUInt(const QString name, const QString label, const QString description) :
    GSLibParType( name, label, description )
{
}

GSLibParUInt::GSLibParUInt(uint initValue) :
    GSLibParType("", "", ""),
    _value( initValue )
{
}

void GSLibParUInt::save(QTextStream *out)
{
    *out << _value << '\n';
}

QWidget *GSLibParUInt::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParUInt();
    ((WidgetGSLibParUInt*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParUInt::update()
{
    ((WidgetGSLibParUInt*)this->_widget)->updateValue( this );
    return true;
}

GSLibParUInt *GSLibParUInt::clone()
{
    GSLibParUInt *new_par = new GSLibParUInt("","",_description);
    new_par->_value = _value;
    return new_par;
}
