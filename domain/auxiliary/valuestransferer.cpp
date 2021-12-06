#include "valuestransferer.h"

#include "domain/attribute.h"
#include "domain/datafile.h"
#include "domain/application.h"
#include "domain/geogrid.h"
#include "domain/cartesiangrid.h"
#include "domain/pointset.h"
#include "domain/section.h"

#include <QProgressDialog>
#include <QApplication>
#include <iostream>

ValuesTransferer::ValuesTransferer(const QString newAttributeName,
                                   DataFile *dfDestination,
                                   const Attribute *atOrigin) :
    m_newAttributeName( newAttributeName ),
    m_dfDestination( dfDestination ),
    m_atOrigin( atOrigin )

{

}

bool ValuesTransferer::transfer()
{
    DataFile* dfDest = m_dfDestination;
    DataFile* dfOrig = dynamic_cast<DataFile*>( m_atOrigin->getContainingFile() );

    if( ! dfDest->hasNoDataValue() ){
        Application::instance()->logError("ValuesTransferer::transfer(): destination file must have no-data value defined.");
        return false;
    }

    if( dfDest ){
        if( dfOrig->getFileType() == "CARTESIANGRID" && dfDest->getFileType() == "GEOGRID" )
            return transferFromCGtoGG();
        else if( dfOrig->getFileType() == "CARTESIANGRID" && dfDest->getFileType() == "POINTSET" )
            return transferFromCGtoPS();
        else if( dfOrig->getFileType() == "POINTSET" && dfDest->getFileType() == "CARTESIANGRID" )
            return transferFromPStoCG();
        else if( dfOrig->getFileType() == "CARTESIANGRID" && dfDest->getFileType() == "CARTESIANGRID" ) {
            ProjectComponent* parentOfDest = dfDest->getParent();
            if( parentOfDest->getTypeName() != "SECTION" ) { //if the destination Cartesian grid is not the data store
                                                             //of a geologic section, select CG->CG transfer.
                return transferFromCGtoCG();
            } else {
                Application::instance()->logInfo("ValuesTransferer::transfer(): destination Cartesian grid is the data store"
                                                 " of a geologic section.  Data locations will be those defined by the Section"
                                                 " geometry and not of the grid.");
                return transferFromCGtoSection();
            }
        } else
            Application::instance()->logError("ValuesTransferer::transfer(): collocated transfer of values from a " +
                                              dfOrig->getFileType()
                                              + " to a " +
                                              dfDest->getFileType()
                                              + " is not currently implemented.");
    } else
        Application::instance()->logError("ValuesTransferer::transfer(): destination file is not a DataFile or is a null pointer.");
    return false;
}

bool ValuesTransferer::transferFromCGtoGG()
{
    //get the data sets as concrete data types
    GeoGrid*       ggDest = dynamic_cast<GeoGrid*>      ( m_dfDestination );
    CartesianGrid* cgOrig = dynamic_cast<CartesianGrid*>( m_atOrigin->getContainingFile() );

    //load everything from the filesystem
    cgOrig->loadData();
    ggDest->loadData();
    ggDest->loadMesh();

    //get some data information
    uint rowCount = ggDest->getDataLineCount();
    uint atIndex = m_atOrigin->getAttributeGEOEASgivenIndex()-1;
    double NDVofDest = ggDest->getNoDataValueAsDouble();

    //create a vector to hold the collocated values
    std::vector< double > collocatedValues;
    collocatedValues.reserve( rowCount );

    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Transfering collocated values...");
    progressDialog.setMinimum( 0 );
    progressDialog.setValue( 0 );
    progressDialog.setMaximum( rowCount );
    /////////////////////////////////

    int progressUpdateStep = rowCount / 100;

    //loop over the GeoGrid cells (one cell == one data record)
    for( int iRow = 0; iRow < rowCount; ++iRow ){
        uint i, j, k;
        double x, y, z;
        ggDest->indexToIJK( iRow, i, j, k );
        ggDest->IJKtoXYZ( i, j, k, x, y, z );
        double collocatedValue = cgOrig->valueAt( atIndex, x, y, z );
        if( std::isfinite( collocatedValue ) && ! cgOrig->isNDV( collocatedValue ) )
            collocatedValues.push_back( collocatedValue );
        else
            collocatedValues.push_back( NDVofDest );

        if( ! ( iRow % progressUpdateStep ) ){
            progressDialog.setValue( iRow );
            QApplication::processEvents();
        }
    }

    //if the original variable is categorical, obtain the category definition object
    //so it stays so in the destination data set.
    CategoryDefinition* cd = nullptr; //a null category definition means that the variable is continuous.
    if( cgOrig->isCategorical( m_atOrigin ) )
        cd = cgOrig->getCategoryDefinition( m_atOrigin );

    //adds the collocated values a new attribute to the destination data file
    ggDest->addNewDataColumn( m_newAttributeName, collocatedValues, cd );

    return true;
}

