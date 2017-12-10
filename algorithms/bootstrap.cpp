#include "bootstrap.h"
#include "ialgorithmdatasource.h"


template <class Generator>
long useRand(Generator & generator, long min, long max){
    std::uniform_int_distribution<long> distribution(min, max);
    return distribution(generator);
}

Bootstrap::Bootstrap(const IAlgorithmDataSource &input, ResamplingType resType, long randomNumberGeneratorSeed) :
    m_input( input ),
    m_resType( resType ),
    m_seed( randomNumberGeneratorSeed )
{
    m_randomNumberGenerator.seed( randomNumberGeneratorSeed );
}

void Bootstrap::resample(IAlgorithmDataSource &result)
{
    long sampleCount = m_input.getSampleCount();

    //intialize the output.
    result.initZeroes( m_input.getSampleCount(), m_input.getVariableCount() );

    if( m_resType == ResamplingType::CASE ){
        //the output must have the same number of samples of the input
        for( long sampleNumberOfOutput = 0; sampleNumberOfOutput < sampleCount; ++sampleNumberOfOutput){
            //get a random input sample number
            long sampleNumberOfInput = useRand( m_randomNumberGenerator, 0, sampleCount-1);
            //assign its values to the output
            result.setDataFrom( sampleNumberOfOutput, m_input, sampleNumberOfInput );
        }
    }
    //TODO: add suport for the other resampling types here.
}
