#include "icalcproperty.h"
#include <QString>

ICalcProperty::ICalcProperty()
{
}

QString ICalcProperty::getScriptCompatibleName()
{
	QString compatibleName = getCalcPropertyName();
	compatibleName = compatibleName.replace( ',', '_' );
	compatibleName = compatibleName.replace( ' ', '_' );
	compatibleName = compatibleName.replace( '(', '_' );
	compatibleName = compatibleName.replace( ')', '_' );
    compatibleName = compatibleName.replace( '-', '_' );
    compatibleName = compatibleName.replace( '+', '_' );
    return compatibleName;
}
