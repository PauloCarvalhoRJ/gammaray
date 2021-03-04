#include "section.h"

#include <QFile>
#include <QTextStream>
#include <cassert>

#include "domain/pointset.h"
#include "domain/cartesiangrid.h"
#include "domain/application.h"
#include "domain/project.h"
#include "viewer3d/view3dviewdata.h"
#include "viewer3d/view3dbuilders.h"

Section::Section(QString path) : File(path),
    m_PointSet( nullptr ),
    m_CartesianGrid( nullptr )
{
}

void Section::setPointSet(PointSet *pointSet)
{
    if( m_PointSet ){
        removeChild( m_PointSet );
        delete m_PointSet;
    }
    m_PointSet = pointSet;
    addChild( pointSet );
    pointSet->setParent( this );
}

void Section::setCartesianGrid(CartesianGrid *cartesianGrid)
{
    if( m_CartesianGrid ){
        removeChild( m_CartesianGrid ); //TODO: verify memory leak
        delete m_CartesianGrid;
    }
    m_CartesianGrid = cartesianGrid;
    addChild( cartesianGrid );
    cartesianGrid->setParent( this );
}

void Section::setInfoFromMetadataFile()
{
    QString md_file_path( this->_path );
    QFile md_file( md_file_path.append(".md") );
    if( md_file.exists() ){
        md_file.open( QFile::ReadOnly | QFile::Text );
        QTextStream in(&md_file);
        for (int i = 0; !in.atEnd(); ++i)
        {
           QString line = in.readLine();
           if( line.startsWith( "POINTSET:" ) ){
               QString pointSetName = line.split(":")[1];
               //The point set component must always be a sibling file.
               QString pointSetPath = Util::getParentDirectory( this->getPath() ) +
                                      '/' + pointSetName;
               if( !pointSetName.isEmpty() && Util::fileExists( pointSetPath ) ) {
                   PointSet* componentPointSet = new PointSet( pointSetPath );
                   componentPointSet->setInfoFromMetadataFile();
                   setPointSet( componentPointSet );
               }
           }else if( line.startsWith( "CARTESIANGRID:" ) ){
               QString cartesianGridName = line.split(":")[1];
               //The Cartesian grid component must always be a sibling file.
               QString cartesianGridPath = Util::getParentDirectory( this->getPath() )  +
                                      '/' + cartesianGridName;
               if( !cartesianGridName.isEmpty() && Util::fileExists( cartesianGridPath ) ) {
                   CartesianGrid* componentCartesianGrid = new CartesianGrid( cartesianGridPath );
                   componentCartesianGrid->setInfoFromMetadataFile();
                   setCartesianGrid( componentCartesianGrid );
               }
           }
        }
        md_file.close();
    }
}

void Section::IKtoXYZ(uint i, uint k, double &x, double &y, double &z) const
{
    //Get the cell's geometry.
    Quadrilateral quad = makeQuadrilateral( i, k );

    //Compute the cell center.
    x = ( quad.v[0].x + quad.v[1].x + quad.v[2].x + quad.v[3].x ) / 4.0;
    y = ( quad.v[0].y + quad.v[1].y + quad.v[2].y + quad.v[3].y ) / 4.0;
    z = ( quad.v[0].z + quad.v[1].z + quad.v[2].z + quad.v[3].z ) / 4.0;
}

