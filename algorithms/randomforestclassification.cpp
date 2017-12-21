#include "randomforestclassification.h"
#include "CART/cart.h"

RandomForestClassification::RandomForestClassification(const IAlgorithmDataSource &trainingData,
                                                       const IAlgorithmDataSource &outputData,
                                                       unsigned int B) :
    m_trainingData( trainingData ),
    m_outputData( outputData ),
    m_B( B )
{
}
