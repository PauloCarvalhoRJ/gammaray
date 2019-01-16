#include "gslibparcustomcolor.h"
#include <QTextStream>
#include "gslib/gslibparams/widgets/widgetgslibcustomcolor.h"

GSLibParCustomColor::GSLibParCustomColor(const QString name, const QString label, const QString description) :
    GSLibParType(name, label, description)
{

}

void GSLibParCustomColor::save(QTextStream *out)
{
    *out << _r << "   " << _g << "   " << _b << '\n';
}

QWidget *GSLibParCustomColor::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibCustomColor();
    ((WidgetGSLibCustomColor*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParCustomColor::update()
{
    ((WidgetGSLibCustomColor*)this->_widget)->updateValue( this );
    return true;
}

GSLibParCustomColor *GSLibParCustomColor::clone()
{
    GSLibParCustomColor *new_par = new GSLibParCustomColor("","",_description);
    new_par->_r = _r;
    new_par->_g = _g;
    new_par->_b = _b;
    return new_par;
}
