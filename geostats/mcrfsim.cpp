#include "mcrfsim.h"

#include "gslib/gslibparameterfiles/commonsimulationparameters.h"
#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "domain/categorypdf.h"
#include "domain/verticaltransiogrammodel.h"
#include "domain/categorydefinition.h"
#include "domain/application.h"
#include "domain/pointset.h"
#include "domain/segmentset.h"
#include "geostats/searchneighborhood.h"
#include "geostats/searchellipsoid.h"
#include "geostats/pointsetcell.h"
#include "geostats/segmentsetcell.h"
#include "geostats/pointsetcell.h"
#include "spatialindex/spatialindex.h"
#include "util.h"

#include <thread>
#include <QApplication>
#include <QProgressDialog>

MCRFSim::MCRFSim() :
    //---------simulation parameters----------------
    m_atPrimary( nullptr),
    m_gradationFieldOfPrimaryData( nullptr ),
    m_cgSim( nullptr ),
    m_pdf( nullptr ),
    m_transiogramModel( nullptr ),
    m_gradationFieldOfSimGrid( nullptr ),
    m_probFields( std::vector< Attribute*>() ),
    m_tauFactorForTransiography( 1.0 ),
    m_tauFactorForProbabilityFields( 1.0 ),
    m_commonSimulationParameters( nullptr ),
    m_invertGradationFieldConvention( false ),
    m_maxNumberOfThreads( 1 ),
    //------other member variables--------------------
    m_progressDialog( nullptr ),
    m_spatialIndexOfPrimaryData( new SpatialIndex() ),
    m_spatialIndexOfSimGrid( new SpatialIndex() ),
    m_primaryDataType( PrimaryDataType::UNDEFINED ),
    m_primaryDataFile( nullptr )
{ }

