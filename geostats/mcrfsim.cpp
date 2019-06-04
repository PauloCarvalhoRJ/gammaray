#include "mcrfsim.h"

#include "gslib/gslibparameterfiles/commonsimulationparameters.h"
#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "domain/categorypdf.h"
#include "domain/verticaltransiogrammodel.h"
#include "domain/categorydefinition.h"
#include "domain/application.h"
#include "spectral/spectral.h"

#include <thread>
#include <random>

MCRFSim::MCRFSim() :
    m_atPrimary( nullptr),
    m_cgSim( nullptr ),
    m_pdf( nullptr ),
    m_transiogramModel( nullptr ),
    m_lateralGradationType( LateralGradationType::TAIL_TRANSIOGRAMS_ONLY ),
    m_gradationField( nullptr ),
    m_LVAazimuth( nullptr ),
    m_LVAsemiMajorAxis( nullptr ),
    m_LVAsemiMinorAxis( nullptr ),
    m_probFields( std::vector< Attribute*>() ),
    m_tauFactorForGlobalPDF( 1.0 ),
    m_tauFactorForTransiography( 1.0 ),
    m_tauFactorForProbabilityFields( 1.0 ),
    m_commonSimulationParameters( nullptr )
{ }

bool MCRFSim::isOKtoRun()
{
    if( ! m_atPrimary ){
        m_lastError = "Categorical variable not provided.";
        return false;
    }

    if( ! m_cgSim ){
        m_lastError = "Simulation grid not provided.";
        return false;
    } else if( ! m_cgSim->hasNoDataValue() ) {
        m_lastError = "Simulation grid does not have a No-Data value configured.";
        return false;
    }

    if( ! m_pdf ){
        m_lastError = "Global PDF not provided.";
        return false;
    } else {
        DataFile* dfPrimary = dynamic_cast<DataFile*>( m_atPrimary->getContainingFile() );
        if( ! dfPrimary ){
            m_lastError = "The file of input categorical variable is not a DataFile object.";
            return false;
        }
        CategoryDefinition* cdOfPrimData = dfPrimary->getCategoryDefinition( m_atPrimary );
        if( ! cdOfPrimData ){
            m_lastError = "Category definition of input variable not found (nullptr).";
            return false;
        }
        CategoryDefinition* cdOfPDF = m_pdf->getCategoryDefinition();
        if( ! cdOfPDF ){
            m_lastError = "Category definition of PDF not found (nullptr).";
            return false;
        }
        if( cdOfPDF != cdOfPrimData ){
            m_lastError = "Category definition of input variable must be the same object as that the PDF is based on.";
            return false;
        }
    }

    if( ! m_transiogramModel ){
        m_lastError = "Vertical transiogram model not provided.";
        return false;
    } else {
        DataFile* dfPrimary = dynamic_cast<DataFile*>( m_atPrimary->getContainingFile() );
        CategoryDefinition* cdOfPrimData = dfPrimary->getCategoryDefinition( m_atPrimary );
        CategoryDefinition* cdOfTransiogramModel = m_transiogramModel->getCategoryDefinition();
        if( ! cdOfTransiogramModel ){
            m_lastError = "Category definition of vertical transiogram model not found (nullptr).";
            return false;
        }
        if( cdOfTransiogramModel != cdOfPrimData ){
            m_lastError = "Category definition of input variable must be the same object as that the vertical transiogram model is based on.";
            return false;
        }
    }

    if( m_lateralGradationType == LateralGradationType::USE_GRADATIONAL_FIELD && ! m_gradationField ){
        m_lastError = "Use of a gradation field was selected for lateral transiography ranging, but none was provided.";
        return false;
    }

    if( ( m_lateralGradationType == LateralGradationType::TAIL_TRANSIOGRAMS_ONLY ||
          m_lateralGradationType == LateralGradationType::HEAD_TRANSIOGRAMS_ONLY ||
          m_lateralGradationType == LateralGradationType::HEAD_AND_TAIL_TRANSIOGRAMS_AT_RANDOM )
           && ( ! m_LVAazimuth || ! m_LVAsemiMajorAxis || ! m_LVAsemiMinorAxis ) ){
        m_lastError = "Use of lateral transiogram ranges was selected, which requires three additional fields"
                      " in the simulation grid: azimuth, semi-major axis and semi-minor axis.";
        return false;
    }

    if( useSecondaryData() ){
        DataFile* dfPrimary = dynamic_cast<DataFile*>( m_atPrimary->getContainingFile() );
        CategoryDefinition* cdOfPrimData = dfPrimary->getCategoryDefinition( m_atPrimary );
        int nProbFields = m_probFields.size();
        int nCategories = cdOfPrimData->getCategoryCount();
        if( nProbFields != nCategories ){
            m_lastError = " Number of probability fields ( " +
                    QString::number(nProbFields) + " ) differs from the number of categories ( " +
                    QString::number(nCategories) + " ).";
            return false;
        }
    }

    if( ! m_commonSimulationParameters ){
        m_lastError = "A common simulation parameter object was not provided.  This object contains non-Markov-specific parameters such"
                      " as neighborhood parameters, random number generator seed, number of realization, etc.";
        return false;
    } else {
        if( m_commonSimulationParameters->getNumberOfRealizations() > 99 || m_commonSimulationParameters->getNumberOfRealizations() < 1 ){
            m_lastError = "Number of realizations must be between 1 and 99.";
            return false;
        }
    }

    return true;
}

