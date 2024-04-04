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
#include "spatialindex/spatialindex.h"
#include "geostats/searchneighborhood.h"
#include "geostats/searchannulus.h"
#include "geostats/searchstrategy.h"
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

DataCellPtrMultiset ContactAnalysis::getSamplesFromInputDataSet(const DataCell &sample,
                                                                const SearchStrategy& searchStrategyPrimary,
                                                                const SpatialIndex& spatialIndexOfPrimaryData ) const
{
    DataCellPtrMultiset result;
    if( m_attributeGrade ){

        //if the user set the max number of primary data samples to search to zero, returns the empty result.
        if( ! searchStrategyPrimary.m_nb_samples )
            return result;

        //Fetch the indexes of the samples to be used in the simulation.
        QList<uint> samplesIndexes = spatialIndexOfPrimaryData.getNearestWithinGenericRTreeBased( sample, searchStrategyPrimary );
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
                Application::instance()->logError( "ContactAnalysis::getSamplesFromInputDataSet(): GeoGrids cannot be used as input data yet."
                                                   "  It is necessary to create a class like GeoGridCell inheriting from DataCell." );
                return result;
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

    //load the input data file
    m_inputDataFile->loadData();

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
        } else {
            m_lastError = "Error building spatial index: input data of type " + m_inputDataFile->getFileType() + " are not currently supported.";
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

    //for each lag
    for( uint16_t iLag = 0; iLag < m_numberOfLags; iLag++, current_lag += m_lagSize ){

        //build the search strategy for current lag
        //TODO: the search strategy may vary according to input data set type, whether it is 2D/3D or
        //      whether the mode is lateral or vertical.
        SearchStrategyPtr searchStrategy;
        {
            uint nb_samples          =         m_maxNumberOfSamples;
            uint min_nb_samples      =         m_minNumberOfSamples;
            SearchNeighborhoodPtr searchNeighborhood( new SearchAnnulus( current_lag - m_lagSize, current_lag,
                                                                         min_nb_samples,          nb_samples ) );
            searchStrategy.reset( new SearchStrategy( searchNeighborhood, nb_samples, 0.0, min_nb_samples ) );
        }

        //for each data sample
        for( uint64_t iSample = 0; iSample < m_inputDataFile->getDataLineCount(); iSample++, total_done_so_far++ ){

            //update the progress bar 1% of the time to not impact performance
            if( ! ( total_done_so_far % ( (total_steps>1000?total_steps:100) / 100 ) ) ){ //if total steps is less than 1000,
                                                                                          //then update the progress bar for every step
                progressDialog.setValue( total_done_so_far );
                QApplication::processEvents();
            }

            //if the data sample has already been visited, skip to the next sample
            if( visitedSamplesIndexes.find( iSample ) != visitedSamplesIndexes.end() )
                continue;

            //construct a data cell representing the current sample according to the input data set type
            //this object is used in spatial queries
            DataCellPtr currentSampleCell;
            switch ( m_inputDataType ) {
                case InputDataSetType::POINTSET:
                    currentSampleCell.reset( new PointSetCell( dynamic_cast<PointSet*>(m_inputDataFile),
                                                               m_attributeGrade->getAttributeGEOEASgivenIndex()-1,
                                                               iSample ) );
                    break;
                case InputDataSetType::CARTESIANGRID:
                    {
                    CartesianGrid* cgAspect = dynamic_cast<CartesianGrid*>(m_inputDataFile);
                    uint i, j, k;
                    cgAspect->indexToIJK( iSample, i, j, k );
                    currentSampleCell.reset( new GridCell( cgAspect,
                                                           m_attributeGrade->getAttributeGEOEASgivenIndex()-1,
                                                           i, j, k ) );
                    }
                    break;
                case InputDataSetType::GEOGRID:
                    m_lastError = "Geogrids are not currently supported.";
                    return false;
                case InputDataSetType::SEGMENTSET:
                    currentSampleCell.reset( new SegmentSetCell( dynamic_cast<SegmentSet*>(m_inputDataFile),
                                                                 m_attributeGrade->getAttributeGEOEASgivenIndex()-1,
                                                                 iSample ) );
                    break;
                default:
                    m_lastError = "Unspecified input data type.  This is an internal error.  "
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

                //TODO:

                //add the processed cell to the visited list
                visitedSamplesIndexes.insert( neighborSampleCell->getDataRowIndex() );
            }

        }
    } //for each lag

    return true;
}
