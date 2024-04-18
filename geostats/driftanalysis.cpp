#include "driftanalysis.h"

#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "domain/datafile.h"
#include "domain/geogrid.h"
#include "domain/pointset.h"
#include "domain/segmentset.h"
#include "geometry/boundingbox.h"
#include "geostats/searchbox.h"
#include "geostats/searchstrategy.h"
#include "spatialindex/spatialindex.h"

#include <QProgressDialog>
#include <QApplication>

DriftAnalysis::DriftAnalysis() :
    m_inputDataFile(nullptr),
    m_attribute(nullptr),
    m_NumberOfSteps(2),
    m_lastError(""),
    m_inputDataType( DataSetType::UNDEFINED )
{}

// getters and setters
DataFile *DriftAnalysis::getInputDataFile() const{return m_inputDataFile;}
void DriftAnalysis::setInputDataFile(DataFile *inputDataFile){   m_inputDataFile = inputDataFile;}
Attribute *DriftAnalysis::getAttribute() const{    return m_attribute;}
void DriftAnalysis::setAttribute(Attribute *attribute){    m_attribute = attribute;}
uint16_t DriftAnalysis::getNumberOfSteps() const{    return m_NumberOfSteps;}
void DriftAnalysis::setNumberOfSteps(const uint16_t &numberOfSteps){    m_NumberOfSteps = numberOfSteps;}
QString DriftAnalysis::getLastError() const{    return m_lastError;}
std::vector<std::pair<DriftAnalysis::coordX, DriftAnalysis::Mean> > DriftAnalysis::getResultsWestEast() const{    return m_resultsWestEast;}
std::vector<std::pair<DriftAnalysis::coordY, DriftAnalysis::Mean> > DriftAnalysis::getResultsSouthNorth() const{    return m_resultsSouthNorth;}
std::vector<std::pair<DriftAnalysis::coordZ, DriftAnalysis::Mean> > DriftAnalysis::getResultsVertical() const{    return m_resultsVertical;}

