#include "pointset.h"
#include <QTextStream>
#include <QFile>
#include <QStringList>
#include <QRegularExpression>
#include "../util.h"
#include "attribute.h"
#include "application.h"
#include "../exceptions/invalidgslibdatafileexception.h"
#include "weight.h"
#include "util.h"
#include <limits>
#include "viewer3d/view3dviewdata.h"
#include "viewer3d/view3dbuilders.h"


PointSet::PointSet( QString path ) : DataFile( path )
{
    this->_x_field_index = 0;
    this->_y_field_index = 0;
    this->_z_field_index = 0;
    this->_no_data_value = "";
}

QIcon PointSet::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/pointset16");
    else
        return QIcon(":icons32/pointset32");
}

void PointSet::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
    //also saves the metadata file.
    this->updateMetaDataFile();
}

View3DViewData PointSet::build3DViewObjects(View3DWidget *widget3D)
{
	return View3DBuilders::build( this, widget3D );
}

void PointSet::getSpatialAndTopologicalCoordinates(int iRecord, double & x, double & y, double & z, int & i, int & j, int & k)
{
    x = data( iRecord, getXindex() - 1 );
    y = data( iRecord, getYindex() - 1 );
	if( is3D() )
        z = data( iRecord, getZindex() - 1 );
	else
		z = 0.0;
	//Point sets don't have topological coordinates, they don't even have topology.
	i = 0;
	j = 0;
	k = 0;
}

double PointSet::getNeighborValue(int iRecord, int iVar, int dI, int dJ, int dK)
{
	Q_UNUSED(iRecord);
	Q_UNUSED(iVar);
	Q_UNUSED(dI);
	Q_UNUSED(dJ);
	Q_UNUSED(dK);
	return std::numeric_limits<double>::quiet_NaN();
}

void PointSet::setInfo(int x_index, int y_index, int z_index, const QString no_data_value)
{
    QMap<uint, uint> empty;
    QMap< uint, QPair<uint, QString> > empty2;
    QList< QPair< uint, QString> > empty3;
    this->setInfo( x_index, y_index, z_index, no_data_value, empty, empty2, empty3);
}

void PointSet::setInfo(int x_index, int y_index, int z_index, const QString no_data_value,
                       const QMap<uint, uint> &wgt_var_pairs,
                       const QMap<uint, QPair<uint, QString> > &nvar_var_trn_triads,
                       const QList< QPair<uint,QString> > &categorical_attributes)
{
    //updates metadata
    this->_x_field_index = x_index;
    this->_y_field_index = y_index;
    this->_z_field_index = z_index;
    this->_no_data_value = no_data_value;
    _wgt_var_pairs.clear();
    _wgt_var_pairs.unite( wgt_var_pairs );
    _nsvar_var_trn.clear();
    _nsvar_var_trn.unite( nvar_var_trn_triads );
    _categorical_attributes.clear();
    _categorical_attributes << categorical_attributes;
    this->updateChildObjectsCollection();
}


bool PointSet::isWeight(Attribute *at)
{
    uint index_in_GEOEAS_file = this->getFieldGEOEASIndex( at->getName() );
    return _wgt_var_pairs.contains( index_in_GEOEAS_file );
}

Attribute *PointSet::getVariableOfWeight(Attribute *weight)
{
    uint wgt_index_in_GEOEAS_file = this->getFieldGEOEASIndex( weight->getName() );
    Attribute* variable = nullptr;
    if( _wgt_var_pairs.contains( wgt_index_in_GEOEAS_file ) ){
        uint var_index_in_GEOEAS_file = _wgt_var_pairs[ wgt_index_in_GEOEAS_file ];
        variable = this->getAttributeFromGEOEASIndex( var_index_in_GEOEAS_file );
    }
    return variable;
}