bool ValuesTransferer::transferFromCGtoPS()
{
    //get the data sets as concrete data types
    PointSet*      psDest = dynamic_cast<PointSet*>     ( m_dfDestination );
    CartesianGrid* cgOrig = dynamic_cast<CartesianGrid*>( m_atOrigin->getContainingFile() );

    //load everything from the filesystem
    cgOrig->loadData();
    psDest->loadData();

    //get some data information
    uint rowCount = psDest->getDataLineCount();
    uint atIndex = m_atOrigin->getAttributeGEOEASgivenIndex()-1;
    double NDVofDest = psDest->getNoDataValueAsDouble();

    //create a vector to hold the collocated values
    std::vector< double > collocatedValues;
    collocatedValues.reserve( rowCount );

    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Transfering collocated values...");
    progressDialog.setMinimum( 0 );
    progressDialog.setValue( 0 );
    progressDialog.setMaximum( rowCount );
    /////////////////////////////////

    int progressUpdateStep = rowCount / 100;
    if( progressUpdateStep <= 0 )
        progressUpdateStep = 1;

    //loop over the PointSet samples (one sample == one data record)
    //to transfer values
    for( int iRow = 0; iRow < rowCount; ++iRow ){
        double x = psDest->data( iRow, psDest->getXindex()-1 );
        double y = psDest->data( iRow, psDest->getYindex()-1 );
        double z = 0.0;
        if( psDest->is3D() )
            z = psDest->data( iRow, psDest->getZindex()-1 );

        double collocatedValue = cgOrig->valueAt( atIndex, x, y, z );
        if( std::isfinite( collocatedValue ) && ! cgOrig->isNDV( collocatedValue ) )
            collocatedValues.push_back( collocatedValue );
        else
            collocatedValues.push_back( NDVofDest );

        if( ! ( iRow % progressUpdateStep ) ){
            progressDialog.setValue( iRow );
            QApplication::processEvents();
        }
    }

    //if the original variable is categorical, obtain the category definition object
    //so it stays so in the destination data set.
    CategoryDefinition* cd = nullptr; //a null category definition means that the variable is continuous.
    if( cgOrig->isCategorical( m_atOrigin ) )
        cd = cgOrig->getCategoryDefinition( m_atOrigin );

    //adds the collocated values a new attribute to the destination data file
    psDest->addNewDataColumn( m_newAttributeName, collocatedValues, cd );

    return true;
}

bool ValuesTransferer::transferFromCGtoCG()
{
    //get the data sets as concrete data types
    CartesianGrid* cgDest = dynamic_cast<CartesianGrid*>( m_dfDestination );
    CartesianGrid* cgOrig = dynamic_cast<CartesianGrid*>( m_atOrigin->getContainingFile() );

    //load everything from the filesystem
    cgOrig->loadData();
    cgDest->loadData();

    //get some data information
    uint rowCount = cgDest->getDataLineCount();
    uint atIndex = m_atOrigin->getAttributeGEOEASgivenIndex()-1;
    double NDVofDest = cgDest->getNoDataValueAsDouble();

    //create a vector to hold the collocated values
    std::vector< double > collocatedValues;
    collocatedValues.reserve( rowCount );

    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Transfering collocated values...");
    progressDialog.setMinimum( 0 );
    progressDialog.setValue( 0 );
    progressDialog.setMaximum( rowCount );
    /////////////////////////////////

    int progressUpdateStep = rowCount / 100;

    //loop over the destination Cartesian grid cells
    //to transfer values
    for( int iRow = 0; iRow < rowCount; ++iRow ){
        uint i, j, k;
        cgDest->indexToIJK( iRow, i, j, k );
        double x, y, z;
        cgDest->IJKtoXYZ( i, j, k, x, y, z );

        double collocatedValue = cgOrig->valueAt( atIndex, x, y, z );
        if( std::isfinite( collocatedValue ) && ! cgOrig->isNDV( collocatedValue ) )
            collocatedValues.push_back( collocatedValue );
        else
            collocatedValues.push_back( NDVofDest );

        if( ! ( iRow % progressUpdateStep ) ){
            progressDialog.setValue( iRow );
            QApplication::processEvents();
        }
    }

    //if the original variable is categorical, obtain the category definition object
    //so it stays so in the destination data set.
    CategoryDefinition* cd = nullptr; //a null category definition means that the variable is continuous.
    if( cgOrig->isCategorical( m_atOrigin ) )
        cd = cgOrig->getCategoryDefinition( m_atOrigin );

    //adds the collocated values a new attribute to the destination data file
    cgDest->addNewDataColumn( m_newAttributeName, collocatedValues, cd );

    return true;
}

