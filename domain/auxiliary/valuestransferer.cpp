#include "valuestransferer.h"

#include "domain/attribute.h"
#include "domain/datafile.h"
#include "domain/application.h"
#include "domain/geogrid.h"
#include "domain/cartesiangrid.h"
#include "domain/pointset.h"

#include <QProgressDialog>
#include <QApplication>

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
        else
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

    //adds the collocated values a new attribute to the destination data file
    ggDest->addNewDataColumn( m_newAttributeName, collocatedValues );

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

    //loop over the PointSet samples (one sample == one data record)
    //to transfer values
    for( int iRow = 0; iRow < rowCount; ++iRow ){
        double x = psDest->data( iRow, psDest->getXindex()-1 );
        double y = psDest->data( iRow, psDest->getYindex()-1 );
        double z = psDest->data( iRow, psDest->getZindex()-1 );

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

    //adds the collocated values a new attribute to the destination data file
    psDest->addNewDataColumn( m_newAttributeName, collocatedValues );

    return true;
}