void PointSet::deleteVariable(uint columnToDelete)
{
    if( isCoordinate( columnToDelete )){
        Application::instance()->logError("PointSet::deleteVariable(): cannot remove spatial coordinates.");
        return;
    }

    uint columnToDeleteGEOEAS = columnToDelete+1; //first GEO-EAS index is 1, not zero.

    //remove any references to the variable in the list of weight-variable relation
    //The indexes in this list are in GEO-EAS convention (1 == first)
    QMap< uint, uint >::iterator it = _wgt_var_pairs.begin();
    for(; it != _wgt_var_pairs.end();){
        if( it.key() == columnToDeleteGEOEAS || *it == columnToDeleteGEOEAS )
            it = _wgt_var_pairs.erase( it ); //QMap::erase() does the increment to the next element (do not add ++ here)
        else
            ++it;
    }

    //Decrement all indexes greater than the deleted variable index in the list of weight-variable relation
    QMap< uint, uint > temp;
    it = _wgt_var_pairs.begin();
    for(; it != _wgt_var_pairs.end(); ++it){
        uint key = it.key();
        uint index = *it;
        if( key > columnToDeleteGEOEAS )
            --key;
        if( index > columnToDeleteGEOEAS )
            --index;
        temp.insert( key, index );
    }
    _wgt_var_pairs.swap(temp);

    //it may be necessary to update the indexes of the fields set as the coordinates of this point set.
    if( _x_field_index > columnToDeleteGEOEAS )
        --_x_field_index;
    if( _y_field_index > columnToDeleteGEOEAS )
        --_y_field_index;
    if( _z_field_index > columnToDeleteGEOEAS )
        --_z_field_index;

    //call superclass' deleteVariable() to do the rest of the job
	DataFile::deleteVariable( columnToDelete );
}

double PointSet::getDataSpatialLocation(uint line, CartesianCoord whichCoord)
{
	switch ( whichCoord ) {
	case CartesianCoord::X: return data( line, _x_field_index - 1 ); //x,y,z is in data file directly
	case CartesianCoord::Y: return data( line, _y_field_index - 1 ); //x,y,z is in data file directly
	case CartesianCoord::Z:
		if( isTridimensional() )
			return data( line, _z_field_index - 1 ); //x,y,z is in data file directly
		else
			return 0.0; //returns z=0.0 for datasets in 2D.
    }
}

void PointSet::getDataSpatialLocation(uint line, double &x, double &y, double &z)
{
    x = data( line, _x_field_index - 1 ); //x,y,z is in data file directly
    y = data( line, _y_field_index - 1 ); //x,y,z is in data file directly
    if( isTridimensional() )
        z = data( line, _z_field_index - 1 ); //x,y,z is in data file directly
    else
        z = 0.0; //returns z=0.0 for datasets in 2D.
}

bool PointSet::isTridimensional()
{
    return is3D();
}

bool PointSet::getCenter(double &x, double &y, double &z) const
{
    if( getDataLineCount() == 0){
        Application::instance()->logError("PointSet::getCenter(): data not loaded."
                                          " Maybe a prior call to readFromFS() is missing. ");
        return false;
    } else {
        double meanX = 0.0, meanY = 0.0, meanZ = 0.0;
        //sums up the mid points of each segment
        for( int iDataLine = 0; iDataLine < getDataLineCount(); ++iDataLine ){
            meanX += dataConst( iDataLine, _x_field_index - 1 ) ; //x,y,z is in data file directly
            meanY += dataConst( iDataLine, _y_field_index - 1 ) ; //x,y,z is in data file directly
            if( is3D() ) //returns z=0.0 for datasets in 2D.
                meanZ += dataConst( iDataLine, _z_field_index - 1 ) ; //x,y,z is in data file directly
        }
        //returns the means of the coordinates.
        x = meanX / getDataLineCount();
        y = meanY / getDataLineCount();
        z = meanZ / getDataLineCount();
        return true;
    }
}

