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

std::list<DataValue> IAlgorithmDataSource::getUniqueDataValues(int columnIndex) const
{
    long rowCount = getRowCount();
    std::list<DataValue> result;
    for( long iRow = 0; iRow < rowCount; ++iRow )
        result.push_back( getDataValue( iRow, columnIndex ) );
    result.sort();
    result.unique();
    return result;
}

void IAlgorithmDataSource::getCategoriesCounts(std::list< std::pair<DataValue, long> > &result,
                                               int columnIndex) const
{
    long rowCount = getRowCount();
    std::list< std::pair<DataValue, long> >::iterator it = result.begin();
    //for each category
    for(; it != result.end(); ++it){
        std::pair<DataValue, long>& pair = *it;
        //for each row
        for( long iRow = 0; iRow < rowCount; ++iRow ){
            if( getDataValue( iRow, columnIndex ) == pair.first ){
                pair.second++;
            }
        }
    }
}
