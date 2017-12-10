#include "ialgorithmdatasource.h"

IAlgorithmDataSource::IAlgorithmDataSource()
{
}

void IAlgorithmDataSource::initZeroes(long sampleCount, int variableCount)
{
    clear();
    reserve( sampleCount, variableCount );
    for( long iSample = 0; iSample < sampleCount; ++iSample )
        for( int iVar = 0; iVar < variableCount; ++iVar)
            setData( iVar, iSample, 0.0d);
}

void IAlgorithmDataSource::setDataFrom(int sampleIndexInThisDataSource,
                                       const IAlgorithmDataSource &anotherDataSource,
                                       int sampleIndexInAnotherDataSource)
{
    int numberOfVariables = anotherDataSource.getVariableCount();
    for( int iVar = 0; iVar < numberOfVariables; ++iVar ){
        double value = anotherDataSource.getData( iVar, sampleIndexInAnotherDataSource );
        setData( iVar, sampleIndexInThisDataSource, value );
    }
}
