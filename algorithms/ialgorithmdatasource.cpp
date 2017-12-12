#include "ialgorithmdatasource.h"

IAlgorithmDataSource::IAlgorithmDataSource()
{
}

void IAlgorithmDataSource::initZeroes(long rowCount, int columnCount)
{
    clear();
    reserve( rowCount, columnCount );
    for( long iRow = 0; iRow < rowCount; ++iRow )
        for( int iCol = 0; iCol < columnCount; ++iCol)
            setDataValue( iCol, iRow, DataValue(0.0d));
}

void IAlgorithmDataSource::setDataFrom(int rowIndexInThisDataSource,
                                       const IAlgorithmDataSource &anotherDataSource,
                                       int rowIndexInAnotherDataSource)
{
    int numberOfColumns = anotherDataSource.getColumnCount();
    for( int iColumn = 0; iColumn < numberOfColumns; ++iColumn ){
        DataValue value = anotherDataSource.getDataValue( iColumn, rowIndexInAnotherDataSource );
        setDataValue( iColumn, rowIndexInThisDataSource, value );
    }
}

