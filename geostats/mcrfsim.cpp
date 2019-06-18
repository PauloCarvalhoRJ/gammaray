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
#include <random>
#include <QApplication>
#include <QProgressDialog>

MCRFSim::MCRFSim() :
    m_atPrimary( nullptr),
    m_cgSim( nullptr ),
    m_pdf( nullptr ),
    m_transiogramModel( nullptr ),
    m_gradationField( nullptr ),
    m_probFields( std::vector< Attribute*>() ),
    m_tauFactorForTransiography( 1.0 ),
    m_tauFactorForProbabilityFields( 1.0 ),
    m_commonSimulationParameters( nullptr ),
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
        } else {
            m_primaryDataFile = dfPrimary;
            if( dfPrimary->getFileType() == "POINTSET" )
                m_primaryDataType = PrimaryDataType::POINTSET;
            if( dfPrimary->getFileType() == "CARTESIANGRID" )
                m_primaryDataType = PrimaryDataType::CARTESIANGRID;
            if( dfPrimary->getFileType() == "GEOGRID" )
                m_primaryDataType = PrimaryDataType::GEOGRID;
            if( dfPrimary->getFileType() == "SEGMENTSET" )
                m_primaryDataType = PrimaryDataType::SEGMENTSET;
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

    if( ! m_gradationField ){
        m_lastError = "Use of a gradation field is required to stablish a correlation between vertical (time) and lateral facies succession in 3D Markov Chain.";
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

bool MCRFSim::useSecondaryData() const
{
    return ! m_probFields.empty();
}

double MCRFSim::simulateOneCellMT(uint i, uint j, uint k, const spectral::array& simulatedData ) const
{

    //get the facies set to be simulated
    CategoryDefinition* cd = m_pdf->getCategoryDefinition();

    double anisoVerticalAxis = 0.0;
    {

    }

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

    //get the probabilities from the global PDF, they're the marginal
    //probabilities for the Tau Model
    for( int categoryIndex = 0; categoryIndex < cd->getCategoryCount(); ++categoryIndex )
        tauModelCopy.setMarginalProbability( categoryIndex, m_pdf->get2ndValue( categoryIndex ) );

    //for each primary datum found in the search neighborhood
    DataCellPtrMultiset::iterator itSamples = vSamplesPrimary.begin();
    for( uint i = 0; i < vSamplesPrimary.size(); ++i, ++itSamples){

        //TODO: ROAD WORK.

    }

    //for each grid cells found in the search neighborhood
    DataCellPtrMultiset::iterator itSimGridCells = vNeighboringSimGridCells.begin();
    for( uint i = 0; i < vNeighboringSimGridCells.size(); ++i, ++itSimGridCells){
        DataCellPtr dataCell = *itSimGridCells;
        //we know the data cell is a grid cell
        const GridCell* gridCellAspect = dynamic_cast<GridCell*>(dataCell.get());

        //get the realization value in the neighboring cell (may be NDV)
        double realizationValue = simulatedData( gridCellAspect->_indexIJK._i,
                                                 gridCellAspect->_indexIJK._j,
                                                 gridCellAspect->_indexIJK._k );

        //if there is a previously simulated data in the neighboring cell
        // DataFile::isNDV() is non-const and has a slow string-to-double conversion
        if( ! Util::almostEqual2sComplement( m_simGridNDV, realizationValue, 1 ) ){

        }

        //TODO: ROAD WORK.

    }

    //get the probabilities of facies from secondary data (in simulation grid) for the Tau Model
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

    //define a uniform distribution
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
            double catCode = mcrfSim->simulateOneCellMT( i, j, k, *simulatedData );
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

    //get the number of threads from logical CPUs or number of realizations (whichever is the lowest)
    unsigned int nThreads = std::min( std::thread::hardware_concurrency(), nRealizations );

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
            DataFile* dfPrimary = dynamic_cast<DataFile*>( m_atPrimary->getContainingFile() );
            if( dfPrimary->getFileType() == "POINTSET" ){
                PointSet* psPrimary = dynamic_cast<PointSet*>( dfPrimary );
                m_spatialIndexOfPrimaryData->fill( psPrimary, m_cgSim->getDX() ); //use cell size as tolerance
            } else if (dfPrimary->getFileType() == "SEGMENTSET") {
                SegmentSet* ssPrimary = dynamic_cast<SegmentSet*>( dfPrimary );
                m_spatialIndexOfPrimaryData->fill( ssPrimary, m_cgSim->getDX() ); //use cell size as tolerance
            } else {
                m_lastError = "Error building spatial indexes: primary data of type " + dfPrimary->getFileType() + " are not currently supported.";
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
