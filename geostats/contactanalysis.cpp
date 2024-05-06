#include "contactanalysis.h"

#include "util.h"
#include "domain/attribute.h"
#include "domain/datafile.h"
#include "domain/application.h"
#include "domain/categorydefinition.h"
#include "domain/pointset.h"
#include "domain/segmentset.h"
#include "domain/cartesiangrid.h"
#include "domain/segmentset.h"
#include "domain/geogrid.h"
#include "spatialindex/spatialindex.h"
#include "geostats/searchannulus.h"
#include "geostats/searchwasher.h"
#include "geostats/searchverticaldumbbell.h"
#include "geostats/searchstrategy.h"
#include "geostats/searchsphericalshell.h"
#include "geostats/pointsetcell.h"
#include "geostats/gridcell.h"
#include "geostats/segmentsetcell.h"

#include <QProgressDialog>
#include <QApplication>

#include <unordered_set>

ContactAnalysis::ContactAnalysis() :
    m_inputDataFile(nullptr),
    m_attributeGrade(nullptr),
    m_attributeDomains(nullptr),
    m_mode( ContactAnalysisMode::LATERAL ),
    m_ztolerance(0.0),
    m_domain1_code(0),
    m_domain2_code(0),
    m_minNumberOfSamples(1),
    m_maxNumberOfSamples(8),
    m_lagSize(0.0),
    m_numberOfLags(5),
    m_lastError(""),
    m_inputDataType( InputDataSetType::UNDEFINED )
{
}

//-----------------------------------------GETTERS AND SETTERS--------------------------------------------------------------
DataFile* ContactAnalysis::getInputDataFile() const{return m_inputDataFile;}
void ContactAnalysis::setInputDataFile(DataFile *inputDataFile){m_inputDataFile = inputDataFile;}
Attribute* ContactAnalysis::getAttributeGrade() const{return m_attributeGrade;}
void ContactAnalysis::setAttributeGrade(Attribute *attributeGrade){m_attributeGrade = attributeGrade;}
Attribute* ContactAnalysis::getAttributeDomains() const{return m_attributeDomains;}
void ContactAnalysis::setAttributeDomains(Attribute *attributeDomains){m_attributeDomains = attributeDomains;}
ContactAnalysisMode ContactAnalysis::getMode() const{return m_mode;}
void ContactAnalysis::setMode(const ContactAnalysisMode &mode){ m_mode = mode;}
double ContactAnalysis::getZtolerance() const{ return m_ztolerance;}
void ContactAnalysis::setZtolerance(double ztolerance){ m_ztolerance = ztolerance;}
uint16_t ContactAnalysis::getDomain1_code() const{return m_domain1_code;}
void ContactAnalysis::setDomain1_code(const uint16_t &domain1_code){m_domain1_code = domain1_code;}
uint16_t ContactAnalysis::getDomain2_code() const{return m_domain2_code;}
void ContactAnalysis::setDomain2_code(const uint16_t &domain2_code){m_domain2_code = domain2_code;}
uint16_t ContactAnalysis::getMinNumberOfSamples() const{return m_minNumberOfSamples;}
void ContactAnalysis::setMinNumberOfSamples(const uint16_t &minNumberOfSamples){m_minNumberOfSamples = minNumberOfSamples;}
uint16_t ContactAnalysis::getMaxNumberOfSamples() const{return m_maxNumberOfSamples;}
void ContactAnalysis::setMaxNumberOfSamples(const uint16_t &maxNumberOfSamples){m_maxNumberOfSamples = maxNumberOfSamples;}
double ContactAnalysis::getLagSize() const{return m_lagSize;}
void ContactAnalysis::setLagSize(double lagSize){m_lagSize = lagSize;}
uint16_t ContactAnalysis::getNumberOfLags() const{return m_numberOfLags;}
void ContactAnalysis::setNumberOfLags(const uint16_t &numberOfLags){m_numberOfLags = numberOfLags;}
//---------------------------------------------------------------------------------------------------------------------------

