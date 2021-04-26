#include "segmentset.h"
#include "viewer3d/view3dviewdata.h"
#include "viewer3d/view3dbuilders.h"
#include "domain/attribute.h"
#include "domain/application.h"
#include "domain/project.h"
#include "util.h"
#include <QFile>
#include <QTextStream>
#include <cassert>

SegmentSet::SegmentSet(QString path) : PointSet ( path )
{
    _x_final_field_index = 0;
    _y_final_field_index = 0;
    _z_final_field_index = 0;
}

void SegmentSet::setInfo(int x_intial_index, int y_intial_index, int z_intial_index,
                         int x_final_index,  int y_final_index,  int z_final_index,
                         const QString no_data_value,
                         const QMap<uint, uint> &wgt_var_pairs,
                         const QMap<uint, QPair<uint, QString> > &nvar_var_trn_triads,
                         const QList<QPair<uint, QString> > &categorical_attributes)
{
    //updates metadata
    this->_x_final_field_index = x_final_index;
    this->_y_final_field_index = y_final_index;
    this->_z_final_field_index = z_final_index;
    PointSet::setInfo( x_intial_index, y_intial_index, z_intial_index, no_data_value,
                       wgt_var_pairs, nvar_var_trn_triads, categorical_attributes );
}

void SegmentSet::setInfo(int x_intial_index, int y_intial_index, int z_intial_index,
                         int x_final_index, int y_final_index, int z_final_index,
                         const QString no_data_value)
{
    //updates metadata
    this->_x_final_field_index = x_final_index;
    this->_y_final_field_index = y_final_index;
    this->_z_final_field_index = z_final_index;
    PointSet::setInfo( x_intial_index, y_intial_index, z_intial_index, no_data_value );
}

void SegmentSet::setInfoFromMetadataFile()
{
    QString md_file_path( this->_path );
    QFile md_file( md_file_path.append(".md") );
    int x_initial_index = 0, y_initial_index = 0, z_initial_index = 0;
    int x_final_index = 0, y_final_index = 0, z_final_index = 0;
    QMap<uint, uint> wgt_var_pairs;
    QMap<uint, QPair<uint,QString> > nsvar_var_trn;
    QList< QPair<uint,QString> > categorical_attributes;
    QString ndv;
    if( md_file.exists() ){
        md_file.open( QFile::ReadOnly | QFile::Text );
        QTextStream in(&md_file);
        for (int i = 0; !in.atEnd(); ++i)
        {
           QString line = in.readLine();
           if( line.startsWith( "Xinitial:" ) ){
               QString value = line.split(":")[1];
               x_initial_index = value.toInt();
           }else if( line.startsWith( "Yinitial:" ) ){
               QString value = line.split(":")[1];
               y_initial_index = value.toInt();
           }else if( line.startsWith( "Zinitial:" ) ){
               QString value = line.split(":")[1];
               z_initial_index = value.toInt();
           }else if( line.startsWith( "Xfinal:" ) ){
               QString value = line.split(":")[1];
               x_final_index = value.toInt();
           }else if( line.startsWith( "Yfinal:" ) ){
               QString value = line.split(":")[1];
               y_final_index = value.toInt();
           }else if( line.startsWith( "Zfinal:" ) ){
               QString value = line.split(":")[1];
               z_final_index = value.toInt();
           }else if( line.startsWith( "NDV:" ) ){
               QString value = line.split(":")[1];
               ndv = value;
           }else if( line.startsWith( "WEIGHT:" ) ){
               QString pair = line.split(":")[1];
               QString var = pair.split(">")[0];
               QString wgt = pair.split(">")[1];
               //weight index is key
               //variable index is value
               wgt_var_pairs.insert( wgt.toUInt(), var.toUInt() );
           }else if( line.startsWith( "NSCORE:" ) ){
               QString triad = line.split(":")[1];
               QString var = triad.split(">")[0];
               QString pair = triad.split(">")[1];
               QString ns_var = pair.split("=")[0];
               QString trn_filename = pair.split("=")[1];
               //normal variable index is key
               //variable index and transform table filename are the value
               nsvar_var_trn.insert( ns_var.toUInt(), QPair<uint,QString>(var.toUInt(), trn_filename ));
           }else if( line.startsWith( "CATEGORICAL:" ) ){
               QString var_and_catDefName = line.split(":")[1];
               QString var = var_and_catDefName.split(",")[0];
               QString catDefName = var_and_catDefName.split(",")[1];
               categorical_attributes.append( QPair<uint,QString>( var.toUInt(), catDefName ) );
           }
        }
        md_file.close();
        setInfo( x_initial_index, y_initial_index, z_initial_index,
                 x_final_index, y_final_index, z_final_index,
                 ndv, wgt_var_pairs, nsvar_var_trn, categorical_attributes );
    }
}

