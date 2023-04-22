#include "gslibpardir.h"
#include <QTextStream>
#include "widgets/widgetgslibpardir.h"

GSLibParDir::GSLibParDir(const QString name, const QString label, const QString description)
     : GSLibParType(name, label, description)
{
}


void GSLibParDir::save(QTextStream *out)
{
    (*out) << this->_path << '\n';
}

QWidget *GSLibParDir::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParDir();
    ((WidgetGSLibParDir*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParDir::update()
{
    ((WidgetGSLibParDir*)this->_widget)->updateValue( this );
    return true;
}

GSLibParDir *GSLibParDir::clone()
{
    GSLibParDir *new_par = new GSLibParDir("","",_description);
    new_par->_path = _path;
    return new_par;
}
