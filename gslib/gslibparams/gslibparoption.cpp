#include "gslibparoption.h"
#include <QTextStream>
#include "widgets/widgetgslibparoption.h"

GSLibParOption::GSLibParOption(const QString name, const QString label, const QString description) :
    GSLibParType(name, label, description)
{
}

void GSLibParOption::addOption(const uint value, const QString description)
{
    _options.insert(value, description);
}


void GSLibParOption::save(QTextStream *out)
{
    *out << _selected_value << '\n';
}

QWidget *GSLibParOption::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParOption();
    ((WidgetGSLibParOption*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParOption::update()
{
    ((WidgetGSLibParOption*)this->_widget)->updateValue( this );
    return true;
}

GSLibParOption *GSLibParOption::clone()
{
    GSLibParOption *result = new GSLibParOption("","","");
    result->_selected_value = this->_selected_value;
    result->_options = QMap<uint, QString>( this->_options );
    return result;
}