void SegmentSet::setInfoFromAnotherSegmentSet(SegmentSet *otherSS)
{
    QString md_file_path( this->_path );
    QFile md_file( md_file_path.append(".md") );
    int x_initial_index = 0, y_initial_index = 0, z_initial_index = 0;
    int x_final_index = 0, y_final_index = 0, z_final_index = 0;
    QMap<uint, uint> wgt_var_pairs;
    QMap<uint, QPair<uint,QString> > nsvar_var_trn;
    QList< QPair<uint,QString> > categorical_attributes;
    QString ndv;

    x_initial_index = otherSS->getXindex();
    y_initial_index = otherSS->getYindex();
    z_initial_index = otherSS->getZindex();
    x_final_index = otherSS->getXFinalIndex(); //the only metadata not already present in PointSet
    y_final_index = otherSS->getYFinalIndex(); //the only metadata not already present in PointSet
    z_final_index = otherSS->getZFinalIndex(); //the only metadata not already present in PointSet
    ndv = otherSS->getNoDataValue();
    wgt_var_pairs = otherSS->getWeightsVariablesPairs();
    nsvar_var_trn = otherSS->getNSVarVarTrnTriads();
    categorical_attributes = otherSS->getCategoricalAttributes();

    setInfo( x_initial_index, y_initial_index, z_initial_index,
             x_final_index,   y_final_index,   z_final_index,
             ndv, wgt_var_pairs, nsvar_var_trn, categorical_attributes );
}

int SegmentSet::getXFinalIndex() const
{
    return _x_final_field_index;
}

int SegmentSet::getYFinalIndex() const
{
    return _y_final_field_index;
}

int SegmentSet::getZFinalIndex() const
{
    return _z_final_field_index;
}

double SegmentSet::getSegmentLenght(int iRecord)
{
    double dx = data( iRecord, getXFinalIndex()-1 ) - data( iRecord, getXindex()-1 );
    double dy = data( iRecord, getYFinalIndex()-1 ) - data( iRecord, getYindex()-1 );
    double dz = data( iRecord, getZFinalIndex()-1 ) - data( iRecord, getZindex()-1 );
    return std::sqrt( dx*dx + dy*dy + dz*dz );
}

double SegmentSet::getSegmentLenghtConst(int iRecord) const
{
    double dx = dataConst( iRecord, getXFinalIndex()-1 ) - dataConst( iRecord, getXindex()-1 );
    double dy = dataConst( iRecord, getYFinalIndex()-1 ) - dataConst( iRecord, getYindex()-1 );
    double dz = dataConst( iRecord, getZFinalIndex()-1 ) - dataConst( iRecord, getZindex()-1 );
    return std::sqrt( dx*dx + dy*dy + dz*dz );
}

double SegmentSet::getDistanceToNextSegment(int iRecord)
{
    if( iRecord == getDataLineCount() - 1 )
        return 0.0;
    double dx = data( iRecord+1, getXindex()-1 ) - data( iRecord, getXFinalIndex()-1 );
    double dy = data( iRecord+1, getYindex()-1 ) - data( iRecord, getYFinalIndex()-1 );
    double dz = data( iRecord+1, getZindex()-1 ) - data( iRecord, getZFinalIndex()-1 );
    return std::sqrt( dx*dx + dy*dy + dz*dz );
}

double SegmentSet::getDistanceToNextSegmentConst(int iRecord) const
{
    if( iRecord == getDataLineCount() - 1 )
        return 0.0;
    double dx = dataConst( iRecord+1, getXindex()-1 ) - dataConst( iRecord, getXFinalIndex()-1 );
    double dy = dataConst( iRecord+1, getYindex()-1 ) - dataConst( iRecord, getYFinalIndex()-1 );
    double dz = dataConst( iRecord+1, getZindex()-1 ) - dataConst( iRecord, getZFinalIndex()-1 );
    return std::sqrt( dx*dx + dy*dy + dz*dz );
}