bool ContactAnalysis::isOKtoRun()
{
    //assuming everything is ok
    m_lastError = "";

    if( ! m_attributeDomains ){
        m_lastError = "Categorical attribute with the domains not provided.";
        return false;
    } else if( ! m_attributeDomains->isCategorical() ){
        m_lastError = "Attribute with the domains is not categorical.";
        return false;
    }

    if( ! m_attributeGrade ){
        m_lastError = "Attribute with the grade velues not provided.";
        return false;
    }

    if( ! m_inputDataFile ){
        m_lastError = "Input data file not provided.";
        return false;
    } else {
        DataFile* dataFileOfDomain = dynamic_cast<DataFile*>( m_attributeDomains->getContainingFile() );
        DataFile* dataFileOfGrade  = dynamic_cast<DataFile*>(   m_attributeGrade->getContainingFile() );
        if(              !  dataFileOfGrade ||
                         ! dataFileOfDomain ||
                (dataFileOfDomain != dataFileOfGrade)   ||
                (dataFileOfDomain != m_inputDataFile)   ||
                (dataFileOfGrade  != m_inputDataFile        )) {
            m_lastError = "Make sure both attributes have a valid parent data file and whether both belong to the"
                          " data file provided.";
            return false;
        }
    }

    if( m_mode == ContactAnalysisMode::VERTICAL && ! m_inputDataFile->isTridimensional() ){
        m_lastError = "Mode vertical is unavailable for non-3D datasets.";
        return false;
    }

    if( m_mode == ContactAnalysisMode::OMNI3D && ! m_inputDataFile->isTridimensional() ){
        m_lastError = "Mode omnidirectional is unavailable for non-3D datasets.";
        return false;
    }

    CategoryDefinition* cd = m_inputDataFile->getCategoryDefinition( m_attributeDomains );
    if( ! cd ){
        m_lastError = "Could not access the category definition of the attribute with the domains (nullptr).";
        return false;
    } else {
        int ret = cd->getCategoryIndex( m_domain1_code );
        if( ret < 0 ){
            m_lastError = "Domain 1 category code not found in the category definition of the attribute with the domains.";
            return false;
        }
        ret = cd->getCategoryIndex( m_domain2_code );
        if( ret < 0 ){
            m_lastError = "Domain 2 category code not found in the category definition of the attribute with the domains.";
            return false;
        }
    }

    if( m_domain1_code == m_domain2_code ){
        m_lastError = "The domain category codes must be different.";
        return false;
    }

    if( m_minNumberOfSamples >= m_maxNumberOfSamples ){
        m_lastError = "Minimum number of samples cannot be greater than or equal to maximum number of samples.";
        return false;
    }

    if( Util::almostEqual2sComplement( m_lagSize, 0.0, 1 ) ){
        m_lastError = "Lag size cannot be zero.";
        return false;
    }

    if( m_numberOfLags == 0 ){
        m_lastError = "Number of lags must be at least 1.";
        return false;
    }

    return true;
}

std::vector<std::pair<ContactAnalysis::Lag, ContactAnalysis::MeanGradesBothDomains> > ContactAnalysis::getResults() const
{
    return m_results;
}

DataCellPtrMultiset ContactAnalysis::getSamplesFromInputDataSet(const DataCell &sample,
                                                                const SearchStrategy& searchStrategy,
                                                                const SpatialIndex& spatialIndex ) const
{
    DataCellPtrMultiset result;
    if( m_attributeGrade ){

        //if the user set the max number of primary data samples to search to zero, returns the empty result.
        if( ! searchStrategy.m_nb_samples )
            return result;

        //Fetch the indexes of the samples to be used in the simulation.
        QList<uint> samplesIndexes = spatialIndex.getNearestWithinGenericRTreeBased( sample, searchStrategy );
        QList<uint>::iterator it = samplesIndexes.begin();

        //Create and collect the searched sample objects, which depend on the type of the input file.
        for( ; it != samplesIndexes.end(); ++it ){
            switch ( m_inputDataType ) {
            case InputDataSetType::POINTSET:
                {
                DataCellPtr p(new PointSetCell( static_cast<PointSet*>( m_inputDataFile ), m_attributeGrade->getAttributeGEOEASgivenIndex()-1, *it ));
                p->computeCartesianDistance( sample );
                result.insert( p );
                }
                break;
            case InputDataSetType::CARTESIANGRID:
                {
                CartesianGrid* cg = static_cast<CartesianGrid*>( m_inputDataFile );
                uint i, j, k;
                cg->indexToIJK( *it, i, j, k );
                DataCellPtr p(new GridCell( cg, m_attributeGrade->getAttributeGEOEASgivenIndex()-1, i, j, k ));
                p->computeCartesianDistance( sample );
                result.insert( p );
                }
                break;
            case InputDataSetType::GEOGRID:
                {
                GeoGrid* ggAspect = dynamic_cast<GeoGrid*>(m_inputDataFile);
                uint i, j, k;
                ggAspect->indexToIJK( *it, i, j, k );
                DataCellPtr p( new GridCell( ggAspect,
                               m_attributeGrade->getAttributeGEOEASgivenIndex()-1,
                               i, j, k ) );
                p->computeCartesianDistance( sample );
                result.insert( p );
                }
                break;
            case InputDataSetType::SEGMENTSET:
                {
                DataCellPtr p(new SegmentSetCell( static_cast<SegmentSet*>( m_inputDataFile ), m_attributeGrade->getAttributeGEOEASgivenIndex()-1, *it ));
                p->computeCartesianDistance( sample );
                result.insert( p );
                }
                break;
            default:
                Application::instance()->logError( "ContactAnalysis::getSamplesFromInputDataSet(): Input data file type not recognized or undefined." );
                return result;
            }
        }
    } else {
        Application::instance()->logError( "ContactAnalysis::getSamplesFromInputDataSet(): sample search failed.  Search strategy and/or input data not set." );
    }
    return result;
}