PointSet *Section::toPointSetCentroids(const QString &psName) const
{
    //Get the section's data.
    CartesianGrid* sectionDataCG = getCartesianGrid();

    //sanity checks
    assert( sectionDataCG->getDataLineCount() && "Section::toPointSetCentroids(): Cartesian grid with "
                                                 "section data not loaded. Maybe a prior call to "
                                                 "DataFile::readFromFS() is missing." );

    int nDataRows = sectionDataCG->getDataLineCount();
    int nDataColumns = sectionDataCG->getDataColumnCountConst();
    assert( nDataRows && "Section::toPointSetCentroids(): zero data lines. "
                         "Perhaps a prior call to DataFile::readFromFS() is missing.");

    //copies this segment set's file as a new file
    QString psFilePath = Application::instance()->getProject()->getPath() + "/" + psName;
    Util::copyFile( sectionDataCG->getPath(), psFilePath );
    PointSet* new_ps = new PointSet( psFilePath );

    //load the data
    new_ps->loadData();
    new_ps->updateChildObjectsCollection();

    //append the new data columns for the mid points coordinates and segment lengths
    int iColumnCx        = new_ps->addEmptyDataColumn( "centroidX", nDataRows );
    int iColumnCy        = new_ps->addEmptyDataColumn( "centroidY", nDataRows );
    int iColumnCz        = new_ps->addEmptyDataColumn( "centroidZ", nDataRows );
    int iColumnCellAreas = new_ps->addEmptyDataColumn( "cell_area", nDataRows );

    //compute mid points for the PointSet object
    int nI = sectionDataCG->getNI();
    int nK = sectionDataCG->getNK();
    for( int k = 0; k < nK; ++k ){
        for( int i = 0; i < nI; ++i ){
            double center_x;
            double center_y;
            double center_z;

            IKtoXYZ( i, k, center_x, center_y, center_z );

            int iDataLine = k * nI + i;
            new_ps->setData( iDataLine, iColumnCx, center_x );
            new_ps->setData( iDataLine, iColumnCy, center_y );
            new_ps->setData( iDataLine, iColumnCz, center_z );
            //compute the cellsize
            new_ps->setData( iDataLine, iColumnCellAreas, getCellArea( i, k ) );
        }
    }

    //commit data to file system
    new_ps->writeToFS();

    //set appropriate metadata
    new_ps->setInfo( iColumnCx+1, //these indexes are GEO-EAS indexes (1st == 1)
                     iColumnCy+1,
                     iColumnCz+1,
                     sectionDataCG->getNoDataValue(),
                     QMap<uint, uint>(),
                     sectionDataCG->getNSVarVarTrnTriads(),
                     sectionDataCG->getCategoricalAttributes() );
    new_ps->updateMetaDataFile();

    //return the pointer to the created object
    return new_ps;
}

double Section::getCellArea(uint i, uint k) const
{
    Quadrilateral quad = makeQuadrilateral( i, k );
    return quad.getArea();
}

Quadrilateral Section::makeQuadrilateral(uint i, uint k) const
{
    //Get the section's component data files.
    PointSet* sectionPathPS = getPointSet();
    CartesianGrid* sectionDataCG = getCartesianGrid();

    //sanity checks
    assert( sectionPathPS->getDataLineCount() && "Section::makeQuadrilateral(): Pointset with section path not loaded. Maybe a prior call to DataFile::readFromFS() is missing." );
    assert( sectionDataCG->getDataLineCount() && "Section::makeQuadrilateral(): Cartesian grid with section data not loaded. Maybe a prior call to DataFile::readFromFS() is missing." );

    //Get some info of the component data files.
    int nSegments = sectionPathPS->getDataLineCount()-1; //if the PS has two entries, it defines one section segment.
    int nRows = sectionDataCG->getNK(); //number of rows of a section is the number of layers of its data grid.
    int nHorizons = nRows + 1;

    //determine the section segment where the desired column is.
    int iSegment, dataColIni, dataColFin;
    for( iSegment = 0;  iSegment < nSegments; ++iSegment){
        dataColIni = sectionPathPS->data( iSegment,   4 ); //data grid column is 5th variable per the section format
        dataColFin = sectionPathPS->data( iSegment+1, 4 ) - 1; //data grid column is 5th variable per the section format
        if( iSegment + 1 == nSegments ) //the last segment's final column is not the first column of the next segment (it can't be!)
            ++dataColFin;
        if( i >= dataColIni && i <= dataColFin ){
            ++iSegment;
            break;
        }
    }
    --iSegment; //for's last iteration cause an aditional increment
    int nDataColumnsOfSegment = dataColFin - dataColIni + 1;

    //Get geometry of the entire section segment there the cell is.
    //See documentation of the Section class for point set convention regarding variable order.
    double segment_Xi    = sectionPathPS->data( iSegment,   0 ); //X is 1st variable per the section format
    double segment_Yi    = sectionPathPS->data( iSegment,   1 ); //Y is 2nd variable per the section format
    double segment_topZi = sectionPathPS->data( iSegment,   2 ); //Z1 is 3rd variable per the section format
    double segment_botZi = sectionPathPS->data( iSegment,   3 ); //Z2 is 4th variable per the section format
    double segment_Xf    = sectionPathPS->data( iSegment+1, 0 );
    double segment_Yf    = sectionPathPS->data( iSegment+1, 1 );
    double segment_topZf = sectionPathPS->data( iSegment+1, 2 );
    double segment_botZf = sectionPathPS->data( iSegment+1, 3 );

    //The deltas of X,Y coordinate are simple because the geologic section is vertical.
    double segmentDeltaX = segment_Xf - segment_Xi;
    double segmentDeltaY = segment_Yf - segment_Yi;

    //The delta z is a bit more complicated because depth can vary along the section.
    //Furthermore, we have a bottom and a top z in the head and tail of the section segment.
    double segmentHeadDeltaZ  = segment_topZi     - segment_botZi;
    double segmentTailDeltaZ  = segment_topZf     - segment_botZf;
    double segmentHeadZlower  = segment_botZi     + segmentHeadDeltaZ / nHorizons * k;
    double segmentTailZlower  = segment_botZf     + segmentTailDeltaZ / nHorizons * k;
    double segmentDeltaZlower = segmentTailZlower - segmentHeadZlower;
    double segmentHeadZupper  = segment_botZi     + segmentHeadDeltaZ / nHorizons * ( k + 1 );
    double segmentTailZupper  = segment_botZf     + segmentTailDeltaZ / nHorizons * ( k + 1 );
    double segmentDeltaZupper = segmentTailZupper - segmentHeadZupper;

    //Compute the vertexes that define the cell's geometry.
    int columnOffset = i - dataColIni;
    double x1 = segment_Xi        + segmentDeltaX      / nDataColumnsOfSegment * columnOffset;
    double y1 = segment_Yi        + segmentDeltaY      / nDataColumnsOfSegment * columnOffset;
    double z1 = segmentHeadZlower + segmentDeltaZlower / nDataColumnsOfSegment * columnOffset;
    double x2 = segment_Xi        + segmentDeltaX      / nDataColumnsOfSegment * ( columnOffset + 1 );
    double y2 = segment_Yi        + segmentDeltaY      / nDataColumnsOfSegment * ( columnOffset + 1 );
    double z2 = segmentHeadZlower + segmentDeltaZlower / nDataColumnsOfSegment * ( columnOffset + 1 );
    double x3 = x2;
    double y3 = y2;
    double z3 = segmentHeadZupper + segmentDeltaZupper / nDataColumnsOfSegment * ( columnOffset + 1 );
    double x4 = x1;
    double y4 = y1;
    double z4 = segmentHeadZupper + segmentDeltaZupper / nDataColumnsOfSegment * columnOffset;

    //Create and return the Quadrilateral object.
    Quadrilateral quad;
    quad.v[0].x = x1;
    quad.v[0].y = y1;
    quad.v[0].z = z1;
    quad.v[1].x = x2;
    quad.v[1].y = y2;
    quad.v[1].z = z2;
    quad.v[2].x = x3;
    quad.v[2].y = y3;
    quad.v[2].z = z3;
    quad.v[3].x = x4;
    quad.v[3].y = y4;
    quad.v[3].z = z4;
    return quad;
}