bool ValuesTransferer::transferFromCGtoSection()
{
    //get the data sets as concrete data types
    CartesianGrid* cgDest = dynamic_cast<CartesianGrid*>( m_dfDestination );
    Section*  sectionDest = dynamic_cast<Section*>      ( m_dfDestination->getParent() );
    CartesianGrid* cgOrig = dynamic_cast<CartesianGrid*>( m_atOrigin->getContainingFile() );

    //load everything from the filesystem
    cgOrig->loadData();
    cgDest->loadData();
    sectionDest->readFromFS();

    //get some data information
    uint rowCount = cgDest->getDataLineCount();
    uint atIndex = m_atOrigin->getAttributeGEOEASgivenIndex()-1;
    double NDVofDest = cgDest->getNoDataValueAsDouble();

    //create a vector to hold the collocated values
    std::vector< double > collocatedValues;
    collocatedValues.reserve( rowCount );

    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Transfering collocated values...");
    progressDialog.setMinimum( 0 );
    progressDialog.setValue( 0 );
    progressDialog.setMaximum( rowCount );
    /////////////////////////////////

    int progressUpdateStep = rowCount / 100;

    //loop over the Section's cells (one cell == one data record)
    for( int iRow = 0; iRow < rowCount; ++iRow ){
        uint i, j, k;
        double x, y, z;
        // convert the run-length section cell index to the IJK grid address.
        cgDest->indexToIJK( iRow, i, j, k );
        // get the location of the section's cell in space (XYZ).
        sectionDest->IKtoXYZ( i, k, x, y, z );
        // get the value of the origin data set at the XYZ location (collocated value).
        double collocatedValue = cgOrig->valueAt( atIndex, x, y, z );
        // set the value or set null value if the orginal value is invalid or if the XYZ
        // location is outside the original data set.
        if( std::isfinite( collocatedValue ) && ! cgOrig->isNDV( collocatedValue ) )
            collocatedValues.push_back( collocatedValue );
        else
            collocatedValues.push_back( NDVofDest );
        //update the progress bar.
        if( ! ( iRow % progressUpdateStep ) ){
            progressDialog.setValue( iRow );
            QApplication::processEvents();
        }
    }

    //if the original variable is categorical, obtain the category definition object
    //so it stays so in the destination data set.
    CategoryDefinition* cd = nullptr; //a null category definition means that the variable is continuous.
    if( cgOrig->isCategorical( m_atOrigin ) )
        cd = cgOrig->getCategoryDefinition( m_atOrigin );

    //adds the collocated values as a new attribute to the destination data file.
    cgDest->addNewDataColumn( m_newAttributeName, collocatedValues, cd );

    return true;
}

bool ValuesTransferer::transferFromPStoCG()
{
    //get the data sets as concrete data types
    CartesianGrid* cgDest = dynamic_cast<CartesianGrid*>( m_dfDestination );
    PointSet*      psOrig = dynamic_cast<PointSet*>( m_atOrigin->getContainingFile() );

    //load everything from the filesystem
    psOrig->loadData();
    cgDest->loadData();

    //get some data information
    uint dataCount = psOrig->getDataLineCount();
    uint atIndex = m_atOrigin->getAttributeGEOEASgivenIndex()-1;
    double NDVofDest = cgDest->getNoDataValueAsDouble();

    //create a vector to hold the collocated values all filled with the destination file's no-data value.
    std::vector< double > valuesForDestCG( cgDest->getDataLineCount(), NDVofDest );

    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Transfering collocated values...");
    progressDialog.setMinimum( 0 );
    progressDialog.setValue( 0 );
    progressDialog.setMaximum( dataCount );
    /////////////////////////////////

    int progressUpdateStep = dataCount / 100;

    //loop over the point set samples
    //to transfer values
    for( int iRow = 0; iRow < dataCount; ++iRow ){

        //get location of sample
        double x, y, z;
        {
            int i, j, k; //not used, required by PointSet::getSpatialAndTopologicalCoordinates()
            psOrig->getSpatialAndTopologicalCoordinates( iRow, x, y, z, i, j, k );
        }

        //get the run-length address of the destination grid's collocated cell
        int runLengthIndex = -1;
        {
            uint i, j, k; //not used, required by PointSet::getSpatialAndTopologicalCoordinates()
            bool isInside = cgDest->XYZtoIJK( x, y, z, i, j, k );
            if( ! isInside )
                continue; //abort current iteration if the sample fell outside the grid
            runLengthIndex = cgDest->IJKtoIndex( i, j, k );
        }

        //get the sample value
        double collocatedValue = psOrig->dataConst( iRow, atIndex );

        //set the collocated value to the data soon to be added to the destination grid
        if( std::isfinite( collocatedValue ) && ! psOrig->isNDV( collocatedValue ) )
            valuesForDestCG[ runLengthIndex ] = collocatedValue ;
        else
            valuesForDestCG[ runLengthIndex ] = NDVofDest ;

        //update progress
        if( progressUpdateStep > 0 && ! ( iRow % progressUpdateStep ) ){
            progressDialog.setValue( iRow );
            QApplication::processEvents();
        }
    }

    //if the original variable is categorical, obtain the category definition object
    //so it stays so in the destination data set.
    CategoryDefinition* cd = nullptr; //a null category definition means that the variable is continuous.
    if( psOrig->isCategorical( m_atOrigin ) )
        cd = psOrig->getCategoryDefinition( m_atOrigin );

    //adds the collocated values a new attribute to the destination data file
    cgDest->addNewDataColumn( m_newAttributeName, valuesForDestCG, cd );

    return true;
}
