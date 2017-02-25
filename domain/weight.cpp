#include "weight.h"
#include "util.h"

Weight::Weight(QString name, int index_in_file, Attribute *parentAttribute) :
    Attribute( name, index_in_file )
{
    _parent = parentAttribute;
}

QIcon Weight::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/weight16");
    else
        return QIcon(":icons32/weight32");
}
