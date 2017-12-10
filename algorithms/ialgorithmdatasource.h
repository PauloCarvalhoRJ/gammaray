#ifndef IALGORITHMDATASOURCE_H
#define IALGORITHMDATASOURCE_H

/** The IAlgorithmDataSource interface must be implemented by any data sources that need to be
 * used in the algorithms framework.
 */
class IAlgorithmDataSource
{
public:
    IAlgorithmDataSource();

    //============ Pure virtual methods (interface contract)===================
    /** Returs the sample count. */
    virtual long getSampleCount() const = 0;

    /** Get the number of variables in the data set. */
    virtual int getVariableCount() const = 0;

    /** Empties the data set. */
    virtual void clear() = 0;

    /** Allocates the elements of the data set. */
    virtual void reserve( long sampleCount, int variableCount ) = 0;

    /** Set a data value. */
    virtual void setData( int variableIndex, long sampleIndex, double value ) = 0;

    /** Get a data value. */
    virtual double getData( int variableIndex, long sampleIndex ) const = 0;

    //============ Concrete methods =============================================
    /** Allocates and initializes the data set with zeroes. */
    virtual void initZeroes( long sampleCount, int variableCount );

    /** Set data values from a sample in another data source.
     *  This method assumes both data sources have the same number of variables.
     */
    virtual void setDataFrom( int sampleIndexInThisDataSource,
                              const IAlgorithmDataSource& anotherDataSource,
                              int sampleIndexInAnotherDataSource );
};

#endif // IALGORITHMDATASOURCE_H
