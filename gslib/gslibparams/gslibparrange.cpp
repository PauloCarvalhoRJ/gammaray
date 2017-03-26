#include "gslibparrange.h"
#include <QTextStream>
#include "widgets/widgetgslibparrange.h"

GSLibParRange::GSLibParRange(const QString name, const QString label, const QString description) :
    GSLibParType(name, label, description)
{
}


void GSLibParRange::save(QTextStream *out)
{
    *out << QString::number(_value, 'g', 12) << '\n';
}

QWidget *GSLibParRange::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParRange();
    ((WidgetGSLibParRange*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParRange::update()
{
    ((WidgetGSLibParRange*)this->_widget)->updateValue( this );
    return true;
}