bool ContactAnalysis::run()
{
    //assuming the algorithm will finish normally
    m_lastError = "";

    //check whether everything is ok to run
    if( ! isOKtoRun() )
        return false;

    //make sure the results vector is empty
    m_results.clear();

    //load the input data file
    m_inputDataFile->loadData();

    //get the data file column index corresponding to the categorcial attribute with the domains
    uint indexOfDomainsVariable = m_inputDataFile->getFieldGEOEASIndex( m_attributeDomains->getName() ) - 1;
    if( indexOfDomainsVariable > m_inputDataFile->getDataColumnCount() ){
        m_lastError = "Error getting the data file column index of the categorical attribute with the domains.";
        return false;
    }

    //get the data file column index corresponding to the continuous attribute with the grade
    uint indexOfGradeVariable = m_inputDataFile->getFieldGEOEASIndex( m_attributeGrade->getName() ) - 1;
    if( indexOfGradeVariable > m_inputDataFile->getDataColumnCount() ){
        m_lastError = "Error getting the data file column index of the continuos attribute with the grade values.";
        return false;
    }

    //determine the type of the input data set once (avoid repetitive calls to the slow File::getFileType())
    //define a cell object that represents the current simulation cell.  Also construct a data cell concrete object
    //depending on the input data set type.
    if( m_inputDataFile->getFileType() == "POINTSET" )
        m_inputDataType = InputDataSetType::POINTSET;
    else if( m_inputDataFile->getFileType() == "CARTESIANGRID" )
        m_inputDataType = InputDataSetType::CARTESIANGRID;
    else if( m_inputDataFile->getFileType() == "GEOGRID" )
        m_inputDataType = InputDataSetType::GEOGRID;
    else if( m_inputDataFile->getFileType() == "SEGMENTSET" )
        m_inputDataType = InputDataSetType::SEGMENTSET;
    else {
        m_inputDataType = InputDataSetType::UNDEFINED;
        m_lastError = "Input data type not currently supported:" + m_inputDataFile->getFileType();
        return false;
    }

    //build the spatial index
    SpatialIndex spatialIndex;
    {
        //////////////////////////////////
        QProgressDialog progressDialog;
        progressDialog.show();
        progressDialog.setLabelText("Building spatial index...");
        progressDialog.setMinimum(0);
        progressDialog.setValue(0);
        progressDialog.setMaximum( 0 );
        QApplication::processEvents();
        //////////////////////////////////
        //the spatial index is filled differently depending on the type of the input data set
        if( m_inputDataFile->getFileType() == "POINTSET" ){
            PointSet* psAspect = dynamic_cast<PointSet*>( m_inputDataFile );
            spatialIndex.fill( psAspect, 0.1 );
        } else if ( m_inputDataFile->getFileType() == "SEGMENTSET") {
            SegmentSet* ssAspect = dynamic_cast<SegmentSet*>( m_inputDataFile );
            spatialIndex.fill( ssAspect, 0.1 ); //use cell size as tolerance
        } else if ( m_inputDataFile->getFileType() == "CARTESIANGRID") {
            CartesianGrid* cgAspect = dynamic_cast<CartesianGrid*>( m_inputDataFile );
            spatialIndex.fill( cgAspect );
        } else if ( m_inputDataFile->getFileType() == "GEOGRID") {
            GeoGrid* ggAspect = dynamic_cast<GeoGrid*>( m_inputDataFile );
            spatialIndex.fillWithCenters( ggAspect, 0.0001 );
        } else {
            m_lastError = "Internal error building spatial index: input data of type " + m_inputDataFile->getFileType() + " are not currently supported.";
            return false;
        }
    }

    //defining the first lag (increases outwards)
    double current_lag = m_lagSize;

    //determine the total number of processing steps (#of lags X #of samples)
    uint64_t total_steps = m_numberOfLags * m_inputDataFile->getDataLineCount();

    //configure and display a progress bar for the simulation task
    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Running contact analysis...");
    progressDialog.setMinimum(0);
    progressDialog.setValue(0);
    progressDialog.setMaximum( total_steps );
    /////////////////////////////////

    //this is to keep track of processing progress to update the progress bar
    uint64_t total_done_so_far = 0;

    //initialize a hash table containing the indexes of samples already visited (to be ignored).
    std::unordered_set<uint64_t> visitedSamplesIndexes( m_inputDataFile->getDataLineCount() );

    //prepare the containers to store the grade values for each domain.
    //the values within them will be used to compute the mean grade values for each category.
    //the inner vectors are in the same order of the lags (e.g. 1st inner vector -> 100m;
    //2nd inner vector -> 200m and so on until the largest lag.
    std::vector< std::vector<double> > gradesOfDomain1( m_numberOfLags );
    std::vector< std::vector<double> > gradesOfDomain2( m_numberOfLags );

    //for each lag
    for( uint16_t iLag = 0; iLag < m_numberOfLags; iLag++, current_lag += m_lagSize ){

        //build a search strategy for current lag appropriate for the input data set
        SearchStrategyPtr searchStrategy;
        {
            uint nb_samples     = m_maxNumberOfSamples;
            uint min_nb_samples = m_minNumberOfSamples;
            SearchNeighborhoodPtr searchNeighborhood;
            {
                if( m_mode == ContactAnalysisMode::LATERAL ){
                    if( ! m_inputDataFile->isTridimensional() ){ //if data set is 2D
                        searchNeighborhood.reset( new SearchAnnulus( current_lag - m_lagSize, current_lag ) );
                    } else { //if data set is 3D
                        if( ! m_inputDataFile->isGridded() ) {
                            searchNeighborhood.reset( new SearchWasher( current_lag - m_lagSize, current_lag, m_ztolerance*2 ) );
                        } else if ( m_inputDataType == InputDataSetType::CARTESIANGRID ){
                            CartesianGrid* cg = dynamic_cast< CartesianGrid* >( m_inputDataFile );
                            searchNeighborhood.reset( new SearchWasher( current_lag - m_lagSize, current_lag, cg->getDZ() ) );
                        } else {
                            m_lastError = "Internal error: no search neighborhood is available "
                                          "for lateral contact analysis with datasets of type " + m_inputDataFile->getFileType() + ".";
                            return false;
                        }
                    }
                } else if( m_mode == ContactAnalysisMode::VERTICAL ) {
                    if( ! m_inputDataFile->isTridimensional() ){ //if data set is 2D
                        m_lastError = "Cannot perform vertical contact analysis on a 2D dataset.";
                        return false;
                    } else { //if data set is 3D
                        if( ! m_inputDataFile->isGridded() ) {
                            searchNeighborhood.reset( new SearchVerticalDumbbell( m_lagSize, 2*(current_lag-m_lagSize), m_lagSize ) );
                        } else if ( m_inputDataType == InputDataSetType::CARTESIANGRID ) {
                            CartesianGrid* cg = dynamic_cast< CartesianGrid* >( m_inputDataFile );
                            const double dx = cg->getDX();
                            const double dy = cg->getDY();
                            double radius = std::sqrt( dx*dx + dy*dy ) / 2;
                            searchNeighborhood.reset( new SearchVerticalDumbbell( m_lagSize, 2*(current_lag-m_lagSize), radius ) );
                        } else if ( m_inputDataType == InputDataSetType::GEOGRID ) {
                            GeoGrid* gg = dynamic_cast< GeoGrid* >( m_inputDataFile );
                            double bbMinX, bbMinY, bbMinZ; //BB == Bounding Box
                            double bbMaxX, bbMaxY, bbMaxZ;
                            double maxBBdX = -std::numeric_limits<double>::max();
                            double maxBBdY = -std::numeric_limits<double>::max();
                            uint maxIndex = gg->getDataLineCount();
                            for( uint i = 0; i < maxIndex; i++ ){
                                gg->getCellBoundingBox( i, bbMinX, bbMinY, bbMinZ, bbMaxX, bbMaxY, bbMaxZ );
                                double bbDx = bbMaxX - bbMinX;
                                double bbDy = bbMaxY - bbMinY;
                                if( bbDx > maxBBdX ) maxBBdX = bbDx;
                                if( bbDy > maxBBdY ) maxBBdY = bbDy;
                            }
                            double radius = std::sqrt( maxBBdX*maxBBdX + maxBBdY*maxBBdY ) / 2;
                            searchNeighborhood.reset( new SearchVerticalDumbbell( m_lagSize, 2*(current_lag-m_lagSize), radius ) );
                        } else {
                            m_lastError = "Internal error: no search neighborhood is available "
                                          "for vertical contact analysis with datasets of type " + m_inputDataFile->getFileType() + ".";
                            return false;
                        }
                    }
                } else { //if m_mode == ContactAnalysisMode::OMNI3D
                    if( ! m_inputDataFile->isTridimensional() ){ //if data set is 2D
                        m_lastError = "Cannot perform omnidirectional contact analysis on a 2D dataset.";
                        return false;
                    } else { //if data set is 3D
                        searchNeighborhood.reset( new SearchSphericalShell( current_lag - m_lagSize, current_lag ) );
                    }
                }
            }
            searchStrategy.reset( new SearchStrategy( searchNeighborhood, nb_samples, 0.0, min_nb_samples ) );
        }

        //for each data sample
        for( uint64_t iCurrentSample = 0; iCurrentSample < m_inputDataFile->getDataLineCount(); iCurrentSample++, total_done_so_far++ ){

            //update the progress bar 1% of the time to not impact performance
            if( ! ( total_done_so_far % ( (total_steps>1000?total_steps:100) / 100 ) ) ){ //if total steps is less than 1000,
                                                                                          //then update the progress bar for every step
                progressDialog.setValue( total_done_so_far );
                QApplication::processEvents();
            }

            //if the data sample has already been visited, skip to the next sample
            //if( visitedSamplesIndexes.find( iCurrentSample ) != visitedSamplesIndexes.end() )
            //    continue;

            //get the domain category code of the current sample
            uint currentSampleDomainCategoryCode = m_inputDataFile->data( iCurrentSample, indexOfDomainsVariable );

            //construct a data cell representing the current sample according to the input data set type
            //this object is used in spatial queries
            DataCellPtr currentSampleCell;
            switch ( m_inputDataType ) {
                case InputDataSetType::POINTSET:
                    currentSampleCell.reset( new PointSetCell( dynamic_cast<PointSet*>(m_inputDataFile),
                                                               m_attributeGrade->getAttributeGEOEASgivenIndex()-1,
                                                               iCurrentSample ) );
                    break;
                case InputDataSetType::CARTESIANGRID:
                    {
                    CartesianGrid* cgAspect = dynamic_cast<CartesianGrid*>(m_inputDataFile);
                    uint i, j, k;
                    cgAspect->indexToIJK( iCurrentSample, i, j, k );
                    currentSampleCell.reset( new GridCell( cgAspect,
                                                           m_attributeGrade->getAttributeGEOEASgivenIndex()-1,
                                                           i, j, k ) );
                    }
                    break;
                case InputDataSetType::GEOGRID:
                    {
                    GeoGrid* ggAspect = dynamic_cast<GeoGrid*>(m_inputDataFile);
                    uint i, j, k;
                    ggAspect->indexToIJK( iCurrentSample, i, j, k );
                    currentSampleCell.reset( new GridCell( ggAspect,
                                                           m_attributeGrade->getAttributeGEOEASgivenIndex()-1,
                                                           i, j, k ) );
                    }
                break;
                case InputDataSetType::SEGMENTSET:
                    currentSampleCell.reset( new SegmentSetCell( dynamic_cast<SegmentSet*>(m_inputDataFile),
                                                                 m_attributeGrade->getAttributeGEOEASgivenIndex()-1,
                                                                 iCurrentSample ) );
                    break;
                default:
                    m_lastError = "Unspecified or unsupported input data type.  This is an internal error.  "
                                  "Please report a bug in the program's project website.";
                    return false;
            }

            //collect neighboring samples from the input data set ordered by their distance with respect
            //to the current sample.
            DataCellPtrMultiset vNeighboringSamples = getSamplesFromInputDataSet( *currentSampleCell, *searchStrategy, spatialIndex );

            //process each samples found in the search neighborhood around the current sample.
            //NOTE: the SpatialIndex::getNearestWithinGenericRTreeBased() method used in this->getSamplesFromInputDataSet()
            //      *does not* remove the cell corresponding to the current sample, so, the returned container contains
            //      the neighboring samples as well as the current sample.
            DataCellPtrMultiset::iterator itNeighborCells = vNeighboringSamples.begin();
            for( uint i = 0; i < vNeighboringSamples.size(); ++i, ++itNeighborCells){
                DataCellPtr neighborSampleCell = *itNeighborCells;

                //get the data file row corresponding to the neighbor cell
                uint64_t neighborSampleCellRowIndex = neighborSampleCell->getDataRowIndex();

                //if the neighbor sample has already been visited, skip to the next neighbor
                if( visitedSamplesIndexes.find( neighborSampleCellRowIndex ) != visitedSamplesIndexes.end() )
                    continue;

                //get the domain category code of the neighbor sample
                uint neighborSampleDomainCategoryCode = static_cast<uint>( m_inputDataFile->data( neighborSampleCellRowIndex, indexOfDomainsVariable ) );

                //if the neighbor is domain 1 and current sample is domain 2
                if( neighborSampleDomainCategoryCode == m_domain1_code &&
                    currentSampleDomainCategoryCode  == m_domain2_code ){
                    //store the grade value of the neighboring sample for computing the mean grade afterwards
                    gradesOfDomain1[iLag].push_back( neighborSampleCell->readValueFromDataSet() );
                    //add the processed cell to the visited list
                    visitedSamplesIndexes.insert( neighborSampleCellRowIndex );

                //if the neighbor is domain 2 and current sample is domain 1
                } else if( neighborSampleDomainCategoryCode == m_domain2_code &&
                           currentSampleDomainCategoryCode  == m_domain1_code ){
                   //store the grade value of the neighboring sample for computing the mean grade afterwards
                   gradesOfDomain2[iLag].push_back( neighborSampleCell->readValueFromDataSet() );
                   //add the processed cell to the visited list
                   visitedSamplesIndexes.insert( neighborSampleCellRowIndex );
               }

            } // for each neighboring sample
        } // for each sample
    } //for each lag (searching neighboring samples and fetching grade values)

    //get the input data file's NDV as floating point number
    double dummyValue = m_inputDataFile->getNoDataValueAsDouble();

    //process the results
    //for each lag
    current_lag = m_lagSize;
    for( uint16_t iLag = 0; iLag < m_numberOfLags; iLag++, current_lag += m_lagSize ){

        //compute the mean grade for domain 1
        double meanGradeDomain1 = std::numeric_limits<double>::quiet_NaN();
        {
            double sum = 0.0;
            uint64_t count = 0;
            for( double gradeValue : gradesOfDomain1[iLag] ){
                //if the grade value is not dummy (not informed)
                if( ! Util::almostEqual2sComplement( gradeValue, dummyValue, 1 ) ){
                    sum += gradeValue;
                    count++;
                }
            }
            if( count > 0 )
                meanGradeDomain1 = sum / count;
        }

        //compute the mean grade for domain 2
        double meanGradeDomain2 = std::numeric_limits<double>::quiet_NaN();
        {
            double sum = 0.0;
            double count = 0;
            for( double gradeValue : gradesOfDomain2[iLag] ){
                //if the grade value is not dummy (not informed)
                if( ! Util::almostEqual2sComplement( gradeValue, dummyValue, 1 ) ){
                    sum += gradeValue;
                    count++;
                }
            }
            if( count > 0 )
                meanGradeDomain2 = sum / count;
        }

        //store the result
        m_results.push_back( { current_lag, { meanGradeDomain1, meanGradeDomain2 } } );

    } //for each lag (computing the mean grades)

    return true;
}