void PointSet::setInfoFromMetadataFile()
{
    QString md_file_path( this->_path );
    QFile md_file( md_file_path.append(".md") );
    int x_index = 0, y_index = 0, z_index = 0;
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
           if( line.startsWith( "X:" ) ){
               QString value = line.split(":")[1];
               x_index = value.toInt();
           }else if( line.startsWith( "Y:" ) ){
               QString value = line.split(":")[1];
               y_index = value.toInt();
           }else if( line.startsWith( "Z:" ) ){
               QString value = line.split(":")[1];
               z_index = value.toInt();
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
        this->setInfo( x_index, y_index, z_index, ndv, wgt_var_pairs, nsvar_var_trn, categorical_attributes );
    }
}

void PointSet::setInfoFromOtherPointSet(PointSet *otherPS)
{
    QString ndv;
    QMap<uint, QPair<uint, QString> > nsvar_var_trn_triads;
    QList< QPair<uint, QString> > categorical_attributes;
    QMap<uint, uint> wgt_var_pairs;
    ndv = otherPS->getNoDataValue();
    nsvar_var_trn_triads = otherPS->getNSVarVarTrnTriads();
    wgt_var_pairs = otherPS->getWeightsVariablesPairs();
    categorical_attributes = otherPS->getCategoricalAttributes();
    this->setInfo( otherPS->getXindex(), otherPS->getYindex(), otherPS->getZindex(),
                   ndv, wgt_var_pairs, nsvar_var_trn_triads, categorical_attributes);
}

int PointSet::getXindex() const
{
    return this->_x_field_index;
}

int PointSet::getYindex() const
{
    return this->_y_field_index;
}

int PointSet::getZindex() const
{
    return this->_z_field_index;
}

bool PointSet::is3D() const
{
    return getZindex() > 0;
}

void PointSet::addVariableWeightRelationship(uint variableGEOEASindex, uint weightGEOEASindex)
{
    this->_wgt_var_pairs[weightGEOEASindex] = variableGEOEASindex;
    //save the updated metadata to disk.
    this->updateMetaDataFile();
}

bool PointSet::isCoordinate(uint column) const
{
    int columnGEOEAS = column + 1;
    return ( _x_field_index == columnGEOEAS ) ||
           ( _y_field_index == columnGEOEAS ) ||
            ( _z_field_index == columnGEOEAS ) ;
}

PointSet *PointSet::createPointSetByFiltering(uint column, double vMin, double vMax)
{
    //Create new empty point set.
    PointSet* newPS = new PointSet( "" );
    //Get filtered data frame.
    std::vector< std::vector< double > > filteredData = getDataFilteredBy( column, vMin, vMax );
    //Assign it as the new point set's data.
    newPS->_data = filteredData;
    //Set the same metadata.
    newPS->setInfoFromOtherPointSet( this );
    //Return the new filtered data set.
    return newPS;
}

void PointSet::cloneDataLine(uint row)
{
    //get a copy of the desired data line.
    const std::vector< double > rowDataToCopy = _data[ row ];

    //get the vector iterator refering to the first data record.
    std::vector< std::vector<double> >::iterator itFirstElement;
    itFirstElement = _data.begin();

    //std::vector.insert() inserts elements BEFORE the given index, hence we increase the
    //given index by 1 so the copied data row is added AFTER the original data record.
    _data.insert( itFirstElement + row + 1, rowDataToCopy );
}

bool PointSet::canHaveMetaData()
{
    return true;
}

QString PointSet::getFileType() const
{
    return "POINTSET";
}

void PointSet::updateMetaDataFile()
{
    QFile file( this->getMetaDataFilePath() );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);
    out << APP_NAME << " metadata file.  This file is generated automatically.  Do not edit this file.\n";
    out << "version=" << APP_VERSION << '\n';
    out << "X:" << this->_x_field_index << '\n';
    out << "Y:" << this->_y_field_index << '\n';
    out << "Z:" << this->_z_field_index << '\n';
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
