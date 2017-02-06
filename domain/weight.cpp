#include "weight.h"

Weight::Weight(QString name, int index_in_file, Attribute *parentAttribute) :
    Attribute( name, index_in_file )
{
    _parent = parentAttribute;
}

QIcon Weight::getIcon()
{
    return QIcon(":icons/weight16");
}
