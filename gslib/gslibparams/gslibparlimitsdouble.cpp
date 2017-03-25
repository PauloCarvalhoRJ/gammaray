#include "gslibparlimitsdouble.h"
#include <QTextStream>
#include "widgets/widgetgslibparlimitsdouble.h"

GSLibParLimitsDouble::GSLibParLimitsDouble(const QString name, const QString label, const QString description)
    : GSLibParType(name, label, description)
{
}


void GSLibParLimitsDouble::save(QTextStream *out)
{
    (*out) << QString::number(_min, 'g', 12) << "    " << QString::number(_max, 'g', 12) << '\n';
}

QWidget *GSLibParLimitsDouble::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParLimitsDouble();
    ((WidgetGSLibParLimitsDouble*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParLimitsDouble::update()
{
    ((WidgetGSLibParLimitsDouble*)this->_widget)->updateValue( this );
    return true;
}