bool MCRFSim::isOKtoRun()
{
    if( ! m_atPrimary ){
        m_lastError = "Categorical variable not provided.";
        return false;
    }

    if( ! m_gradationFieldOfPrimaryData ){
        m_lastError = "Gradation field value in input data not provided.";
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
        m_dfPrimary = dynamic_cast<DataFile*>( m_atPrimary->getContainingFile() );
        if( ! m_dfPrimary ){
            m_lastError = "The file of input categorical variable is not a DataFile object.";
            return false;
        } else {
            m_primaryDataFile = m_dfPrimary;
            if( m_dfPrimary->getFileType() == "POINTSET" )
                m_primaryDataType = PrimaryDataType::POINTSET;
            if( m_dfPrimary->getFileType() == "CARTESIANGRID" )
                m_primaryDataType = PrimaryDataType::CARTESIANGRID;
            if( m_dfPrimary->getFileType() == "GEOGRID" )
                m_primaryDataType = PrimaryDataType::GEOGRID;
            if( m_dfPrimary->getFileType() == "SEGMENTSET" )
                m_primaryDataType = PrimaryDataType::SEGMENTSET;
        }
        CategoryDefinition* cdOfPrimData = m_dfPrimary->getCategoryDefinition( m_atPrimary );
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
        CategoryDefinition* cdOfPrimData = m_dfPrimary->getCategoryDefinition( m_atPrimary );
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

    {
        CategoryDefinition* cdOfPDF = m_pdf->getCategoryDefinition();
        CategoryDefinition* cdOfTransiogramModel = m_transiogramModel->getCategoryDefinition();
        if( cdOfPDF != cdOfTransiogramModel ){
            m_lastError = "Category definition of transiogram model must be the same object as that the PDF is based on.";
            return false;
        }
    }

    if( ! m_gradationFieldOfSimGrid ){
        m_lastError = "Use of a gradation field in the simulation grid is required to stablish a correlation"
                      " between vertical (time) and lateral facies succession in 3D Markov Chain.";
        return false;
    }

    if( useSecondaryData() ){
        CategoryDefinition* cdOfPrimData = m_dfPrimary->getCategoryDefinition( m_atPrimary );
        cdOfPrimData->loadQuintuplets();
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

bool MCRFSim::useSecondaryData() const
{
    return ! m_probFields.empty();
}

double MCRFSim::simulateOneCellMT(uint i, uint j, uint k,
                                  std::mt19937 &randomNumberGenerator, const spectral::array& simulatedData ) const
{

    //get the facies set to be simulated
    CategoryDefinition* cd = m_pdf->getCategoryDefinition();

    //define a cell object that represents the current simulation cell
    GridCell simulationCell( m_cgSim, -1, i, j, k );

    //collect samples from the input data set ordered by their distance with respect
    //to the simulation cell.
    DataCellPtrMultiset vSamplesPrimary = getSamplesFromPrimaryMT( simulationCell );

    //collect neighboring simulation grid cells ordered by their distance with respect
    //to the simulation cell.
    DataCellPtrMultiset vNeighboringSimGridCells = getNeighboringSimGridCellsMT( simulationCell );

    //make a local copy of the Tau Model (this is potentially a multi-threaded code)
    TauModel tauModelCopy( *m_tauModel );

    //get relevant information of the simulation cell
    uint simCellLinearIndex           = m_cgSim->IJKtoIndex( i, j, k );
    double simCellZ                   = m_cgSim->getDataSpatialLocation( simCellLinearIndex, CartesianCoord::Z );
    double simCellGradationFieldValue = m_cgSim->dataIJKConst( m_gradationFieldOfSimGrid->getAttributeGEOEASgivenIndex()-1,
                                                               i, j, k );
    //get the probabilities from the global PDF, they're the marginal
    //probabilities for the Tau Model
    for( int categoryIndex = 0; categoryIndex < cd->getCategoryCount(); ++categoryIndex )
        tauModelCopy.setMarginalProbability( categoryIndex, m_pdf->get2ndValue( categoryIndex ) );


    //To compute the facies probabilities for the Monte Carlo draw we only need to collect the codes of the
    //facies found in the search neighborhood along with their distances to the simulation cell.
    //the facies codes and distances are taken from the primary data and the previously simulated cells
    //found in search neighborhood
    typedef double FaciesCodeFrom;
    typedef double SuccessionSeparation;
    std::vector< std::pair< FaciesCodeFrom, SuccessionSeparation > > faciesFromCodesAndSuccessionSeparations;
    faciesFromCodesAndSuccessionSeparations.reserve( m_commonSimulationParameters->getNumberOfSamples() +
                                                     m_commonSimulationParameters->getNumberOfSimulatedNodesForConditioning() );

    ///======================================== PROCESSING OF EACH PRIMARY DATUM  FOUND IN THE SEARCH NEIGHBORHOOD=============================================
    DataCellPtrMultiset::iterator itSampleCells = vSamplesPrimary.begin();
    for( uint i = 0; i < vSamplesPrimary.size(); ++i, ++itSampleCells){
        DataCellPtr sampleDataCell = *itSampleCells;

        //get the facies value (it is a double due to DataFile API, but it is an integer value).
        double sampleFaciesValue = sampleDataCell->readValueFromDataSet( m_atPrimary->getAttributeGEOEASgivenIndex()-1 );

        // Sanity check against No-data-values
        // DataFile::isNDV() is non-const and has a slow string-to-double conversion
        if( ! Util::almostEqual2sComplement( m_simGridNDV, sampleFaciesValue, 1 ) ){

            // get the sample's gradation field value
            double sampleGradationValue = sampleDataCell->readValueFromDataSet( m_gradationFieldOfPrimaryData->getAttributeGEOEASgivenIndex()-1 );

            //To preserve Markovian property, we cannot use data ahead in the facies succession.
            bool isAheadInSuccession = false;
            {
                isAheadInSuccession = isAheadInSuccession || ( sampleDataCell->_center._z > simCellZ ); // a sample location above the current cell is considered ahead (in time)
                //a sampple location ahead in the lateral facies succession should not be computed (Walther's Law)
                isAheadInSuccession = isAheadInSuccession || ( ! m_invertGradationFieldConvention && sampleGradationValue >  simCellGradationFieldValue );
                isAheadInSuccession = isAheadInSuccession || (   m_invertGradationFieldConvention && sampleGradationValue <= simCellGradationFieldValue );
            }

            if( ! isAheadInSuccession ){
                //get the facies code that was simulated in the neighboring cell
                uint faciesCodeInSample = static_cast< uint >( sampleGradationValue );
                //compute the resulting succession distance ( vector resulting from vertical separation and
                // variation in the gradation field - lateral succession separation )
                double faciesSuccessionDistance = 0.0;
                {
                    double verticalSeparation = simCellZ - sampleDataCell->_center._z;
                    double lateralSuccessionSeparation = std::abs( sampleGradationValue - simCellGradationFieldValue );
                    faciesSuccessionDistance = std::sqrt( verticalSeparation*verticalSeparation + lateralSuccessionSeparation*lateralSuccessionSeparation );
                }
                //Finally, collect the facies code and the succession separation for the ensuing transiogram
                //query for the facies transition probability
                faciesFromCodesAndSuccessionSeparations.push_back( { faciesCodeInSample, faciesSuccessionDistance } );
            }

        } else {
            Application::instance()->logWarn("MCRFSim::simulateOneCellMT(): Primary data is not supposed to have no-data values.");
        }
    }

    ///======================================== PROCESSING OF EACH GRID CELL FOUND IN THE SEARCH NEIGHBORHOOD=============================================
    DataCellPtrMultiset::iterator itSimGridCells = vNeighboringSimGridCells.begin();
    for( uint i = 0; i < vNeighboringSimGridCells.size(); ++i, ++itSimGridCells){
        DataCellPtr neighborDataCell = *itSimGridCells;
        //we know the data cell is a grid cell
        const GridCell* neighborGridCellAspect = dynamic_cast<GridCell*>(neighborDataCell.get());

        //get the topological coordinates of the neighnoring cell
        uint neighI = neighborGridCellAspect->_indexIJK._i;
        uint neighJ = neighborGridCellAspect->_indexIJK._j;
        uint neighK = neighborGridCellAspect->_indexIJK._k;

        //get the realization value (a facies code) in the neighboring cell (may be NDV)
        double realizationValue = simulatedData( neighI, neighJ, neighK );

        //if there is a previously simulated data in the neighboring cell
        // DataFile::isNDV() is non-const and has a slow string-to-double conversion
        if( ! Util::almostEqual2sComplement( m_simGridNDV, realizationValue, 1 ) ){

            // get the neighboring cell's gradation field value
            double neighborGradationFieldValue = m_cgSim->dataIJKConst( m_gradationFieldOfSimGrid->getAttributeGEOEASgivenIndex()-1,
                                                                         neighI, neighJ, neighK );

            //To preserve Markovian property, we cannot use data ahead in the facies succession.
            bool isAheadInSuccession = false;
            {
                isAheadInSuccession = isAheadInSuccession || ( neighK > k ); // a grid cell above the current cell is considered ahead (in time)
                //a grid cell ahead in the lateral facies succession should not be computed (Walther's Law)
                isAheadInSuccession = isAheadInSuccession || ( ! m_invertGradationFieldConvention && neighborGradationFieldValue >  simCellGradationFieldValue );
                isAheadInSuccession = isAheadInSuccession || (   m_invertGradationFieldConvention && neighborGradationFieldValue <= simCellGradationFieldValue );
            }

            if( ! isAheadInSuccession ){
                //get the facies code that was simulated in the neighboring cell
                uint faciesCodeInPreviouslySimulatedData = static_cast< uint >( realizationValue );
                //compute the resulting succession distance ( vector resulting from vertical separation and
                // variation in the gradation field - lateral succession separation )
                double faciesSuccessionDistance = 0.0;
                {
                    double verticalSeparation = simCellZ - neighborGridCellAspect->_center._z;
                    double lateralSuccessionSeparation = std::abs( neighborGradationFieldValue - simCellGradationFieldValue );
                    faciesSuccessionDistance = std::sqrt( verticalSeparation*verticalSeparation + lateralSuccessionSeparation*lateralSuccessionSeparation );
                }

                //Finally, collect the facies code and the succession separation for the ensuing transiogram
                //query for the facies transition probability
                faciesFromCodesAndSuccessionSeparations.push_back( { faciesCodeInPreviouslySimulatedData, faciesSuccessionDistance } );
            }
        }
    }


    //////////////// COMPUTE THE PROBABILITIES OF THIS SIMULATION CELL BEING EACH CANDIDATE FACIES//////////////////////
    /////// FOR THEORY AND FORMULATION, SEE PROGRAM MANUAL IN THE SECTION "MARKOV CHAIN RANDOM FIELD SIMULATION" ///////

    // A lambda function to reuse the multiplaction operator over all the facies found in
    // samples and previously simulated cells.
    const VerticalTransiogramModel& transiogramModel = *m_transiogramModel;
    auto lambdaMultiplicationProbs = [ faciesFromCodesAndSuccessionSeparations, transiogramModel ] ( uint faciesCodeTo ) {
        double result = 1.0;
        std::vector< std::pair< FaciesCodeFrom, SuccessionSeparation > >::const_iterator it =
                faciesFromCodesAndSuccessionSeparations.cbegin();
        //iterate over all "from" facies codes, which reside in the primary data samples and previously simulated nodes
        //found in the search neighborhood
        for( ; it != faciesFromCodesAndSuccessionSeparations.cend(); ++it ){
            uint faciesCodeFrom = (*it).first;
            double h = (*it).second;
            double probability = transiogramModel.getTransitionProbability( faciesCodeFrom, faciesCodeTo, h );
            result *= probability;
        }
        return result;
    };

    //compute the denominator (a summation of multiplications) part of the MCRF equation
    double denominator = 0.0;
    for( uint iFaciesTo = 0; iFaciesTo < cd->getCategoryCount(); ++iFaciesTo ){
        //get the "to" facies code
        uint toFaciesCode = cd->getCategoryCode( iFaciesTo );
        denominator += lambdaMultiplicationProbs( toFaciesCode );
    }

    //for each possible facies that can be assigned to the simulation cell
    for( uint iCandidateFacies = 0; iCandidateFacies < cd->getCategoryCount(); ++iCandidateFacies ){
        //get the candidate facies code
        uint candidateFaciesCode = cd->getCategoryCode( iCandidateFacies );
        //compute the numerator (a multiplication) part of the MCRF equation
        double numerator = lambdaMultiplicationProbs( candidateFaciesCode );
        //finaly compute the probability according to transiography (primary data and previously simulated cells)
        double probabilityFromTransiography = numerator / denominator;
        //set the probability in the Tau Model
        tauModelCopy.setProbabilityFromSource( iCandidateFacies,
                                              static_cast<uint>( ProbabilitySource::FROM_TRANSIOGRAM ),
                                              probabilityFromTransiography );
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //get the probabilities of facies from secondary data (collocated in simulation grid) for the Tau Model
    if( useSecondaryData() ){
        //for each category
        for( unsigned int categoryIndex = 0; categoryIndex < cd->getCategoryCount(); ++categoryIndex ){
            uint probColumnIndex = m_probFields[ categoryIndex ]->getAttributeGEOEASgivenIndex() - 1;
            double probabilityFromSecondary = simulationCell.readValueFromDataSet(  );
            tauModelCopy.setProbabilityFromSource( categoryIndex,
                                                  static_cast<uint>( ProbabilitySource::FROM_SECONDARY_DATA ),
                                                  probabilityFromSecondary );
        }
    }

    ///====================================MONTE CARLO DRAW=============================================

    //make a cumulative probability function
    typedef double CumulativeProbability;
    std::vector< CumulativeProbability > cdf;
    cdf.reserve( cd->getCategoryCount() );
    double cumulativeProbability = 0.0;
    for( unsigned int categoryIndex = 0; categoryIndex < cd->getCategoryCount(); ++categoryIndex ){
        double prob = tauModelCopy.getFinalProbability( categoryIndex );

        //caps cumulative probability at 1.0 (user can model a transiogram that sums more than 1.0 at certain lags)
        if( cumulativeProbability + prob > 1.0 )
            cumulativeProbability = 1.0;
        else
            cumulativeProbability += prob;

        cdf.push_back( cumulativeProbability );
    }

    //ensures cdf ends at 1.0 (user can model a transiogram that sums less than 1.0 at certain lags)
    if( cumulativeProbability < 1.0 ){
        cumulativeProbability = 1.0;
        //forces last probability to 1.0
        cdf[cdf.size()-1] = 1.0;
    }

    //sanity check
    double tolerance = 0.1;
    if( cumulativeProbability + tolerance < 1.0 || cumulativeProbability - tolerance > 1.0 ){
        return m_simGridNDV;
    }

    //Draw a cumulative probability from an uniform distribution
    std::uniform_real_distribution<double> uniformDistributionBetween0and1( 0.0, 1.0 );
    double drawnCumulativeProbability = uniformDistributionBetween0and1( randomNumberGenerator );

    //return the facies code
    for( unsigned int categoryIndex = 0; categoryIndex < cd->getCategoryCount()-1; ++categoryIndex ){
        if( drawnCumulativeProbability > cdf[ categoryIndex ] && drawnCumulativeProbability <= cdf[ categoryIndex+1 ] )
            return cd->getCategoryCode( categoryIndex+1 );
        else if( drawnCumulativeProbability <= cdf[ 0 ] )
            return cd->getCategoryCode( 0 );
    }

    //execution is not supposed to reach this point
    return m_simGridNDV;
}

/** ///////////// Simulate some realizations in a separate thread. /////////////////////////
 * @param nRealsForOneThread The number of realizations the thread should simulate.
 * @param cgSim The simulation grid.
 * @param seed The seed for the random number generator (should be different from those of the other threads)
 * @param mcrfSim The pointer to the MCRFSim object coordinating the simulation.
 * @param completedFlag The pointer to an output boolean variable.
 *                      It'll receive the "true" value upon completion of all simulations.
 * @param realizations A pointer to a vector of spectral::array objects where the thread will deposit simulated data.
 *                     Each spectral::array contains the simulated data of one realization.
 *//////////////////////////////////////////////////////////////////////////////////////////
void simulateSomeRealizationsThread( uint nRealsForOneThread,
                                     const CartesianGrid* cgSim,
                                     uint seed,
                                     MCRFSim* mcrfSim,
                                     bool* completedFlag,
                                     std::vector< spectral::arrayPtr >* realizationsOutput ){

    //initialize the thread-local random number generator with the seed reserved for this thread
    std::mt19937 randomNumberGenerator;
    randomNumberGenerator.seed( seed );

    //define a uniform distribution between 0 and an integer called RAND_MAX
    std::uniform_int_distribution<long> distribution( 0, RAND_MAX );

    //get simulation grid dimensions
    uint nI = cgSim->getNI();
    uint nJ = cgSim->getNJ();
    uint nK = cgSim->getNK();
    ulong nCells = nI * nJ * nK;

    ulong numberOfSimulationsExecuted = 0;
    ulong reportProgressEveryNumberOfSimulations = 1000;

    // A lambda function for the random walk generation
    // Note: the "mutable" keyword is in the lambda declaration because we need to capture the distribution and random
    // number generator objects as non-const references, as inherently using them changes their state.
    auto lambdaFnShuffler = [ distribution, randomNumberGenerator ] (int i) mutable {
        return static_cast<int>( distribution(randomNumberGenerator) % i );
    };

    //for each realization of this thread
    for( uint iRealization = 0; iRealization < nRealsForOneThread; ++iRealization ){

        //init realization data with the sim grid's NDV
        spectral::arrayPtr simulatedData = spectral::arrayPtr( new spectral::array( nI, nJ, nK, cgSim->getNoDataValueAsDouble() ) );

        //prepare a vector with the random walk (sequence of linear cell indexes to simulate)
        std::vector<ulong> linearIndexesRandomWalk;
        linearIndexesRandomWalk.reserve( nCells );
        for (int i=0; i<nCells; ++i) linearIndexesRandomWalk.push_back(i);

        // shuffles the cell linear indexes to make the random walk.
        std::random_shuffle( linearIndexesRandomWalk.begin(), linearIndexesRandomWalk.end(), lambdaFnShuffler );

        //traverse the grid's cells according to the random walk.
        for( uint iRandomWalkIndex = 0; iRandomWalkIndex < nCells; ++iRandomWalkIndex ){
            //get the cell's linear index
            uint iCellLinearIndex = linearIndexesRandomWalk[ iRandomWalkIndex ];
            //get the IJK cell index
            uint i, j, k;
            cgSim->indexToIJK( iCellLinearIndex, i, j, k );
            //simulate the cell (attention: may return the simulation grid's no-data value)
            double catCode = mcrfSim->simulateOneCellMT( i, j, k, randomNumberGenerator, *simulatedData );
            //save the value to the data array of the realization
            (*simulatedData)( i, j, k ) = catCode;
            //keep track of simulation progress
            ++numberOfSimulationsExecuted;
            if( ! ( numberOfSimulationsExecuted % reportProgressEveryNumberOfSimulations ) )
                mcrfSim->setOrIncreaseProgressMT( reportProgressEveryNumberOfSimulations );
        } //grid traversal (random walk)

        //return the realization data
        realizationsOutput->push_back( simulatedData );

    } // for each reazation of this thread

    //signals the client code that this thread finished
    *completedFlag = true;
}
///////////////////////////////////////////////////////////////////////////////


bool MCRFSim::run()
{
    //check whether everything is ok
    if( !isOKtoRun() )
        return false;

    //sets progress count to zero
    m_progress = 0;

    //clears any previous realization data
    m_realizations.clear();

    //get simulation grid dimensions
    uint nI = m_cgSim->getNI();
    uint nJ = m_cgSim->getNJ();
    uint nK = m_cgSim->getNK();

    //get the simulation grid's NDV (getNoDataValueAsDouble() is expensive)
    m_simGridNDV = m_cgSim->getNoDataValueAsDouble();

    //get the number of realizations the user wants to simulate
    uint nRealizations = m_commonSimulationParameters->getNumberOfRealizations();

    //get the number of threads from max number of threads set by the user
    //or number of realizations (whichever is the lowest)
    unsigned int nThreads = std::min( m_maxNumberOfThreads, nRealizations );

    //loads the a priori facies distribution from the filesystem
    m_pdf->loadPairs();

    //loads the transiogram model data.
    m_transiogramModel->readFromFS();

    //loads category information from filesystem
    CategoryDefinition* cd = m_pdf->getCategoryDefinition();
    cd->loadQuintuplets();

    //announce the simulation has begun.
    Application::instance()->logInfo("Commencing MCRF simulation with " + QString::number(nThreads) + " thread(s).");

    //distribute the realizations among the n-threads
    uint numberOfRealizationsForAThread[nThreads];
    for( uint iThread = 0; iThread < nThreads; ++iThread )
        numberOfRealizationsForAThread[ iThread ] = 0; //initialize the numbers of realizations with zeros
    for( uint iReal = 0; iReal < nRealizations; ++iReal )
        ++numberOfRealizationsForAThread[ iReal % nThreads ];

    //create vectors of data arrays for each thread, so they deposit their realizations in them.
    std::vector< spectral::arrayPtr > realizationDepots[nThreads];

    //create an array of flags that tells whether a thread is completed.
    // intialize all with false
    bool completed[ nThreads ];
    for( uint iThread = 0; iThread < nThreads; ++iThread )
        completed[ iThread ] = false;

    // Build the search strategy.
    {
        double hMax              =         m_commonSimulationParameters->getSearchEllipHMax();
        double hMin              =         m_commonSimulationParameters->getSearchEllipHMin();
        double hVert             =         m_commonSimulationParameters->getSearchEllipHVert();
        double azimuth           =         m_commonSimulationParameters->getSearchEllipAzimuth();
        double dip               =         m_commonSimulationParameters->getSearchEllipDip();
        double roll              =         m_commonSimulationParameters->getSearchEllipRoll();
        uint nb_samples          =         m_commonSimulationParameters->getNumberOfSamples();
        uint min_nb_samples      =         m_commonSimulationParameters->getMinNumberOfSamples();
        uint numberOfSectors     =         m_commonSimulationParameters->getNumberOfSectors();
        uint minSamplesPerSector =         m_commonSimulationParameters->getMinNumberOfSamplesPerSector();
        uint maxSamplesPerSector =         m_commonSimulationParameters->getMaxNumberOfSamplesPerSector();
        double minDistanceBetweensamples = m_commonSimulationParameters->getMinDistanceBetweenSecondaryDataSamples();
        uint nbSimNodesConditioning      = m_commonSimulationParameters->getNumberOfSimulatedNodesForConditioning();
        SearchNeighborhoodPtr searchNeighborhood(
                    new SearchEllipsoid(hMax, hMin, hVert,
                                        azimuth, dip, roll,
                                        numberOfSectors, minSamplesPerSector, maxSamplesPerSector
                                        )
                    );
        m_searchStrategyPrimary = SearchStrategyPtr( new SearchStrategy( searchNeighborhood, nb_samples, 0.0, min_nb_samples ) );
        m_searchStrategySimGrid = SearchStrategyPtr( new SearchStrategy( searchNeighborhood, nbSimNodesConditioning, minDistanceBetweensamples, 0 ) );
    }

    // Build spatial indexes
    {
        //////////////////////////////////
        QProgressDialog progressDialog;
        progressDialog.show();
        progressDialog.setLabelText("Building spatial indexes...");
        progressDialog.setMinimum(0);
        progressDialog.setValue(0);
        progressDialog.setMaximum( 0 );
        QApplication::processEvents();
        /////////////////////////////////
        m_spatialIndexOfPrimaryData->clear();
        {
            //for the primary data
            if( m_dfPrimary->getFileType() == "POINTSET" ){
                PointSet* psPrimary = dynamic_cast<PointSet*>( m_dfPrimary );
                m_spatialIndexOfPrimaryData->fill( psPrimary, m_cgSim->getDX() ); //use cell size as tolerance
            } else if (m_dfPrimary->getFileType() == "SEGMENTSET") {
                SegmentSet* ssPrimary = dynamic_cast<SegmentSet*>( m_dfPrimary );
                m_spatialIndexOfPrimaryData->fill( ssPrimary, m_cgSim->getDX() ); //use cell size as tolerance
            } else {
                m_lastError = "Error building spatial indexes: primary data of type " + m_dfPrimary->getFileType() + " are not currently supported.";
                return false;
            }
        }
        m_spatialIndexOfSimGrid->clear();
        m_spatialIndexOfSimGrid->fill( m_cgSim );
    }


    // Build an appropriate Tau Model object
    {
        CategoryDefinition* cd = m_pdf->getCategoryDefinition();
        uint nSourcesOfInformation = 1; //assumes only transiograms will provide probabilities for the facies
        if( useSecondaryData() )
            nSourcesOfInformation = 2;
        m_tauModel = TauModelPtr( new TauModel( cd->getCategoryCount(), nSourcesOfInformation ) );
        //set the tau factors
        m_tauModel->setTauFactor( static_cast<uint>(ProbabilitySource::FROM_TRANSIOGRAM),
                                  m_tauFactorForTransiography );
        if( useSecondaryData() )
            m_tauModel->setTauFactor( static_cast<uint>(ProbabilitySource::FROM_SECONDARY_DATA),
                                      m_tauFactorForProbabilityFields );
    }

    //configure and display a progress bar for the simulation task
    //////////////////////////////////
    m_progressDialog = new QProgressDialog;
    m_progressDialog->show();
    m_progressDialog->setLabelText("Running MCRF...");
    m_progressDialog->setMinimum(0);
    m_progressDialog->setValue(0);
    m_progressDialog->setMaximum( nI * nJ * nK * nRealizations );
    /////////////////////////////////

    //create and run the simulation threads
    std::thread threads[nThreads];
    for( unsigned int iThread = 0; iThread < nThreads; ++iThread){
        std::vector< spectral::arrayPtr >& realizationDepot = realizationDepots[iThread];
        //Give a different seed to each thread by multiplying the user-given seed by 100 and adding the thread number
        //NOTE on the seed number * 100:
        //number of realizations is capped at 99, so even if there are more than 99
        //logical processors, number of threads will be limited to 99
        threads[iThread] = std::thread( simulateSomeRealizationsThread,
                                        numberOfRealizationsForAThread[ iThread ],
                                        m_cgSim,
                                        m_commonSimulationParameters->getSeed() * 100 + iThread,
                                        this,
                                        &(completed[ iThread ]),
                                        &realizationDepot
                                        );
    }

    //this non-locking loop allows the progress dialog to update (Qt runs in this thread).
    //while the worker threads run.
    bool allThreadsFinished = false;
    int progressRefreshCount = 0;
    int progressRefreshRate = 1000000; // the greater the number, less frequent Qt repaints
    while( ! allThreadsFinished ){
        //monitor the finished flags for all running threads
        allThreadsFinished = true;
        for( unsigned int iThread = 0; iThread < nThreads; ++iThread)
            if( ! completed[iThread] )
                allThreadsFinished = false;
        //allows Qt to redraw stuff as well as respond to events from time to time.
        ++progressRefreshCount;
        if( progressRefreshCount > progressRefreshRate ){
            updateProgessUI();
            progressRefreshCount = 0;
        }
    }

    //lock-wait for the threads to finish their execution contexts.
    for( unsigned int iThread = 0; iThread < nThreads; ++iThread)
        threads[iThread].join();

    //collect the realizations created by the threads:
    for( unsigned int iThread = 0; iThread < nThreads; ++iThread ){
        std::vector< spectral::arrayPtr >::iterator it = realizationDepots[ iThread ].begin();
        for(; it != realizationDepots[ iThread ].end(); ++it)
            m_realizations.push_back( *it );
    }

    //hide the progress dialog
    delete m_progressDialog;

    //announce the simulation has completed with success
    Application::instance()->logInfo("MCRF completed.");
    return true;
}

void MCRFSim::setOrIncreaseProgressMT(ulong ammount, bool increase)
{
    std::unique_lock<std::mutex> lck ( m_mutexMCRF, std::defer_lock );
    lck.lock(); //this code is expected to be called concurrently from multiple simulation threads
    if( increase )
        m_progress += ammount;
    else
        m_progress = ammount;
    lck.unlock();
}

void MCRFSim::updateProgessUI()
{
    m_progressDialog->setValue( m_progress );
    QApplication::processEvents();
}

DataCellPtrMultiset MCRFSim::getSamplesFromPrimaryMT(const GridCell &simulationCell) const
{
    DataCellPtrMultiset result;
    if( m_searchStrategyPrimary && m_atPrimary ){

        //if the user set the max number of primary data samples to search to zero, returns the empty result.
        if( ! m_searchStrategyPrimary->m_nb_samples )
            return result;

        //Fetch the indexes of the samples to be used in the simulation.
        QList<uint> samplesIndexes = m_spatialIndexOfPrimaryData->getNearestWithinGenericRTreeBased( simulationCell, *m_searchStrategyPrimary );
        QList<uint>::iterator it = samplesIndexes.begin();

        //Create and collect the searched sample objects, which depend on the type of the input file.
        for( ; it != samplesIndexes.end(); ++it ){
            switch ( m_primaryDataType ) {
            case PrimaryDataType::POINTSET:
            {
                DataCellPtr p(new PointSetCell( static_cast<PointSet*>( m_primaryDataFile ), m_atPrimary->getAttributeGEOEASgivenIndex()-1, *it ));
                p->computeCartesianDistance( simulationCell );
                result.insert( p );
            }
                break;
            case PrimaryDataType::CARTESIANGRID:
            {
                CartesianGrid* cg = static_cast<CartesianGrid*>( m_primaryDataFile );
                uint i, j, k;
                cg->indexToIJK( *it, i, j, k );
                DataCellPtr p(new GridCell( cg, m_atPrimary->getAttributeGEOEASgivenIndex()-1, i, j, k ));
                p->computeCartesianDistance( simulationCell );
                result.insert( p );
            }
                break;
            case PrimaryDataType::GEOGRID:
            {
                Application::instance()->logError( "MCRFSim::getSamplesFromPrimary(): GeoGrids cannot be used as primary data yet.  Must create a class like GeoGridCell inheriting from DataCell." );
            }
                break;
            case PrimaryDataType::SEGMENTSET:
            {
                DataCellPtr p(new SegmentSetCell( static_cast<SegmentSet*>( m_primaryDataFile ), m_atPrimary->getAttributeGEOEASgivenIndex()-1, *it ));
                p->computeCartesianDistance( simulationCell );
                result.insert( p );
            }
                break;
            default:
                Application::instance()->logError( "MCRFSim::getSamplesFromPrimary(): Primary data file type not recognized or undefined." );
            }
        }

    } else {
        Application::instance()->logError( "MCRFSim::getSamplesFromPrimary(): sample search failed.  Search strategy and/or primary data not set." );
    }
    return result;
}

DataCellPtrMultiset MCRFSim::getNeighboringSimGridCellsMT(const GridCell &simulationCell) const
{
    DataCellPtrMultiset result;
    if( m_searchStrategySimGrid && m_cgSim ){

        //if the user set the number of cells to search to zero, returns the empty result.
        if( ! m_searchStrategySimGrid->m_nb_samples )
            return result;

        //Fetch the indexes of the samples to be used in the simulation.
        QList<uint> samplesIndexes;
        if( m_commonSimulationParameters->getSearchAlgorithmOptionForSimGrid() == 0 )
            samplesIndexes = m_spatialIndexOfSimGrid->getNearestWithinGenericRTreeBased( simulationCell, *m_searchStrategySimGrid );
        else
            samplesIndexes = m_spatialIndexOfSimGrid->getNearestWithinTunedForLargeDataSets( simulationCell, *m_searchStrategySimGrid );
        QList<uint>::iterator it = samplesIndexes.begin();

        //Create and collect the searched sample objects, which depend on the type of the input file.
        for( ; it != samplesIndexes.end(); ++it ){
            uint i, j, k;
            m_cgSim->indexToIJK( *it, i, j, k );
            DataCellPtr p(new GridCell( m_cgSim, -1, i, j, k ));
            p->computeCartesianDistance( simulationCell );
            result.insert( p );
        }

    } else {
        Application::instance()->logError( "MCRFSim::getNeighboringSimGridCellsMT(): simulation grid search failed.  Search strategy and/or simulation grid not set." );
    }
    return result;
}
