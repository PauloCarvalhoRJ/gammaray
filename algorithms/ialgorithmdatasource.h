#ifndef IALGORITHMDATASOURCE_H
#define IALGORITHMDATASOURCE_H

#include <list>
#include <string>
#include <iostream>

/** A safe way to test two doubles for equality. */
bool almostEqual2sComplement(double A, double B, int maxUlps);

/** DataValue is a "tagged union". */
class DataValue{
public:
    explicit DataValue( double initContinuousValue ){ value.continuous = initContinuousValue; usedMember = 1; }
    explicit DataValue( int initCategoricalValue ){ value.categorical = initCategoricalValue; usedMember = 2; }
    explicit DataValue(){ value.continuous = 0.0; usedMember = 1; } //default DataValue is a continuous value inited with 0.0
    bool isCategorical(){ return usedMember == 2; }
    int getCategorical(){ return value.categorical; }
    double getContinuous(){ return value.continuous; }
    bool operator==( DataValue other ){
        switch( usedMember ){
        case 1: return almostEqual2sComplement( value.continuous, other.value.continuous, 1);
        case 2: return value.categorical == other.value.categorical;
        default: return false;
        }
    }
    bool operator<( DataValue other ){
        switch( usedMember ){
        case 1: return value.continuous < other.value.continuous;
        case 2: return value.categorical < other.value.categorical;
        default: return false;
        }
    }
    DataValue& operator=( double pValue ){
        usedMember = 1;
        value.continuous = pValue;
        return *this;
    }
    double operator+( double other ){
        switch( usedMember ){
        case 1: return value.continuous + other;
        case 2: return value.categorical + other;
        default: return 0.0; //neutral element of sum
        }
    }
    double operator*( double other ){
        switch( usedMember ){
        case 1: return value.continuous * other;
        case 2: return value.categorical * other;
        default: return 1.0; //neutral element of product
        }
    }
    double operator-( DataValue other ){
        switch( usedMember ){
        case 1: return value.continuous - other.getContinuous();
        case 2: return value.categorical - other.getCategorical();
        default: return 0.0; //neutral element of sum
        }
    }
    double operator/( double other ){
        switch( usedMember ){
        case 1: return value.continuous / other;
        case 2: return value.categorical / other;
        default: return 1.0; //neutral element of product
        }
    }
    double operator*( DataValue other ){
        switch( usedMember ){
        case 1: return value.continuous * other.getContinuous();
        case 2: return value.categorical * other.getCategorical();
        default: return 1.0; //neutral element of product
        }
    }
protected:
    char usedMember;
    union {
        double continuous;
        int categorical;
    } value;
};

/**
 * Set of convert() functions to convert types to QString.  Add more as needed.
 */
namespace converter2string {
    std::string convert( int value );
    std::string convert( long value );
    std::string convert( double value );
    std::string convert( DataValue value );
}

/**
 * Returns a std::string with the list's values in a single text line separated by the given separator character.
 */
template<typename T>
static std::string printStdListContents( const std::list<T>& list, char separator ){
    std::string result;
    typename std::list<T>::const_iterator it = list.cbegin();
    for(; it != list.cend(); ++it){
        result.append( converter2string::convert( *it ) );
        result.push_back( separator );
    }
    return result;
}

/**
  * This macro is to assist in debugging the contents of std::list objects, since those objects don't have an easy way
  * to access individual elements in those containers.
  * The macro creates a std::string variable called prefix+stdlist, where stdlist is the name of a std::list object in the
  * scope where the macro is being called. type is the elements' type (e.g. double) and separator is a char used to
  * separate the values in the text line to be inspected in the debugger.
  */
#define GDBSTDLIST(prefix,stdlist,type,separator) std::string prefix##stdlist = printStdListContents<type>( stdlist, separator );
#define PRINTSTDLIST(stdlist,type,separator) std::cout << "LIST:====>" << printStdListContents<type>( stdlist, separator ) << std::endl;


/** The IAlgorithmDataSource interface must be implemented by any data sources that need to be
 * used in the algorithms framework.  The data source is a table with each variable corresponding to column
 * and each sample correspoinding to a row.
 */
class IAlgorithmDataSource
{
public:
    IAlgorithmDataSource();

    virtual ~IAlgorithmDataSource();

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

    /** Get a data value.
     * @note This method is called very often, this consider performance when overriding this.
     */
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

    /**
     * Returns whether the data source is empty.
     */
    virtual bool isEmpty() const;

};

#endif // IALGORITHMDATASOURCE_H
