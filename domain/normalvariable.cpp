#include "normalvariable.h"

NormalVariable::NormalVariable(QString name, int index_in_file, Attribute *parentAttribute) :
    Attribute( name, index_in_file )
{
    _parent = parentAttribute;
}

QIcon NormalVariable::getIcon()
{
    return QIcon(":icons/nscore16");
}