void SegmentSet::computeSegmentLenghts(QString variable_name)
{
    //load the data
    loadData();

    //appends a new variable
    addEmptyDataColumn( variable_name, getDataLineCount() );

    //get the current data column count
    uint columnCount = getDataColumnCount();

    //compute the lengths
    uint rowCount = getDataLineCount();
    for( int iRow = 0; iRow < rowCount; ++iRow )
        setData( iRow, columnCount-1, getSegmentLenght( iRow ) );

    //commit results to file system
    writeToFS();
}

void SegmentSet::getBoundingBox(uint dataLineIndex, double &minX, double &minY, double &minZ, double &maxX, double &maxY, double &maxZ) const
{
    //initialize the results to ensure the returned extrema are those of the segment.
    minX = minY = minZ = std::numeric_limits<double>::max();
    maxX = maxY = maxZ = -std::numeric_limits<double>::max();
    //set the max's and min's
    minX = std::min( minX, dataConst( dataLineIndex, getXindex()-1 ) );
    minY = std::min( minY, dataConst( dataLineIndex, getYindex()-1 ) );
    minZ = std::min( minZ, dataConst( dataLineIndex, getZindex()-1 ) );
    minX = std::min( minX, dataConst( dataLineIndex, getXFinalIndex()-1 ) );
    minY = std::min( minY, dataConst( dataLineIndex, getYFinalIndex()-1 ) );
    minZ = std::min( minZ, dataConst( dataLineIndex, getZFinalIndex()-1 ) );
    maxX = std::max( maxX, dataConst( dataLineIndex, getXindex()-1 ) );
    maxY = std::max( maxY, dataConst( dataLineIndex, getYindex()-1 ) );
    maxZ = std::max( maxZ, dataConst( dataLineIndex, getZindex()-1 ) );
    maxX = std::max( maxX, dataConst( dataLineIndex, getXFinalIndex()-1 ) );
    maxY = std::max( maxY, dataConst( dataLineIndex, getYFinalIndex()-1 ) );
    maxZ = std::max( maxZ, dataConst( dataLineIndex, getZFinalIndex()-1 ) );
}

bool SegmentSet::isCoordinate(uint column) const
{
    //tests whether the column is x, y or z initial (inherited from PointSet)
    if( PointSet::isCoordinate( column ) )
        return true;
    //tests whether the column is x, y or z final
    int columnGEOEAS = column + 1;
    return ( _x_final_field_index == columnGEOEAS ) ||
           ( _y_final_field_index == columnGEOEAS ) ||
           ( _z_final_field_index == columnGEOEAS ) ;
}

PointSet *SegmentSet::toPointSetMidPoints(const QString& psName ) const
{
    int nDataRows = getDataLineCount();
    int nDataColumns = getDataColumnCountConst();
    assert( nDataRows && "SegmentSet::toPointSetMidPoints(): zero data lines. "
                         "Perhaps a prior call to DataFile::readFromFS() is missing.");

    //copies this segment set's file as a new file
    QString psFilePath = Application::instance()->getProject()->getPath() + "/" + psName;
    Util::copyFile( getPath(), psFilePath );
    PointSet* new_ps = new PointSet( psFilePath );

    //load the data
    new_ps->loadData();
    new_ps->updateChildObjectsCollection();

    //append the new data columns for the mid points coordinates and segment lengths
    int iColumnMPx        = new_ps->addEmptyDataColumn( "midPointX"     , nDataRows );
    int iColumnMPy        = new_ps->addEmptyDataColumn( "midPointY"     , nDataRows );
    int iColumnMPz        = new_ps->addEmptyDataColumn( "midPointZ"     , nDataRows );
    int iColumnSegLengths = new_ps->addEmptyDataColumn( "segment_length", nDataRows );

    //compute mid points for the PointSet object
    for( int iRow = 0; iRow < nDataRows; ++iRow ){
        double center_x = ( dataConst( iRow, getXFinalIndex()-1 ) + dataConst( iRow, getXindex()-1 ) ) / 2.0;
        double center_y = ( dataConst( iRow, getYFinalIndex()-1 ) + dataConst( iRow, getYindex()-1 ) ) / 2.0;
        double center_z = ( dataConst( iRow, getZFinalIndex()-1 ) + dataConst( iRow, getZindex()-1 ) ) / 2.0;
        new_ps->setData( iRow, iColumnMPx, center_x );
        new_ps->setData( iRow, iColumnMPy, center_y );
        new_ps->setData( iRow, iColumnMPz, center_z );
        //compute the length
        new_ps->setData( iRow, iColumnSegLengths, getSegmentLenghtConst( iRow ) );
    }

    //commit data to file system
    new_ps->writeToFS();

    //set appropriate metadata
    new_ps->setInfo( iColumnMPx+1, //these indexes are GEO-EAS indexes (1st == 1)
                     iColumnMPy+1,
                     iColumnMPz+1,
                     getNoDataValue(),
                     getWeightsVariablesPairs(),
                     getNSVarVarTrnTriads(),
                     getCategoricalAttributes() );
    new_ps->updateMetaDataFile();

    //return the pointer to the created object
    return new_ps;
}

