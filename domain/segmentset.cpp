#include "segmentset.h"
#include "viewer3d/view3dviewdata.h"
#include "viewer3d/view3dbuilders.h"
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

int SegmentSet::getXFinalIndex()
{
    return _x_final_field_index;
}

int SegmentSet::getYFinalIndex()
{
    return _y_final_field_index;
}

int SegmentSet::getZFinalIndex()
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

double SegmentSet::getDistanceToNextSegment(int iRecord)
{
    if( iRecord == getDataLineCount() - 1 )
        return 0.0;
    double dx = data( iRecord+1, getXindex()-1 ) - data( iRecord, getXFinalIndex()-1 );
    double dy = data( iRecord+1, getYindex()-1 ) - data( iRecord, getYFinalIndex()-1 );
    double dz = data( iRecord+1, getZindex()-1 ) - data( iRecord, getZFinalIndex()-1 );
    return std::sqrt( dx*dx + dy*dy + dz*dz );
}


QIcon SegmentSet::getIcon()
{
    return QIcon(":icons32/segmentset32");
}

QString SegmentSet::getTypeName()
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

    double xi = data( iRecord, getXindex() );
    double yi = data( iRecord, getYindex() );
    double zi;
    if( is3D() )
        zi = data( iRecord, getZindex() );
    else
        zi = 0.0;

    double xf = data( iRecord, getXFinalIndex() );
    double yf = data( iRecord, getYFinalIndex() );
    double zf;
    if( is3D() )
        zf = data( iRecord, getZFinalIndex() );
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

QString SegmentSet::getFileType()
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

void SegmentSet::setInfoFromOtherPointSet(PointSet *otherPS)
{
    Q_UNUSED( otherPS );
    assert( false && "Calling setInfoFromOtherPointSet() for a SegmentSet is illegal.");
}
