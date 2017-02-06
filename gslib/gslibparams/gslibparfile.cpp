#include "gslibparfile.h"
#include <QTextStream>
#include "widgets/widgetgslibparfile.h"

GSLibParFile::GSLibParFile(const QString name, const QString label, const QString description)
     : GSLibParType(name, label, description)
{
}


void GSLibParFile::save(QTextStream *out)
{
    (*out) << this->_path << '\n';
}

QWidget *GSLibParFile::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParFile();
    ((WidgetGSLibParFile*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParFile::update()
{
    ((WidgetGSLibParFile*)this->_widget)->updateValue( this );
    return true;
}

GSLibParFile *GSLibParFile::clone()
{
    GSLibParFile *new_par = new GSLibParFile("","",_description);
    new_par->_path = _path;
    return new_par;
}