PointSet *SegmentSet::toPointSetRegularlySpaced(const QString &psName, double step) const
{
    int nDataRows = getDataLineCount();
    int nDataColumns = getDataColumnCountConst();
    assert( nDataRows && "SegmentSet::toPointSetRegularlySpaced(): zero data lines. "
                         "Perhaps a prior call to DataFile::readFromFS() is missing.");

    //copies this segment set's file as a new file
    QString psFilePath = Application::instance()->getProject()->getPath() + "/" + psName;
    Util::copyFile( getPath(), psFilePath );
    PointSet* new_ps = new PointSet( psFilePath );

    //load the data
    new_ps->loadData();
    new_ps->updateChildObjectsCollection();

    //append the new data columns for the regular points coordinates
    int iColumnRSx = new_ps->addEmptyDataColumn( "regSampleX", nDataRows );
    int iColumnRSy = new_ps->addEmptyDataColumn( "regSampleY", nDataRows );
    int iColumnRSz = new_ps->addEmptyDataColumn( "regSampleZ", nDataRows );

    //the output data set will likely have more entries than this input segment set.
    //so we must keep track of additional data records added to the output point set.
    uint iRowOutputOffset = 0;

    //compute regular points for the PointSet object
    for( int iRow = 0; iRow < nDataRows; ++iRow ){

        //get the segment's starting and ending coordinates
        double x0 = dataConst( iRow, getXindex()-1 );
        double y0 = dataConst( iRow, getYindex()-1 );
        double z0 = dataConst( iRow, getZindex()-1 );
        double x1 = dataConst( iRow, getXFinalIndex()-1 );
        double y1 = dataConst( iRow, getYFinalIndex()-1 );
        double z1 = dataConst( iRow, getZFinalIndex()-1 );

        //get the current segment length.
        double segment_length = getSegmentLenghtConst( iRow );

        //the portion of the segment length traversed.
        double processed_length = 0.0;

        //while the segment hasn't been completely traversed.
        for( uint iStep = 0; processed_length <= segment_length; ++iStep ){

            //linearly interpolates a XYZ coordinate between the extremes of the segment.
            double x = Util::linearInterpolation( processed_length, 0.0, segment_length, x0, x1 );
            double y = Util::linearInterpolation( processed_length, 0.0, segment_length, y0, y1 );
            double z = Util::linearInterpolation( processed_length, 0.0, segment_length, z0, z1 );

            //the first point is always written and corresponds to the starting vertex of the segment.
            //the segment may shorter than the user-given step.  The other points along the segment line
            //are clone entries of that first entry.
            if( iStep > 0 ){
                new_ps->cloneDataLine( iRow + iRowOutputOffset );
                ++iRowOutputOffset;
            }

            //outputs the interpolated XYZ coordinates.
            new_ps->setData( iRow + iRowOutputOffset, iColumnRSx, x );
            new_ps->setData( iRow + iRowOutputOffset, iColumnRSy, y );
            new_ps->setData( iRow + iRowOutputOffset, iColumnRSz, z );

            //increase the processed length by the user-given step.
            processed_length += step;
        }
    }

    //commit data to file system
    new_ps->writeToFS();

    //set appropriate metadata
    new_ps->setInfo( iColumnRSx+1, //these indexes are GEO-EAS indexes (1st == 1)
                     iColumnRSy+1,
                     iColumnRSz+1,
                     getNoDataValue(),
                     getWeightsVariablesPairs(),
                     getNSVarVarTrnTriads(),
                     getCategoricalAttributes() );
    new_ps->updateMetaDataFile();

    //return the pointer to the created object
    return new_ps;
}

