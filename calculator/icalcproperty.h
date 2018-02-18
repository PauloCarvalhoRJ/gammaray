#ifndef ICALCPROPERTY_H
#define ICALCPROPERTY_H

#include <QString>
#include <QIcon>

/**
 * The ICalcProperty class represents a variable or attribute, a series of values.
 */
class ICalcProperty
{
public:
    ICalcProperty();

    virtual QString getCalcPropertyName() = 0;
    virtual QIcon getCalcPropertyIcon() = 0;
};

#endif // ICALCPROPERTY_H
