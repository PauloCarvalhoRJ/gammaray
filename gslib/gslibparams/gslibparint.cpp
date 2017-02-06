#include "gslibparint.h"
#include <QTextStream>
#include "widgets/widgetgslibparint.h"

GSLibParInt::GSLibParInt(const QString name, const QString label, const QString description) :
    GSLibParType(name, label, description)
{
}


void GSLibParInt::save(QTextStream *out)
{
    *out << _value << '\n';
}

QWidget *GSLibParInt::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParInt();
    ((WidgetGSLibParInt*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParInt::update()
{
    ((WidgetGSLibParInt*)this->_widget)->updateValue( this );
    return true;
}

GSLibParInt *GSLibParInt::clone()
{
    GSLibParInt *new_par = new GSLibParInt("","",_description);
    new_par->_value = _value;
    return new_par;
}
