#include "mcmcdataimputation.h"

#include "domain/segmentset.h"
#include "domain/categorydefinition.h"

MCMCDataImputation::MCMCDataImputation()
{

}

bool MCMCDataImputation::run()
{
    //check whether everything is ok
    if( !isOKtoRun() )
        return false;

    return true;
}

bool MCMCDataImputation::isOKtoRun()
{
    if( ! m_atVariable ){
        m_lastError = "Categorical variable not provided.";
        return false;
    }

    if( ! m_dataSet ){
        m_lastError = "Data set to be imputed not provided.";
        return false;
    } else if( ! m_dataSet->hasNoDataValue() ) {
        m_lastError = "Data set does not have a No-Data value configured.";
        return false;
    }

    {
        CategoryDefinition* cdOfVariable = m_dataSet->getCategoryDefinition( m_atVariable );
        if( ! cdOfVariable ){
            m_lastError = "Category definition of input variable not found (nullptr).";
            return false;
        }
        CategoryDefinition* cdOfFTM = m_FTM->getAssociatedCategoryDefinition();
        if( ! cdOfFTM ){
            m_lastError = "Category definition of FTM not found (nullptr).";
            return false;
        }
        if( cdOfFTM != cdOfVariable ){
            m_lastError = "Category definition of input variable must be the same object as that of the FTM.";
            return false;
        }
        if( m_FTM->isMainDiagonalZeroed() ){
            m_lastError = "FTM must have a zeroed main diagonal (no self-transitions) to be compatible with Embedded Markov Chains.";
            return false;
        }
    }

    {
        CategoryDefinition* cdOfPrimData = m_dataSet->getCategoryDefinition( m_atVariable );
        cdOfPrimData->loadQuintuplets();
        int nDistributions = m_distributions.size();
        int nCategories = cdOfPrimData->getCategoryCount();
        if( nDistributions != nCategories ){
            m_lastError = " Number of distributions ( " +
                    QString::number(nDistributions) + " ) differs from the number of categories ( " +
                    QString::number(nCategories) + " ).";
            return false;
        }
        for( UnivariateDistribution* ud : m_distributions ){
            if( ! ud ){
                m_lastError = "Passed null pointer to the collection of distributions.";
                return false;
            }
        }
    }

    return true;
}