SegmentSet* SegmentSet::createSegmentSetByFiltering(uint column, double vMin, double vMax)
{
    //Create new empty point set.
    SegmentSet* newSS = new SegmentSet( "" );
    //Get filtered data frame.
    std::vector< std::vector< double > > filteredData = getDataFilteredBy( column, vMin, vMax );
    //Assign it as the new point set's data.
    newSS->_data = filteredData;
    //Set the same metadata.
    newSS->setInfoFromAnotherSegmentSet( this );
    //Return the new filtered data set.
    return newSS;
}

double SegmentSet::getSegmentHeight(int iRecord) const
{
    double dz = dataConst( iRecord, getZFinalIndex()-1 ) - dataConst( iRecord, getZindex()-1 );
    return std::abs( dz );
}

void SegmentSet::getHeadLocation(double &x, double &y, double &z) const
{
    x = dataConst( 0, getXindex()-1 );
    y = dataConst( 0, getYindex()-1 );
    z = dataConst( 0, getZindex()-1 );
}

void SegmentSet::getTailLocation(double &x, double &y, double &z) const
{
    x = dataConst( getDataLineCount()-1, getXFinalIndex()-1 );
    y = dataConst( getDataLineCount()-1, getYFinalIndex()-1 );
    z = dataConst( getDataLineCount()-1, getZFinalIndex()-1 );
}


QIcon SegmentSet::getIcon()
{
    return QIcon(":icons32/segmentset32");
}

QString SegmentSet::getTypeName() const
{
    return getFileType();
}

void SegmentSet::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
    //also saves the metadata file.
    this->updateMetaDataFile();
}

View3DViewData SegmentSet::build3DViewObjects(View3DWidget *widget3D)
{
    return View3DBuilders::build( this, widget3D );
}

void SegmentSet::getSpatialAndTopologicalCoordinates(int iRecord, double &x, double &y, double &z, int &i, int &j, int &k)
{

    double xi = data( iRecord, getXindex() - 1 );
    double yi = data( iRecord, getYindex() - 1 );
    double zi;
    if( is3D() )
        zi = data( iRecord, getZindex() - 1 );
    else
        zi = 0.0;

    double xf = data( iRecord, getXFinalIndex() - 1 );
    double yf = data( iRecord, getYFinalIndex() - 1 );
    double zf;
    if( is3D() )
        zf = data( iRecord, getZFinalIndex() - 1 );
    else
        zf = 0.0;

    x = ( xi + xf ) / 2.0;
    y = ( yi + yf ) / 2.0;
    z = ( zi + zf ) / 2.0;

    //Segment sets don't have topological coordinates, they don't even have topology.
    i = 0;
    j = 0;
    k = 0;
}

QString SegmentSet::getFileType() const
{
    return "SEGMENTSET";
}

void SegmentSet::updateMetaDataFile()
{
    QFile file( this->getMetaDataFilePath() );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);
    out << APP_NAME << " metadata file.  This file is generated automatically.  Do not edit this file.\n";
    out << "version=" << APP_VERSION << '\n';
    out << "Xinitial:" << this->_x_field_index << '\n';
    out << "Yinitial:" << this->_y_field_index << '\n';
    out << "Zinitial:" << this->_z_field_index << '\n';
    out << "Xfinal:" << this->_x_final_field_index << '\n';
    out << "Yfinal:" << this->_y_final_field_index << '\n';
    out << "Zfinal:" << this->_z_final_field_index << '\n';
    out << "NDV:" << this->_no_data_value << '\n';
    QMapIterator<uint, uint> i( this->_wgt_var_pairs );
    while (i.hasNext()) {
        i.next();
        out << "WEIGHT:" << i.value() << '>' << i.key() << '\n';
    }
    QMapIterator<uint, QPair<uint,QString> > j( this->_nsvar_var_trn );
    while (j.hasNext()) {
        j.next();
        out << "NSCORE:" << j.value().first << '>' << j.key() << '=' << j.value().second << '\n';
    }
    QList< QPair<uint,QString> >::iterator k = _categorical_attributes.begin();
    for(; k != _categorical_attributes.end(); ++k){
        out << "CATEGORICAL:" << (*k).first << "," << (*k).second << '\n';
    }
    file.close();
}

