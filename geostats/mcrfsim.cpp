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
#include "domain/project.h"
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
#include <QDir>
#include<fstream>

MCRFSim::MCRFSim( MCRFMode mode ) :
    //---------simulation parameters----------------
    m_atPrimary( nullptr),
    m_gradationFieldOfPrimaryData( nullptr ),
    m_gradationFieldsOfPrimaryDataBayesian( std::vector< Attribute*>() ),
    m_cgSim( nullptr ),
    m_pdf( nullptr ),
    m_transiogramModel( nullptr ),
    m_transiogramModel2Bayesian( nullptr ),
    m_gradationFieldOfSimGrid( nullptr ),
    m_gradationFieldsOfSimGridBayesian( std::vector< Attribute*>() ),
    m_probFields( std::vector< Attribute*>() ),
    m_probsFieldsBayesian( std::vector< std::vector< Attribute* > >() ),
    m_tauFactorForTransiography( 1.0 ),
    m_tauFactorForTransiographyBayesianStarting(1.0),
    m_tauFactorForTransiographyBayesianEnding(5.0),
    m_tauFactorForProbabilityFields( 1.0 ),
    m_tauFactorForProbabilityFieldsBayesianStarting( 1.0 ),
    m_tauFactorForProbabilityFieldsBayesianEnding( 5.0 ),
    m_commonSimulationParameters( nullptr ),
    m_invertGradationFieldConvention( false ),
    m_maxNumberOfThreads( 1 ),
    //------other member variables--------------------
    m_mode( mode ),
    m_progressDialog( nullptr ),
    m_spatialIndexOfPrimaryData( new SpatialIndex() ),
    m_spatialIndexOfSimGrid( new SpatialIndex() ),
    m_primaryDataType( PrimaryDataType::UNDEFINED ),
    m_primaryDataFile( nullptr ),
    m_tauModel( nullptr ),
    m_realNumberForSaving( 1 )
{ }

