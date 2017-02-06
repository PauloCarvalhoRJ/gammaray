#include "gslibparcolor.h"
#include <QTextStream>
#include "widgets/widgetgslibparcolor.h"

GSLibParColor::GSLibParColor(const QString name, const QString label, const QString description) :
    GSLibParType( name, label, description )
{
}

void GSLibParColor::save(QTextStream *out)
{
    *out << _color_code << '\n';
}

QWidget *GSLibParColor::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParColor();
    ((WidgetGSLibParColor*)this->_widget)->fillFields( this );
    return this->_widget;
}

GSLibParColor *GSLibParColor::clone()
{
    GSLibParColor* new_par = new GSLibParColor("","",_description);
    new_par->_color_code = _color_code;
    return new_par;
}
