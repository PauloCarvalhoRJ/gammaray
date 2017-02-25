#include "normalvariable.h"
#include "util.h"

NormalVariable::NormalVariable(QString name, int index_in_file, Attribute *parentAttribute) :
    Attribute( name, index_in_file )
{
    _parent = parentAttribute;
}

QIcon NormalVariable::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/nscore16");
    else
        return QIcon(":icons32/nscore32");
}