void Section::deleteFromFS()
{
    File::deleteFromFS(); //delete data file

    // also deletes the metadata file
    QFile file(this->getMetaDataFilePath());
    file.remove(); // TODO: throw exception if remove() returns false (fails).  Also see
                   // QIODevice::errorString() to see error message.

    //Sections are defined by two more files: a PointSet and a CartesianGrid.
    if( m_CartesianGrid )
        m_CartesianGrid->deleteFromFS();
    if( m_PointSet )
        m_PointSet->deleteFromFS();
}

void Section::writeToFS()
{
    //A Section does not actually have any data.  Its data are in the two component objects.
    if( m_CartesianGrid )
        m_CartesianGrid->writeToFS();
    if( m_PointSet )
        m_PointSet->writeToFS();
}

void Section::readFromFS()
{
    //A Section does not actually have any data.  Its data are in the two component objects.
    if( m_CartesianGrid )
        m_CartesianGrid->readFromFS();
    if( m_PointSet )
        m_PointSet->readFromFS();
}

void Section::updateMetaDataFile()
{
    QFile file( this->getMetaDataFilePath() );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);
    out << APP_NAME << " metadata file.  This file is generated automatically.  Do not edit this file.\n";
    out << "version=" << APP_VERSION << '\n';
    out << "POINTSET:" << (this->m_PointSet ? this->m_PointSet->getName() : "") << '\n';
    out << "CARTESIANGRID:" << (this->m_CartesianGrid ? this->m_CartesianGrid->getName() : "") << '\n';
    file.close();
    //Also updates the metadata file of its two component objects.
    if( m_CartesianGrid )
        m_CartesianGrid->updateMetaDataFile();
    if( m_PointSet )
        m_PointSet->updateMetaDataFile();
}

QIcon Section::getIcon()
{
    return QIcon(":icons32/section32");
}

void Section::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
    //also saves the metadata file.
    this->updateMetaDataFile();
}

View3DViewData Section::build3DViewObjects(View3DWidget *widget3D)
{
    return View3DBuilders::build( this, widget3D );
}
