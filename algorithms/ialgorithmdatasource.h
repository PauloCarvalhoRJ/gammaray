#ifndef IALGORITHMDATASOURCE_H
#define IALGORITHMDATASOURCE_H

#include <list>


/** DataValue is a "tagged union". */
class DataValue{
public:
    explicit DataValue( double initContinuousValue ){ value = initContinuousValue; usedMember = 1; }
    explicit DataValue( int initCategoricalValue ){ value = initCategoricalValue; usedMember = 2; }
    bool isCategorical(){ return usedMember == 2; }
    int usedMember;
    union valueUnion {
        double continuous;
        int categorical;
        bool operator==( valueUnion other ){
            return *this == other;  //TODO: not safe with doubles.
        }
        bool operator<( valueUnion other ){
            return *this < other;
        }
        valueUnion operator=( double doubleValue ){
            return *this = doubleValue;
        }
    } value;
    bool operator==( DataValue other ){
        return value == other.value;
    }
    bool operator<( DataValue other ){
        return value < other.value;
    }
};

/** The IAlgorithmDataSource interface must be implemented by any data sources that need to be
 * used in the algorithms framework.  The data source is a table with each variable corresponding to column
 * and each sample correspoinding to a row.
 */
class IAlgorithmDataSource
{
public:
    IAlgorithmDataSource();

    //============ Pure virtual methods (interface contract)===================
    /** Returns the sample count. */
    virtual long getRowCount() const = 0;

    /** Returns the number of variables in the data set. */
    virtual int getColumnCount() const = 0;

    /** Empties the data set. */
    virtual void clear() = 0;

    /** Allocates the elements of the data set. */
    virtual void reserve( long rowCount, int columnCount ) = 0;

    /** Set a data value. */
    virtual void setDataValue( long rowIndex, int columnIndex, DataValue value ) = 0;

    /** Get a data value. */
    virtual DataValue getDataValue( long rowIndex, int columnIndex  ) const = 0;

    //============ Concrete methods =============================================
    /** Allocates and initializes the data set with zeroes (doubles). */
    virtual void initZeroes( long rowCount, int columnCount );

    /** Set data values from a sample in another data source.
     *  This method assumes both data sources have the same number of variables.
     */
    virtual void setDataFrom( int rowIndexInThisDataSource,
                              const IAlgorithmDataSource& anotherDataSource,
                              int rowIndexInAnotherDataSource );

};

#endif // IALGORITHMDATASOURCE_H
