#include "icalcpropertycollection.h"
#include "icalcproperty.h"

ICalcPropertyCollection::ICalcPropertyCollection()
{
}

int ICalcPropertyCollection::getCalcPropertyIndexByScriptCompatibleName(const std::__cxx11::string &name)
{
    int n = getCalcPropertyCount();
    for( int i = 0; i < n; ++i){
        ICalcProperty* prop = getCalcProperty( i );
        if( prop->getScriptCompatibleName() == QString( name.c_str() ))
            return i;
    }
    return -1;
}