bool MCRFSim::isOKtoRun()
{
    if( ! m_atPrimary ){
        m_lastError = "Categorical variable not provided.";
        return false;
    }

    if( !m_gradationFieldOfPrimaryData && m_gradationFieldsOfSimGridBayesian.empty() ){
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
        m_pdf->readFromFS();
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
        if( m_pdf->hasNegativeProbabilities() ){
            m_lastError = "PDF has negative probability values.";
            return false;
        }
        m_pdf->loadPairs();
        if( ! m_pdf->sumsToOne() ){
            m_lastError = "PDF's probabilities do not sum up to 1.0. Sum = " + QString::number( m_pdf->sumProbs() ) + " .";
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
        CategoryDefinition* cdOfTransiogramModel  = m_transiogramModel->getCategoryDefinition();
        if( cdOfPDF != cdOfTransiogramModel ){
            m_lastError = "Category definition of transiogram model must be the same object as that "
                          "the PDF is based on.";
            return false;
        }
    }

    if( m_mode == MCRFMode::BAYESIAN ){
        if( ! m_transiogramModel2Bayesian ){
            m_lastError = "2nd vertical transiogram model not provided.";
            return false;
        } else {
            CategoryDefinition* cdOfPrimData = m_dfPrimary->getCategoryDefinition( m_atPrimary );
            CategoryDefinition* cdOfTransiogramModel = m_transiogramModel2Bayesian->getCategoryDefinition();
            if( ! cdOfTransiogramModel ){
                m_lastError = "Category definition of 2nd vertical transiogram model not found (nullptr).";
                return false;
            }
            if( cdOfTransiogramModel != cdOfPrimData ){
                m_lastError = "Category definition of input variable must be the same object as that the 2nd "
                              "vertical transiogram model is based on.";
                return false;
            }
        }
        if( ! m_transiogramModel->isCompatibleWith( m_transiogramModel2Bayesian )){
            m_lastError = "Bayesian mode: the transiogram models that define the band are not compatible.";
            return false;
        }
        {
            CategoryDefinition* cdOfPDF = m_pdf->getCategoryDefinition();
            CategoryDefinition* cdOfTransiogramModel2 = m_transiogramModel2Bayesian->getCategoryDefinition();
            if( cdOfPDF != cdOfTransiogramModel2 ){
                m_lastError = "Bayesian mode: category definition of 2nd transiogram model must be the same object as "
                              "that the PDF is based on.";
                return false;
            }
        }
    }

    if( !m_gradationFieldOfSimGrid && m_gradationFieldsOfSimGridBayesian.empty() ){
        m_lastError = "Use of a gradation field in the simulation grid is required to stablish a correlation"
                      " between vertical (time) and lateral facies succession in 3D Markov Chain.";
        return false;
    }

    if( m_gradationFieldsOfPrimaryDataBayesian.size() != m_gradationFieldsOfSimGridBayesian.size() ){
        m_lastError = "Bayesian mode: the number of gradation field variables must be the same for both the"
                      " primary data and the simulation grid.";
        return false;
    }

    if( useSecondaryData() ){
        CategoryDefinition* cdOfPrimData = m_dfPrimary->getCategoryDefinition( m_atPrimary );
        cdOfPrimData->loadQuintuplets();
        int nCategories = cdOfPrimData->getCategoryCount();

        if( m_mode == MCRFMode::NORMAL ){
            int nProbFields = m_probFields.size();
            if( nProbFields != nCategories ){
                m_lastError = " Number of probability fields ( " +
                        QString::number(nProbFields) + " ) differs from the number of categories ( " +
                        QString::number(nCategories) + " ).";
                return false;
            }
        } else { //Bayesian execution mode
            int nProbFields = -1; //must be the same across all categories
            //for each category, we fetch its probability field set
            for( const std::vector<Attribute*> &probFieldsForACategory : m_probsFieldsBayesian ){
                if( nProbFields == -1 )
                    nProbFields = probFieldsForACategory.size();
                else if( nProbFields != probFieldsForACategory.size() ){
                    m_lastError = " Bayesian mode: Number of probability fields must be the same"
                                  " for all categories.";
                    return false;
                }
            }
            //all categories must have a set of probability fields
            if( nCategories != m_probsFieldsBayesian.size() ){
                m_lastError = " Bayesian mode: Number of probability field sets must match"
                              " the number of categories.";
                return false;
            }
        }

    }

    if( ! m_commonSimulationParameters ){
        m_lastError = "A common simulation parameter object was not provided.  This object contains non-Markov-specific parameters such"
                      " as neighborhood parameters, random number generator seed, number of realization, etc.";
        return false;
    } else {
        if( m_commonSimulationParameters->getSeed() > 20000000 ){
            m_lastError = "The seed for the random number generator must not exceed 20 millions.";
            return false;
        }
    }

    if( m_maxNumberOfThreads > 99 || m_maxNumberOfThreads < 1 ){
        m_lastError = "Max number of threads must be between 1 and 99.";
        return false;
    }

    //if the user opts to use the Cartesian grid-tuned algorithm, then the neighborhood
    //becomes a parallelepiped and not a ellipsoid for searches in the simulation grid
    //hence, any angles set to it are illegal.
    if( m_commonSimulationParameters->getSearchAlgorithmOptionForSimGrid() == 2){
        if( m_commonSimulationParameters->getSearchEllipAzimuth() != 0.0 ||
            m_commonSimulationParameters->getSearchEllipDip() != 0.0 ||
            m_commonSimulationParameters->getSearchEllipRoll() != 0.0  ){
            m_lastError = "Cannot set rotation angles if the search algorithm option is 'tuned for Cartesian grids' as the"
                          " search neighborhood becomes a parallelepiped for searches in the simulation grid.";
            return false;
        }
    }

    return true;
}

bool MCRFSim::useSecondaryData() const
{
    if( m_mode == MCRFMode::NORMAL )
        return ! m_probFields.empty();
    else //Bayesian execution mode
        return ! m_probsFieldsBayesian.empty();
}

double MCRFSim::simulateOneCellMT(uint i, uint j, uint k,
                                  std::mt19937 &randomNumberGenerator,
                                  double tauFactorForTransiography,
                                  double tauFactorForSecondaryData,
                                  const Attribute* gradFieldOfPrimaryDataToUse,
                                  const Attribute* gradFieldOfSimGridToUse,
                                  const VerticalTransiogramModel& transiogramToUse,
                                  const std::vector<Attribute *> &probFields,
                                  const spectral::array& simulatedData ) const
{

    //compute the vertical cell anisotropy, which is important to normalize the vertical separations.
    //this is important when the sim grid is in depositional domain, which normally has a vertical cell
    //size much greater than the lateral cell sizes.
    double vertAniso = m_cgSim->getDZ() / std::min( m_cgSim->getDX(), m_cgSim->getDY() );

    //get the facies set to be simulated
    CategoryDefinition* cd = m_pdf->getCategoryDefinition();

    //define a cell object that represents the current simulation cell
    GridCell simulationCell( m_cgSim, -1, i, j, k );

    //collect samples from the input data set ordered by their distance with respect
    //to the simulation cell.
    DataCellPtrMultiset vSamplesPrimary = getSamplesFromPrimaryMT( simulationCell );

    //collect neighboring simulation grid cells ordered by their distance with respect
    //to the simulation cell.
    DataCellPtrMultiset vNeighboringSimGridCells = getNeighboringSimGridCellsMT( simulationCell, simulatedData );

    //make a local copy of the Tau Model (this is potentially a multi-threaded code)
    TauModel tauModelCopy( *m_tauModel );

    //set the Tau factors.
    //These vary between realizations if this simulation's execution mode is for Bayesian application.
    //see the simulateSomeRealizationsThread() function.
    tauModelCopy.setTauFactor( static_cast<uint>(ProbabilitySource::FROM_TRANSIOGRAM),
                               tauFactorForTransiography );
    if( useSecondaryData() )
        tauModelCopy.setTauFactor( static_cast<uint>(ProbabilitySource::FROM_SECONDARY_DATA),
                                   tauFactorForSecondaryData );

    //get relevant information of the simulation cell
    uint simCellLinearIndex           = m_cgSim->IJKtoIndex( i, j, k );
    double simCellZ                   = m_cgSim->getDataSpatialLocation( simCellLinearIndex, CartesianCoord::Z );
    double simCellGradationFieldValue = m_cgSim->dataIJKConst( gradFieldOfSimGridToUse->getAttributeGEOEASgivenIndex()-1,
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
        if( ! m_primaryDataHasNDV || ! Util::almostEqual2sComplement( m_primaryDataNDV, sampleFaciesValue, 1 ) ){

            // get the sample's gradation field value
            double sampleGradationValue = sampleDataCell->readValueFromDataSet(
                        gradFieldOfPrimaryDataToUse->getAttributeGEOEASgivenIndex()-1 );

            //To preserve Markovian property, we cannot use data ahead in the facies succession.
            bool isAheadInSuccession = false;
            {
                isAheadInSuccession = isAheadInSuccession || ( sampleDataCell->_center._z > simCellZ ); // a sample location above the current cell is considered ahead (in time)
                //a sampple location ahead in the lateral facies succession should not be computed (Walther's Law)
                isAheadInSuccession = isAheadInSuccession || ( ! m_invertGradationFieldConvention && sampleGradationValue >  simCellGradationFieldValue );
                isAheadInSuccession = isAheadInSuccession || (   m_invertGradationFieldConvention && sampleGradationValue <= simCellGradationFieldValue );
            }

            if( ! isAheadInSuccession ){
                //get the facies code that was found in the neighboring primary data
                uint faciesCodeInSample = static_cast< uint >( sampleFaciesValue );
                //compute the resulting succession distance ( vector resulting from vertical separation and
                // variation in the gradation field - lateral succession separation )
                double faciesSuccessionDistance = 0.0;
                {
                    double verticalSeparation = ( simCellZ - sampleDataCell->_center._z ) / vertAniso;
                    double lateralSuccessionSeparation = sampleGradationValue - simCellGradationFieldValue;
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
            double neighborGradationFieldValue = m_cgSim->dataIJKConst( gradFieldOfSimGridToUse->getAttributeGEOEASgivenIndex()-1,
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
                    double verticalSeparation = ( simCellZ - neighborGridCellAspect->_center._z ) / vertAniso;
                    double lateralSuccessionSeparation = neighborGradationFieldValue - simCellGradationFieldValue;
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
    auto lambdaMultiplicationProbs = [ faciesFromCodesAndSuccessionSeparations, transiogramToUse ] ( uint faciesCodeTo ) {
        double result = 0.0; //assumes zero probability
        std::vector< std::pair< FaciesCodeFrom, SuccessionSeparation > >::const_iterator it =
                faciesFromCodesAndSuccessionSeparations.cbegin();
        //iterate over all "from" facies codes, which reside in the primary data samples and previously simulated nodes
        //found in the search neighborhood
        for( ; it != faciesFromCodesAndSuccessionSeparations.cend(); ++it ){
            uint faciesCodeFrom = (*it).first;
            double h = (*it).second;
            double probability = transiogramToUse.getTransitionProbability( faciesCodeFrom, faciesCodeTo, h );
            if( it == faciesFromCodesAndSuccessionSeparations.cbegin() )
                result = probability; //initialize the resulting probability with the first probability
            else
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
        double probabilityFromTransiography;
        if( denominator > 0.0 )
            probabilityFromTransiography = numerator / denominator;
        else
            probabilityFromTransiography = 0.0;
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
            uint probColumnIndex = probFields[ categoryIndex ]->getAttributeGEOEASgivenIndex() - 1;
            double probabilityFromSecondary = simulationCell.readValueFromDataSet( probColumnIndex );
            //if the value in the probability field is null, use the probability from the global PDF as default.
            if( Util::almostEqual2sComplement( m_simGridNDV, probabilityFromSecondary, 1 ) ){
                probabilityFromSecondary = m_pdf->get2ndValue( categoryIndex );
            }
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
        //assert( prob != 0.0 && "MCRFSim::simulateOneCellMT(): final probabilities are not supposed to be zero!");
        cumulativeProbability += prob;
        cdf.push_back( cumulativeProbability );
    }

    //ensures cdf ends at 1.0 (user can model a transiogram that sums less than 1.0 at certain lags)
    //the probs are rescaled so the cdf ends at 1.0.
    double ratio = 1.0 / cumulativeProbability;
    for(double &a : cdf) { a *= ratio; }

    //sanity checks
    double tolerance = 0.0001;
    cumulativeProbability = cdf.back();
    if( cumulativeProbability + tolerance < 1.0 || cumulativeProbability - tolerance > 1.0 ){
        assert( false && "MCRFSim::simulateOneCellMT(): Final probabilities for Monte Carlo not summing up to 1.0.");
        return m_simGridNDV;
    }
    for( double &cumulativeProbability : cdf )
        if( ! std::isfinite( cumulativeProbability )){
            assert( false && "MCRFSim::simulateOneCellMT(): At least one final probability for Monte Carlo is infinity or NaN.");
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
    assert( false && "MCRFSim::simulateOneCellMT(): Execution reached a point not supposed to.  "
                     "Please, check the sources of probabilities: global PDF, transiograms and secondary data.");
    return m_simGridNDV;
}

/** ///////////// Simulate some realizations in a separate thread. /////////////////////////
 * @param nRealsForOneThread The number of realizations the thread should simulate.
 * @param cgSim The simulation grid.
 * @param seed The seed for the random number generator (should be different from those of the other threads)
 * @param mcrfSim The pointer to the MCRFSim object coordinating the simulation.
 * @param completedFlag The pointer to an output boolean variable.
 *                      It'll receive the "true" value upon completion of all simulations.
 *//////////////////////////////////////////////////////////////////////////////////////////
void simulateSomeRealizationsThread( uint nRealsForOneThread,
                                     const CartesianGrid* cgSim,
                                     uint seed,
                                     MCRFSim* mcrfSim,
                                     bool* completedFlag ){

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

        // By default, the Tau factors are fixed (normal execution mode).
        double tauFactorForTransiographyInCurrentRealization = mcrfSim->m_tauFactorForTransiography;
        double tauFactorForSecondaryDataInCurrentRealization = mcrfSim->m_tauFactorForProbabilityFields;

        // By default, the gradation field to use is fixed (normal execution mode).
        Attribute* gradFieldOfPrimaryDataToUse = mcrfSim->m_gradationFieldOfPrimaryData;
        Attribute* gradFieldOfSimGridToUse = mcrfSim->m_gradationFieldOfSimGrid;

        // By default, the probability fields for each category are fixed (normal execution mode).
        std::vector<Attribute*> probFieldsToUse = mcrfSim->m_probFields;

        // If the execution mode is for Bayesian application, some hyperparameters vary
        // from realization to realization, thus...
        if( mcrfSim->getMode() == MCRFMode::BAYESIAN ){
            // assuming the number of gradation field values of the primary data chosen is the
            // same of the simulation grid.
            int nGradationFields = mcrfSim->m_gradationFieldsOfPrimaryDataBayesian.size();
            // assuming the number of probability fields for all categories is the same as
            // the number of probability fields of the first category.
            int nProbFieldsPerCategory = mcrfSim->m_probsFieldsBayesian[0].size();
            // ...make uniform distributions for the intervals set by the user.
            std::uniform_real_distribution<double> distributionTauFactorForTransiography(
                        mcrfSim->m_tauFactorForTransiographyBayesianStarting,
                        mcrfSim->m_tauFactorForTransiographyBayesianEnding);
            std::uniform_real_distribution<double> distributionTauFactorForSecondaryData(
                        mcrfSim->m_tauFactorForProbabilityFieldsBayesianStarting,
                        mcrfSim->m_tauFactorForProbabilityFieldsBayesianEnding);
            std::uniform_int_distribution<int> distributionGradFieldIndexes( 0, nGradationFields-1 );
            std::uniform_int_distribution<int> distributionProbFieldSetIndexes( 0, nProbFieldsPerCategory-1 );
            // ...draw new Tau factors from the user-given interval.
            tauFactorForTransiographyInCurrentRealization =
                    distributionTauFactorForTransiography( randomNumberGenerator );
            tauFactorForSecondaryDataInCurrentRealization =
                    distributionTauFactorForSecondaryData( randomNumberGenerator );
            // ...draw the gradation field to be used.
            int selectedGradFieldIndex = distributionGradFieldIndexes ( randomNumberGenerator );
            gradFieldOfPrimaryDataToUse = mcrfSim->m_gradationFieldsOfPrimaryDataBayesian.at( selectedGradFieldIndex );
            gradFieldOfSimGridToUse = mcrfSim->m_gradationFieldsOfSimGridBayesian.at( selectedGradFieldIndex );
            // ...draw the set of probability fields to be used.
            int selectedProbFieldSetIndex = distributionProbFieldSetIndexes( randomNumberGenerator );
            probFieldsToUse = std::vector<Attribute*>();
            //for each category... (elements in outer vector are for each category)
            for( const std::vector<Attribute*>& probFieldsOfACategory : mcrfSim->m_probsFieldsBayesian ){
                //... get the probability field index from the set defined by the user for it.
                // elements of the inner vector are the probability fields for one category.
                probFieldsToUse.push_back( probFieldsOfACategory[ selectedProbFieldSetIndex ] );
            }
        } else { //normal execution mode (fixed hyperparameters)
            // Call the random number generator the same number of times of Bayesian mode to keep the
            // same random path of the other execution mode.
            randomNumberGenerator();
            randomNumberGenerator();
            randomNumberGenerator();
            randomNumberGenerator();
        }

        // By default, the transiogram model is fixed (normal exection mode).
        VerticalTransiogramModel transiogramToUse("", "");
        transiogramToUse.makeAsSameModel( *(mcrfSim->m_transiogramModel) );

        // If the execution mode is for Bayesian application, the transiogram model must vary
        // randomly from realization to realization within the band of uncertainty given by the user.
        int transiogramMatrixDimension = transiogramToUse.getRowOrColCount();
        if( mcrfSim->getMode() == MCRFMode::BAYESIAN ){
            for( uint iRow = 0; iRow < transiogramMatrixDimension; ++iRow )
                for( uint iCol = 0; iCol < transiogramMatrixDimension; ++iCol ){
                    //Get the ranges of variable transiogram parameters.
                    double sill1 = mcrfSim->m_transiogramModel         ->getSill( iRow, iCol );
                    double sill2 = mcrfSim->m_transiogramModel2Bayesian->getSill( iRow, iCol );
                    double range1 = mcrfSim->m_transiogramModel         ->getRange( iRow, iCol );
                    double range2 = mcrfSim->m_transiogramModel2Bayesian->getRange( iRow, iCol );
                    //Make sure they are in ascending order.
                    Util::ensureAscending( sill1,  sill2  );
                    Util::ensureAscending( range1, range2 );
                    //Make uniform distribution for the transiogram parameters.
                    std::uniform_real_distribution<double> distributionForSill (  sill1,  sill2 );
                    std::uniform_real_distribution<double> distributionForRange( range1, range2 );
                    //Draw transiogram parameters values.
                    double drawnSill  = distributionForSill ( randomNumberGenerator );
                    double drawnRange = distributionForRange( randomNumberGenerator );
                    //Assign them to the transiogram model to be used for simulation.
                    transiogramToUse.setSill ( iRow, iCol, drawnSill  );
                    transiogramToUse.setRange( iRow, iCol, drawnRange );
                }
            //Making sure all sills sum 1.0 rowwise (important for Markovian transiography).
            transiogramToUse.unitizeRowwiseSills();
        } else { //normal execution mode (fixed transiography)
            for( int iRow = 0; iRow < transiogramMatrixDimension; ++iRow )
                for( int iCol = 0; iCol < transiogramMatrixDimension; ++iCol ){
                    // Call the random number generator the same number of times of Bayesian mode to keep the
                    // same random path of the other execution mode.
                    randomNumberGenerator();
                    randomNumberGenerator();
                }
        }

        //traverse the grid's cells according to the random walk.
        for( uint iRandomWalkIndex = 0; iRandomWalkIndex < nCells; ++iRandomWalkIndex ){
            //get the cell's linear index
            uint iCellLinearIndex = linearIndexesRandomWalk[ iRandomWalkIndex ];
            //get the IJK cell index
            uint i, j, k;
            cgSim->indexToIJK( iCellLinearIndex, i, j, k );
            //simulate the cell (attention: may return the simulation grid's no-data value)
            double catCode = mcrfSim->simulateOneCellMT( i, j, k,
                                                         randomNumberGenerator,
                                                         tauFactorForTransiographyInCurrentRealization,
                                                         tauFactorForSecondaryDataInCurrentRealization,
                                                         gradFieldOfPrimaryDataToUse,
                                                         gradFieldOfSimGridToUse,
                                                         transiogramToUse,
                                                         probFieldsToUse,
                                                         *simulatedData );
            //save the value to the data array of the realization
            (*simulatedData)( i, j, k ) = catCode;
            //keep track of simulation progress
            ++numberOfSimulationsExecuted;
            if( ! ( numberOfSimulationsExecuted % reportProgressEveryNumberOfSimulations ) )
                mcrfSim->setOrIncreaseProgressMT( reportProgressEveryNumberOfSimulations );
        } //grid traversal (random walk)

        //save the realization data (where depends on the user settings).
        mcrfSim->saveRealizationMT( simulatedData,
                                    transiogramToUse,
                                    probFieldsToUse,
                                    gradFieldOfSimGridToUse,
                                    gradFieldOfPrimaryDataToUse,
                                    tauFactorForTransiographyInCurrentRealization,
                                    tauFactorForSecondaryDataInCurrentRealization );

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

    //inits realization number for saving name.
    m_realNumberForSaving = 1;

    //deletes the previous simulation report file (used for Bayesian mode) if it exists.
    if( m_mode == MCRFMode::BAYESIAN ){
        QFile file( getReportFilePathForBayesianModeMT() );
        if( file.exists() )
            file.remove();
    }

    //get simulation grid dimensions
    uint nI = m_cgSim->getNI();
    uint nJ = m_cgSim->getNJ();
    uint nK = m_cgSim->getNK();

    //get the simulation grid's NDV (getNoDataValueAsDouble() is expensive)
    m_simGridNDV = m_cgSim->getNoDataValueAsDouble();

    //get the primary data's NDV (getNoDataValueAsDouble() is expensive)
    m_primaryDataHasNDV = m_dfPrimary->hasNoDataValue();
    m_primaryDataNDV = m_dfPrimary->getNoDataValueAsDouble();

    //get the number of realizations the user wants to simulate
    uint nRealizations = m_commonSimulationParameters->getNumberOfRealizations();

    //get the number of threads from max number of threads set by the user
    //or number of realizations (whichever is the lowest)
    unsigned int nThreads = std::min( m_maxNumberOfThreads, nRealizations );

    //loads the a priori facies distribution from the filesystem
    m_pdf->loadPairs();

    //loads the transiogram model data.
    m_transiogramModel->readFromFS();
    if( m_mode == MCRFMode::BAYESIAN )
        m_transiogramModel2Bayesian->readFromFS();

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

    //suspend logging during processing
    Application::instance()->logErrorOff();
    Application::instance()->logWarningOff();
    Application::instance()->logInfoOff();

    //create and run the simulation threads
    std::thread threads[nThreads];
    for( unsigned int iThread = 0; iThread < nThreads; ++iThread){
        //Give a different seed to each thread by multiplying the user-given seed by 100 and adding the thread number
        //NOTE on the seed number * 100:
        //number of threads is capped at 99
        threads[iThread] = std::thread( simulateSomeRealizationsThread,
                                        numberOfRealizationsForAThread[ iThread ],
                                        m_cgSim,
                                        m_commonSimulationParameters->getSeed() * 100 + iThread,
                                        this,
                                        &(completed[ iThread ])
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

    //flush any log messages that may have been issued during the simulation
    Application::instance()->logErrorOn();
    Application::instance()->logWarningOn();
    Application::instance()->logInfoOn();

    //hide the progress dialog
    delete m_progressDialog;

    //define the realization variables as categorical (depending on how user opted for
    //saving them).
    switch ( m_commonSimulationParameters->getSaveRealizationsOption() ) {
    case 0: //save realizations as new variables in the simulation grid
        {
            //The simulation threads do a bulk writing of simulation results
            //to the grid's physical file, so we need to update its object
            //representation (CartesianGrid)
            m_cgSim->updateChildObjectsCollection();
            //Get the number of variables of the simulation grid after the simulation.
            uint nVariablesAfter = m_cgSim->getDataColumnCount();
            //set the new variables as categorical
            //iStop is used to stop iterating from back towards first variable.
            for( uint iVar = nVariablesAfter-1, iStop = 0; iStop < nRealizations; --iVar, ++iStop ){
                m_cgSim->setCategorical( iVar, cd );
                // update the metadata file
                m_cgSim->updateMetaDataFile();
                // updates properties list so any changes appear in the project tree.
                m_cgSim->updateChildObjectsCollection();
            }
        }
        break;
    case 1: //save realizations as separante grid files in the project
        //assert( false && "MCRFSim::run(): save realizations mode not implemented: 1.");
        {
            //in saveRealizationMT(), several GEO-EAS grids with the ".TOCOPY" each with one realization
            //were generated in the project's temp directory.  So we need to...

            //For each *.TOCOPY files in the project's temp directory
            QDir dir( Application::instance()->getProject()->getTmpPath() );
            dir.setNameFilters(QStringList() << "*.TOCOPY");
            dir.setFilter(QDir::Files);
            foreach(QString dirFile, dir.entryList()) //foreach is a Qt macro
            {
                //create a local grid object corresponding to the file created in the tmp directory
                //with a simulated realization.
                CartesianGrid local_cg( Application::instance()->getProject()->getTmpPath() +
                                        QDir::separator() + dirFile );
                //set the geometry info so they match that of the simulation grid
                local_cg.setInfoFromOtherCGonlyGridSpecs( m_cgSim );
                //remove the .TOCOPY extension from the file name
                dirFile = dirFile.replace( ".TOCOPY", "" );
                //import the newly created grid file as a global project item
                CartesianGrid* new_cg = Application::instance()->getProject()->
                        importCartesianGrid( &local_cg, dirFile );
                //update the path information in the global Cartesian grid object. Now we can set
                //metadata info for the project.
                new_cg->setPath(
                            Application::instance()->getProject()->getPath() + QDir::separator() + dirFile );
                //set the sole variable as categorical
                new_cg->setCategorical( 0, cd );
                // update the metadata file
                new_cg->updateMetaDataFile();
                // updates properties list so any changes appear in the project tree.
                new_cg->updateChildObjectsCollection();
                // delete the file from project's temp dir (recall it still has the .TOCOPY file exitension there)
                dir.remove(dirFile + ".TOCOPY");
            }
        }
        break;
    case 2: //save realizations as grid files somewhere
        /* No post processing of realizations is necessary. */;
    }

    //make sure newly added objects during simulation
    //show up in the interface after it has completed
    Application::instance()->refreshProjectTree();

    //announce the simulation has completed with success
    Application::instance()->logInfo("MCRF completed.");
    return true;
}

void MCRFSim::setOrIncreaseProgressMT(ulong ammount, bool increase)
{
    std::unique_lock<std::mutex> lck ( m_mutexMCRF, std::defer_lock );
    lck.lock(); //this code is expected to be called concurrently from multiple simulation threads
                //so we define a critical section.
    if( increase )
        m_progress += ammount;
    else
        m_progress = ammount;
    lck.unlock();
}

void MCRFSim::saveRealizationMT( const spectral::arrayPtr simulatedData,
                                 VerticalTransiogramModel &transiogramUsed,
                                 const std::vector<Attribute*> &probFieldsUsed,
                                 const Attribute* gradFieldOfSimGridUsed,
                                 const Attribute* gradFieldOfPrimaryDataUsed,
                                 double tauFactorForTransiographyUsed,
                                 double tauFactorForSecondaryDataUsed )
{
    std::unique_lock<std::mutex> lck ( m_mutexSaveRealizations, std::defer_lock );
    lck.lock(); //this code is expected to be called concurrently from multiple simulation threads
                //so we define a critical section to avoid file corruption due to race condition.

    /*BEGIN CRITICAL SECTION*/
    {

        //Make the realization name.
        QString realizationName;
        {
            QString s1 = m_commonSimulationParameters->getBaseNameForRealizationVariables();
            QString s2 = Util::zeroPad( m_realNumberForSaving, 4 );
            realizationName = s1 + s2;
        }

        //How to save the realization depends on user's choices.
        switch ( m_commonSimulationParameters->getSaveRealizationsOption() ) {
        case 0: //save to the simulation grid
            {
                QString NDV = "-999999";
                if( m_cgSim->hasNoDataValue() )
                    NDV = m_cgSim->getNoDataValue();
                Util::appendPhysicalGEOEASColumn( simulatedData, realizationName, m_cgSim->getPath(), NDV );
            }
            break;
        case 1: //save realization as separate grid in the project
            {
                //write it to the project's temp directory
                //in MCRFSim::run() they will be copied to the project's directory, added to the project and
                //the variabled set as categorical. We can't do these tasks here because some of the used Qt
                //funcionalities are not thread safe.
                QString tmp_file_path = Application::instance()->getProject()->getTmpPath() + QDir::separator()
                                       + realizationName + ".TOCOPY";
                Util::createGEOEASGrid( realizationName, *simulatedData, tmp_file_path, true );
            }
            break;
        case 2: //save realization as grid files somewhere
            //write it to the directory defined by the user
            QString file_path = m_commonSimulationParameters->getSaveRealizationsPath() + QDir::separator()
                              + realizationName + ".dat";
            Util::createGEOEASGrid( realizationName, *simulatedData, file_path, true );
        }

        //If execution mode is for Bayesian application, then transiogram and hyperparameters vary.
        //Hence, we have to report each transiogram used as well as the hyperparameters used in each realization.
        if( m_mode == MCRFMode::BAYESIAN ){
            //apend the model parameters and algorithm hyperparameters to the report file.
            //the file mode creates it if it does not exist.
            std::ofstream reportFile;
            reportFile.open( getReportFilePathForBayesianModeMT().toStdString(), fstream::app );
            reportFile << "<REALIZATION>" << std::endl;
            reportFile << "\t<name>" << realizationName.toStdString() << "</name>" << std::endl;
            reportFile << "\t<transiogram>" << std::endl;
            //Saves the used transiogram to a temporary file, loads it into a string, appends it to the report
            {
                QString tmpPath = Application::instance()->getProject()->generateUniqueTmpFilePath("transiogram");
                transiogramUsed.setPath( tmpPath );
                transiogramUsed.writeToFS();
                std::ifstream t( tmpPath.toStdString() );
                std::stringstream buffer;
                buffer << t.rdbuf();
                reportFile << buffer.str();
            }
            reportFile << "\t</transiogram>" << std::endl;
            reportFile << "\t<prob_fields>" << std::endl;
            if( useSecondaryData() ){
                const CategoryDefinition* cd = transiogramUsed.getCategoryDefinition();
                if( cd ){
                    for( uint iCatIndex = 0; iCatIndex < cd->getCategoryCount(); ++iCatIndex ){
                        reportFile << "\t\t<prob_field>";
                        reportFile << "<facies>" << cd->getCategoryName( iCatIndex ).toStdString() << "</facies><field>" <<
                                      probFieldsUsed[ iCatIndex ]->getName().toStdString() << "</field>";
                        reportFile << "</prob_field>" << std::endl;
                    }
                } else {
                    reportFile << "\t\t<error>Category definition returned a null pointer.</error>" << std::endl;
                }
            }
            reportFile << "\t</prob_fields>" << std::endl;
            reportFile << "\t<grad_field_sim_grid>";
            reportFile << gradFieldOfSimGridUsed->getName().toStdString();
            reportFile << "</grad_field_sim_grid>" << std::endl;
            reportFile << "\t<grad_field_prim_data>";
            reportFile << gradFieldOfPrimaryDataUsed->getName().toStdString();
            reportFile << "</grad_field_prim_data>" << std::endl;
            reportFile << "\t<tau_factor_transiogram>";
            reportFile << tauFactorForTransiographyUsed;
            reportFile << "</tau_factor_transiogram>" << std::endl;
            reportFile << "\t<tau_factor_prob_fields>";
            reportFile << tauFactorForSecondaryDataUsed;
            reportFile << "</tau_factor_prob_fields>" << std::endl;
            reportFile << "</REALIZATION>" << std::endl;
            reportFile.close();
        } // if( m_mode == MCRFMode::BAYESIAN )

        //increases the realization number for the next realization to be saved
        m_realNumberForSaving++;
    }
    /* END CRITICAL SECTION */

    lck.unlock();
}

MCRFMode MCRFSim::getMode() const
{
    return m_mode;
}

void MCRFSim::setMode(const MCRFMode &mode)
{
    m_mode = mode;
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

DataCellPtrMultiset MCRFSim::getNeighboringSimGridCellsMT(const GridCell &simulationCell,
                                                          const spectral::array& simulatedData) const
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
        else if( m_commonSimulationParameters->getSearchAlgorithmOptionForSimGrid() == 1 )
            samplesIndexes = m_spatialIndexOfSimGrid->getNearestWithinTunedForLargeDataSets( simulationCell, *m_searchStrategySimGrid );
        else{
            uint nCellsIDirection = m_commonSimulationParameters->getSearchEllipHMin() / m_cgSim->getDX() * 2.0;
            uint nCellsJDirection = m_commonSimulationParameters->getSearchEllipHMax() / m_cgSim->getDY() * 2.0;
            uint nCellsKDirection = m_commonSimulationParameters->getSearchEllipHVert() / m_cgSim->getDZ() * 2.0;
            if( nCellsIDirection < 1 ) nCellsIDirection = 1;
            if( nCellsJDirection < 1 ) nCellsJDirection = 1;
            if( nCellsKDirection < 1 ) nCellsKDirection = 1;
            //The simulation grid is necessarily a Cartesian grid
            samplesIndexes = m_spatialIndexOfSimGrid->getNearestFromCartesianGrid( simulationCell,
                                                                                   *m_searchStrategySimGrid,
                                                                                   true,
                                                                                   m_simGridNDV,
                                                                                   nCellsIDirection,
                                                                                   nCellsJDirection,
                                                                                   nCellsKDirection,
                                                                                   &simulatedData.d_ );
        }

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

QString MCRFSim::getReportFilePathForBayesianModeMT() const
{
    QString directory;
    if( m_commonSimulationParameters->getSaveRealizationsOption() == 2 /* save realization outside project directory */ )
        directory = m_commonSimulationParameters->getSaveRealizationsPath();
    else /* user opted to save realizations inside the project (either as separate files or variables in the simulation grid*/
        directory = Application::instance()->getProject()->getTmpPath();
    return directory + QDir::separator() + "latestMCRFSimBayesianReport.xml";
}
