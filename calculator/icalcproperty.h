#ifndef ICALCPROPERTY_H
#define ICALCPROPERTY_H

#include <QString>
#include <QIcon>

#include "libCalcScriptingDefs.h"

/**
 * The ICalcProperty class represents a variable or attribute, a series of values.
 */
class CALCSCRIPT_LIB_COMMON_DLLSPEC ICalcProperty
{
public:
    ICalcProperty();

	/** Returns the name of the property. */
    virtual QString getCalcPropertyName() = 0;

	/** Returns the icon of the property. */
    virtual QIcon getCalcPropertyIcon() = 0;

	/** Returns the zero-based index in the parent ICalcPropertyCollection object. */
	virtual int getCalcPropertyIndex() = 0;

	/** Returns the same as getCalcPropertyName(), replacing any whitespaces, commas and other
	 * characters that are illegal to be part of script variable names with underscores.
	 */
	QString getScriptCompatibleName();
};

#endif // ICALCPROPERTY_H