bool DriftAnalysis::run()
{
    //assuming the algorithm will finish normally
    m_lastError = "";

    //check whether everything is ok to run
    if( ! isOKtoRun() )
        return false;

    //make sure the results vectors are empty
    m_resultsWestEast.clear();
    m_resultsSouthNorth.clear();
    m_resultsVertical.clear();

    //load the input data file
    m_inputDataFile->loadData();

    //get the data file column index corresponding to the attribute whose drift is to be analysed
    uint indexOfVariable = m_inputDataFile->getFieldGEOEASIndex( m_attribute->getName() ) - 1;
    if( indexOfVariable > m_inputDataFile->getDataColumnCount() ){
        m_lastError = "Error getting the data file column index of the categorical attribute with the domains.";
        return false;
    }

    //determine the type of the input data set once (avoid repetitive calls to the slow File::getFileType())
    //define a cell object that represents the current simulation cell.  Also construct a data cell concrete object
    //depending on the input data set type.
    if( m_inputDataFile->getFileType() == "POINTSET" )
        m_inputDataType = DataSetType::POINTSET;
    else if( m_inputDataFile->getFileType() == "CARTESIANGRID" )
        m_inputDataType = DataSetType::CARTESIANGRID;
    else if( m_inputDataFile->getFileType() == "GEOGRID" )
        m_inputDataType = DataSetType::GEOGRID;
    else if( m_inputDataFile->getFileType() == "SEGMENTSET" )
        m_inputDataType = DataSetType::SEGMENTSET;
    else {
        m_inputDataType = DataSetType::UNDEFINED;
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

    //determine the total number of processing steps (#of lags X #of samples)
    int total_steps = m_NumberOfSteps * ( m_inputDataFile->isTridimensional() ? 3 : 2 );

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
    int total_done_so_far = 0;

    //get the spatial extent of the data file
    BoundingBox bbox = m_inputDataFile->getBoundingBox();

    //get the sizes of the slices the three axes
    double sliceSizeWE = bbox.getXsize() / m_NumberOfSteps;
    double sliceSizeSN = bbox.getYsize() / m_NumberOfSteps;
    double sliceSizeVertical = bbox.getZsize() / m_NumberOfSteps;

    //get the input data file's NDV as floating point number
    double dummyValue = m_inputDataFile->getNoDataValueAsDouble();

    //compute drift along West->East diretcion (x axis).
    for( int iSlice = 0; iSlice < m_NumberOfSteps; iSlice++, total_done_so_far++ ){

        //define the X interval of the current X slice
        double sliceMin = bbox.m_minX + iSlice * sliceSizeWE;
        double sliceMax = sliceMin + sliceSizeWE;

        //build a bounding box representing the current X slice
        BoundingBox bbox( sliceMin, -std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(),
                          sliceMax,  std::numeric_limits<double>::max(),  std::numeric_limits<double>::max() );

        //do a spatial search, fetching the indexes of the samples lying within the current X slice
        QList<uint> samplesIndexes = spatialIndex.getWithinBoundingBox( bbox );

        //for each sample found inside the current X slice
        double sum = 0.0;
        uint64_t count = 0;
        for( uint sampleIndex : samplesIndexes ){
            double sampleValue = m_inputDataFile->dataConst( sampleIndex, indexOfVariable );
            if( ! Util::almostEqual2sComplement( sampleValue, dummyValue, 1 ) ){
                sum += sampleValue;
                count++;
            }
        }

        //compute the mean of the attribute within the current X slice
        double mean = std::numeric_limits<double>::quiet_NaN();
        if( count > 0 )
            mean = sum / count;

        //store the result for the current X slice
        m_resultsWestEast.push_back( { bbox.getCenterX(), mean } );

        progressDialog.setValue( total_done_so_far ); //update the progress bar
        QApplication::processEvents(); //let Qt repaint its widgets
    } // for each X slice

    //compute drift along South->North diretcion (y axis).
    for( int iSlice = 0; iSlice < m_NumberOfSteps; iSlice++, total_done_so_far++ ){

        //define the Y interval of the current Y slice
        double sliceMin = bbox.m_minY + iSlice * sliceSizeSN;
        double sliceMax = sliceMin + sliceSizeSN;

        //build a bounding box representing the current Y slice
        BoundingBox bbox( -std::numeric_limits<double>::max(), sliceMin, -std::numeric_limits<double>::max(),
                           std::numeric_limits<double>::max(), sliceMax,  std::numeric_limits<double>::max() );

        //do a spatial search, fetching the indexes of the samples lying within the current Y slice
        QList<uint> samplesIndexes = spatialIndex.getWithinBoundingBox( bbox );

        //for each sample found inside the current Y slice
        double sum = 0.0;
        uint64_t count = 0;
        for( uint sampleIndex : samplesIndexes ){
            double sampleValue = m_inputDataFile->dataConst( sampleIndex, indexOfVariable );
            if( ! Util::almostEqual2sComplement( sampleValue, dummyValue, 1 ) ){
                sum += sampleValue;
                count++;
            }
        }

        //compute the mean of the attribute within the current Y slice
        double mean = std::numeric_limits<double>::quiet_NaN();
        if( count > 0 )
            mean = sum / count;

        //store the result for the current Y slice
        m_resultsSouthNorth.push_back( { bbox.getCenterY(), mean } );

        progressDialog.setValue( total_done_so_far ); //update the progress bar
        QApplication::processEvents(); //let Qt repaint its widgets
    } // for each Y slice

    //compute drift along vertical diretcion (z axis).
    if( m_inputDataFile->isTridimensional() ) {
        for( int iSlice = 0; iSlice < m_NumberOfSteps; iSlice++, total_done_so_far++ ){

            //define the Z interval of the current Z slice
            double sliceMin = bbox.m_minZ + iSlice * sliceSizeVertical;
            double sliceMax = sliceMin + sliceSizeVertical;

            //build a bounding box representing the current Z slice
            BoundingBox bbox( -std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(), sliceMin,
                               std::numeric_limits<double>::max(),  std::numeric_limits<double>::max(), sliceMax );

            //do a spatial search, fetching the indexes of the samples lying within the current Z slice
            QList<uint> samplesIndexes = spatialIndex.getWithinBoundingBox( bbox );

            //for each sample found inside the current Z slice
            double sum = 0.0;
            uint64_t count = 0;
            for( uint sampleIndex : samplesIndexes ){
                double sampleValue = m_inputDataFile->dataConst( sampleIndex, indexOfVariable );
                if( ! Util::almostEqual2sComplement( sampleValue, dummyValue, 1 ) ){
                    sum += sampleValue;
                    count++;
                }
            }

            //compute the mean of the attribute within the current Z slice
            double mean = std::numeric_limits<double>::quiet_NaN();
            if( count > 0 )
                mean = sum / count;

            //store the result for the current Z slice
            m_resultsVertical.push_back( { bbox.getCenterZ(), mean } );

            progressDialog.setValue( total_done_so_far ); //update the progress bar
            QApplication::processEvents(); //let Qt repaint its widgets
        } // for each Z slice
    } // if data file is 3D


    return true;
}

bool DriftAnalysis::isOKtoRun()
{
    //assuming everything is ok
    m_lastError = "";

    if( ! m_attribute ){
        m_lastError = "Attribute not provided.";
        return false;
    }

    if( ! m_inputDataFile ){
        m_lastError = "Input data file not provided.";
        return false;
    } else {
        DataFile* dataFile = dynamic_cast<DataFile*>( m_attribute->getContainingFile() );
        if( ! dataFile ) {
            m_lastError = "Make sure the attributes have a valid parent data file.";
            return false;
        }
    }

    if( m_NumberOfSteps < 2 ){
        m_lastError = "Number of steps must be 2 or more.";
        return false;
    }

    return true;
}


