#include "ialgorithmdatasource.h"
#include <cassert>
#include <cstdint>

bool almostEqual2sComplement(double A, double B, int maxUlps) {
        // Make sure maxUlps is non-negative and small enough that the
        // default NAN won't compare as equal to anything.
        //<cassert>'s assert doesn't accept longs
        //assert(maxUlps > 0 && maxUlps < 2 * 1024 * 1024 * 1024 * 1024 * 1024);
        assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
        int64_t aLong = *reinterpret_cast<int64_t*>( &A ); //use the raw bytes from the double to make a long int value (type punning)
        // Make aLong lexicographically ordered as a twos-complement long
        if (aLong < 0)
            aLong = 0x8000000000000000 - aLong;
        // Make bLong lexicographically ordered as a twos-complement long
        int64_t bLong = *reinterpret_cast<int64_t*>( &B ); //use the raw bytes from the double to make a long int value (type punning)
        if (bLong < 0)
            bLong = 0x8000000000000000 - bLong;
        int64_t longDiff = (aLong - bLong) & 0x7FFFFFFFFFFFFFFF;
        if (longDiff <= maxUlps)
            return true;
        return false;
}

IAlgorithmDataSource::IAlgorithmDataSource()
{
}

IAlgorithmDataSource::~IAlgorithmDataSource()
{
}

void IAlgorithmDataSource::initZeroes(long rowCount, int columnCount)
{
    clear();
    reserve( rowCount, columnCount );
    for( long iRow = 0; iRow < rowCount; ++iRow )
        for( int iCol = 0; iCol < columnCount; ++iCol)
			setDataValue( iRow, iCol, DataValue(0.0));
}

void IAlgorithmDataSource::setDataFrom(int rowIndexInThisDataSource,
                                       const IAlgorithmDataSource &anotherDataSource,
                                       int rowIndexInAnotherDataSource)
{
    int numberOfColumns = anotherDataSource.getColumnCount();
    for( int iColumn = 0; iColumn < numberOfColumns; ++iColumn ){
        DataValue value = anotherDataSource.getDataValue( rowIndexInAnotherDataSource, iColumn );
        setDataValue( rowIndexInThisDataSource, iColumn, value );
    }
}

bool IAlgorithmDataSource::isEmpty() const
{
    return getRowCount() < 1;
}

