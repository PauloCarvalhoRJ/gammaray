#ifndef ICALCPROPERTYCOLLECTION_H
#define ICALCPROPERTYCOLLECTION_H

#include <QString>

class ICalcProperty;

/**
 * The ICalcPropertyCollection interface should be implemented by classes to be usable in the Calculator.
 */
class ICalcPropertyCollection
{
public:
    ICalcPropertyCollection();
    virtual ~ICalcPropertyCollection(){}

    virtual QString getCalcPropertyCollectionName() = 0;
    virtual int getCalcPropertyCount() = 0;
    virtual ICalcProperty* getCalcProperty( int index ) = 0;
};

#endif // ICALCPROPERTYCOLLECTION_H