double SegmentSet::getDataSpatialLocation(uint line, CartesianCoord whichCoord)
{
    switch ( whichCoord ) {
    case CartesianCoord::X: return ( data( line, _x_field_index - 1 ) + data( line, _x_final_field_index - 1 ) ) / 2.0; //x,y,z is in data file directly
    case CartesianCoord::Y: return ( data( line, _y_field_index - 1 ) + data( line, _y_final_field_index - 1 ) ) / 2.0; //x,y,z is in data file directly
    case CartesianCoord::Z:
        if( isTridimensional() )
            return  ( data( line, _z_field_index - 1 ) + data( line, _z_final_field_index - 1 ) ) / 2.0; //x,y,z is in data file directly
        else
            return 0.0; //returns z=0.0 for datasets in 2D.
    }
}

void SegmentSet::getDataSpatialLocation(uint line, double &x, double &y, double &z)
{
    x = ( data( line, _x_field_index - 1 ) + data( line, _x_final_field_index - 1 ) ) / 2.0; //x,y,z is in data file directly
    y = ( data( line, _y_field_index - 1 ) + data( line, _y_final_field_index - 1 ) ) / 2.0; //x,y,z is in data file directly
    if( isTridimensional() )
        z = ( data( line, _z_field_index - 1 ) + data( line, _z_final_field_index - 1 ) ) / 2.0; //x,y,z is in data file directly
    else
        z = 0.0; //returns z=0.0 for datasets in 2D.
}

double SegmentSet::getProportion(int variableIndex, double value0, double value1)
{
    double lengthYES = 0.0;
    double lengthNO = 0.0;
    for( int i = 0; i < getDataLineCount(); ++i ){
        double value = data( i, variableIndex );
        if( ! isNDV( value ) ){
            if( value >= value0 && value <= value1 ){
                lengthYES += getSegmentLenght( i );
            } else {
                lengthNO += getSegmentLenght( i );
            }
        }
    }
    if( (lengthYES + lengthNO) > 0 )
        return lengthYES / ( lengthYES + lengthNO );
    else
        return 0.0;
}

bool SegmentSet::getCenter(double &x, double &y, double &z) const
{
    if( getDataLineCount() == 0){
        Application::instance()->logError("SegmentSet::getCenter(): data not loaded."
                                          " Maybe a prior call to readFromFS() is missing. ");
        return false;
    } else {
        double meanX = 0.0, meanY = 0.0, meanZ = 0.0;
        //sums up the mid points of each segment
        for( int iDataLine = 0; iDataLine < getDataLineCount(); ++iDataLine ){
            meanX += ( dataConst( iDataLine, _x_field_index - 1 ) +
                       dataConst( iDataLine, _x_final_field_index - 1 ) ) / 2.0; //x,y,z is in data file directly
            meanY += ( dataConst( iDataLine, _y_field_index - 1 ) +
                       dataConst( iDataLine, _y_final_field_index - 1 ) ) / 2.0; //x,y,z is in data file directly
            if( is3D() ) //returns z=0.0 for datasets in 2D.
                meanZ += ( dataConst( iDataLine, _z_field_index - 1 ) +
                           dataConst( iDataLine, _z_final_field_index - 1 ) ) / 2.0; //x,y,z is in data file directly
        }
        //returns the means of the segments mid points.
        x = meanX / getDataLineCount();
        y = meanY / getDataLineCount();
        z = meanZ / getDataLineCount();
        return true;
    }
}

void SegmentSet::setInfoFromOtherPointSet(PointSet *otherPS)
{
    Q_UNUSED( otherPS );
    assert( false && "Calling setInfoFromOtherPointSet() for a SegmentSet is illegal.");
}

void SegmentSet::deleteVariable(uint columnToDelete)
{
    if( isCoordinate( columnToDelete )){
        Application::instance()->logError("SegmentSet::deleteVariable(): cannot remove spatial coordinates.");
        return;
    }

    uint columnToDeleteGEOEAS = columnToDelete+1; //first GEO-EAS index is 1, not zero.

    //it may be necessary to update the indexes of the fields set as the coordinates of the tail
    //of this segment set.
    if( _x_final_field_index > columnToDeleteGEOEAS )
        --_x_final_field_index;
    if( _y_final_field_index > columnToDeleteGEOEAS )
        --_y_final_field_index;
    if( _z_final_field_index > columnToDeleteGEOEAS )
        --_z_final_field_index;

    //call superclass' deleteVariable() to do the rest of the job
    PointSet::deleteVariable( columnToDelete );
}