bool MCRFSim::useSecondaryData()
{
    return ! m_probFields.empty();
}

/** The code to simulate some realizations per thread *////////////////////////
void simulateSomeRealizations( uint nRealsForOneThread,
                               const CartesianGrid* cgSim,
                               uint seed,
                               std::vector< spectral::arrayPtr >* realizationsOutput ){

    //initialize the local random number generator with the seed reserved for this thread
    std::mt19937 randomNumberGenerator;
    randomNumberGenerator.seed( seed );

    //get simulation grid dimensions
    uint nI = cgSim->getNI();
    uint nJ = cgSim->getNJ();
    uint nK = cgSim->getNK();

    //for each realization for this thread
    for( uint iRealization = 0; iRealization < nRealsForOneThread; ++iRealization ){
        //fill it with the sim grid's NDV
        spectral::array simulatedData( nI, nJ, nK, cgSim->getNoDataValueAsDouble() );
        //traverse the grid's cells
        for( uint k = 0; k < nK; ++k ) //for each z-slice
            for( uint j = 0; j < nJ; ++j ) //for each cross line
                for( uint i = 0; i < nI; ++i ) { //for each in-line
                }
       //realizationsOutput->push_back( simulatedData );
    }
}
///////////////////////////////////////////////////////////////////////////////


bool MCRFSim::run()
{
    //check whether everything is ok
    if( !isOKtoRun() )
        return false;

    //get the number of realizations the user wants to simulate
    uint nRealizations = m_commonSimulationParameters->getNumberOfRealizations();

    //get the number of threads from logical CPUs or number of realizations (whichever is the lowest)
    unsigned int nThreads = std::min( std::thread::hardware_concurrency(), nRealizations );

    Application::instance()->logInfo("Commencing MCRF simulation with " + QString::number(nRealizations) + " thread(s).");

    //distribute the realizations among the n-threads
    uint numberOfRealizationsForAThread[nThreads];
    for( uint iThread = 0; iThread < nRealizations; ++iThread )
        numberOfRealizationsForAThread[ iThread ] = 0; //initialize the numbers of realizations with zeros
    for( uint iReal = 0; iReal < nRealizations; ++iReal )
        ++numberOfRealizationsForAThread[ iReal % nThreads ];

    //create vectors of data arrays for each thread, so they deposit their realizations in them.
    std::vector< spectral::arrayPtr > realizationDepots[nThreads];

    //create and run the simulation threads
    std::thread threads[nThreads];
    for( unsigned int iThread = 0; iThread < nThreads; ++iThread){
        std::vector< spectral::arrayPtr >& realizationDepot = realizationDepots[iThread];
        threads[iThread] = std::thread( simulateSomeRealizations,
                                        numberOfRealizationsForAThread[ iThread ],
                                        m_cgSim,
                                        m_commonSimulationParameters->getSeed() * 100 + iThread, //number of realizations is capped at 99, so even if there are more than 99 logical processors, number of threads will be limited to 99
                                        &realizationDepot
                                        );
    }

    //wait for the threads to finish.
    for( unsigned int iThread = 0; iThread < nThreads; ++iThread)
        threads[iThread].join();

    //collect the realizations created by the threads:
    for( unsigned int iThread = 0; iThread < nThreads; ++iThread ){
        std::vector< spectral::arrayPtr >::iterator it = realizationDepots[ iThread ].begin();
        for(; it != realizationDepots[ iThread ].end(); ++it)
            /*m_realizations.push_back( *it )*/;
    }

    Application::instance()->logInfo("MCRF completed.");
    return true;
}
