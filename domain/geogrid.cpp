#include "geogrid.h"
#include "viewer3d/view3dviewdata.h"
#include "viewer3d/view3dbuilders.h"
#include "domain/attribute.h"
#include "domain/cartesiangrid.h"

#include <QFile>
#include <QTextStream>

#include <cassert>

GeoGrid::GeoGrid( QString path ) : GridFile( path )
{
}

GeoGrid::GeoGrid(QString path, Attribute * atTop, Attribute * atBase, uint nHorizonSlices) : GridFile( path )
{
	CartesianGrid *cgTop = dynamic_cast<CartesianGrid*>( atTop->getContainingFile() );
	CartesianGrid *cgBase = dynamic_cast<CartesianGrid*>( atBase->getContainingFile() );
	assert( cgTop && "GeoGrid(): top attribute is not from a CartesianGrid object." );
	assert( cgBase && "GeoGrid(): base attribute is not from a CartesianGrid object." );
	assert( cgBase == cgTop && "GeoGrid(): top and base attributes must be from the same CartesianGrid object." );
	assert( ! cgBase->isTridimensional() && "GeoGrid(): The CartesianGrid of top and base must be 2D (map)." );

	uint nIVertexes = cgBase->getNI();
	uint nJVertexes = cgBase->getNJ();
	uint nKVertexes = nHorizonSlices + 1;
	uint nVertexes = nIVertexes * nJVertexes * nKVertexes;

	//allocate the vertex list
	m_vertexesPart.reserve( nVertexes );

	//get the indexes of the properties holding the top and base values.
	uint columnIndexBase = atBase->getAttributeGEOEASgivenIndex()-1;
	uint columnIndexTop = atTop->getAttributeGEOEASgivenIndex()-1;

	//create the vertexes
	for( uint k = 0; k < nKVertexes; ++k ){
		for( uint j = 0; j < nJVertexes; ++j ){
			for( uint i = 0; i < nIVertexes; ++i ){
				//get base and top values
				double vBase = cgBase->dataIJK( columnIndexBase, i, j, 0 );
				double vTop = cgTop->dataIJK( columnIndexTop, i, j, 0 );
				//compute the depth (z) of the current vertex.
				double depth = vBase + ( k / (double)nHorizonSlices ) * ( vTop - vBase );
				//create and set the position of the vertex
				VertexRecordPtr vertex( new VertexRecord() );
				double x, y, z;
				cgBase->IJKtoXYZ( i, j, 0, x, y, z );
				vertex->X = x;
				vertex->Y = y;
				vertex->Z = depth;
				m_vertexesPart.push_back( vertex );
			}
		}
	}

	//define the number of cells (cell centered values, one less than the number of vertexes in each direction )
	uint nICells = cgBase->getNI()-1;
	uint nJCells = cgBase->getNJ()-1;
	uint nKCells = nHorizonSlices;
	uint nCells = nICells * nJCells * nKCells;

	//allocate the cell definition list
	m_cellDefsPart.reserve( nCells );

	//assign vertexes id's to the cells
	for( uint k = 0; k < nKCells; ++k ){
		for( uint j = 0; j < nJCells; ++j ){
			for( uint i = 0; i < nICells; ++i ){
				CellDefRecordPtr cellDef( new CellDefRecord() );
				//see Doxygen of CellDefRecordPtr in geogrid.h for a diagram of vertex arrangement in space
				// and how they form the edges and faces of the visual representation of the cell.
				cellDef->vId[0] = ( k + 0 ) * nJVertexes * nIVertexes + ( j + 0 ) * nIVertexes + ( i + 0 );
				cellDef->vId[1] = ( k + 0 ) * nJVertexes * nIVertexes + ( j + 0 ) * nIVertexes + ( i + 1 );
				cellDef->vId[2] = ( k + 0 ) * nJVertexes * nIVertexes + ( j + 1 ) * nIVertexes + ( i + 1 );
				cellDef->vId[3] = ( k + 0 ) * nJVertexes * nIVertexes + ( j + 1 ) * nIVertexes + ( i + 0 );
				cellDef->vId[4] = ( k + 1 ) * nJVertexes * nIVertexes + ( j + 0 ) * nIVertexes + ( i + 0 );
				cellDef->vId[5] = ( k + 1 ) * nJVertexes * nIVertexes + ( j + 0 ) * nIVertexes + ( i + 1 );
				cellDef->vId[6] = ( k + 1 ) * nJVertexes * nIVertexes + ( j + 1 ) * nIVertexes + ( i + 1 );
				cellDef->vId[7] = ( k + 1 ) * nJVertexes * nIVertexes + ( j + 1 ) * nIVertexes + ( i + 0 );
				m_cellDefsPart.push_back( cellDef );
			}
		}
	}
}

void GeoGrid::IJKtoXYZ(uint i, uint j, uint k, double & x, double & y, double & z)
{
	uint cellIndex = k * m_nJ * m_nI + j * m_nI + i;
	CellDefRecordPtr cellDef = m_cellDefsPart.at( cellIndex );
	x = ( m_vertexesPart.at( cellDef->vId[0] )->X +
		  m_vertexesPart.at( cellDef->vId[1] )->X +
		  m_vertexesPart.at( cellDef->vId[2] )->X +
		  m_vertexesPart.at( cellDef->vId[3] )->X +
		  m_vertexesPart.at( cellDef->vId[4] )->X +
		  m_vertexesPart.at( cellDef->vId[5] )->X +
		  m_vertexesPart.at( cellDef->vId[6] )->X +
		  m_vertexesPart.at( cellDef->vId[7] )->X ) / 8 ;
	y = ( m_vertexesPart.at( cellDef->vId[0] )->Y +
		  m_vertexesPart.at( cellDef->vId[1] )->Y +
		  m_vertexesPart.at( cellDef->vId[2] )->Y +
		  m_vertexesPart.at( cellDef->vId[3] )->Y +
		  m_vertexesPart.at( cellDef->vId[4] )->Y +
		  m_vertexesPart.at( cellDef->vId[5] )->Y +
		  m_vertexesPart.at( cellDef->vId[6] )->Y +
		  m_vertexesPart.at( cellDef->vId[7] )->Y ) / 8 ;
	z = ( m_vertexesPart.at( cellDef->vId[0] )->Z +
		  m_vertexesPart.at( cellDef->vId[1] )->Z +
		  m_vertexesPart.at( cellDef->vId[2] )->Z +
		  m_vertexesPart.at( cellDef->vId[3] )->Z +
		  m_vertexesPart.at( cellDef->vId[4] )->Z +
		  m_vertexesPart.at( cellDef->vId[5] )->Z +
		  m_vertexesPart.at( cellDef->vId[6] )->Z +
		  m_vertexesPart.at( cellDef->vId[7] )->Z ) / 8 ;
}

SpatialLocation GeoGrid::getCenter()
{
	//TODO: maybe we can compute centre once and cache it to avoid repetive
	// cell center calculations
	double meanX = 0.0;
	double meanY = 0.0;
	double meanZ = 0.0;
	STOPPED_HERE;
}

double GeoGrid::getDataSpatialLocation(uint line, CartesianCoord whichCoord)
{
	uint i, j, k;
	double x, y, z;
	indexToIJK( line, i, j, k );
	IJKtoXYZ( i, j, k, x, y, z);
	switch ( whichCoord ) {
	case CartesianCoord::X: return x;
	case CartesianCoord::Y: return y;
	case CartesianCoord::Z: return z;
	default: return x;
	}
}

bool GeoGrid::canHaveMetaData()
{
	return true;
}

QString GeoGrid::getFileType()
{
	return "GEOGRID";
}

void GeoGrid::updateMetaDataFile()
{
	QFile file( this->getMetaDataFilePath() );
	file.open( QFile::WriteOnly | QFile::Text );
	QTextStream out(&file);
	out << APP_NAME << " metadata file.  This file is generated automatically.  Do not edit this file.\n";
	out << "version=" << APP_VERSION << '\n';
	out << "NI:" << this->m_nI << '\n';
	out << "NJ:" << this->m_nJ << '\n';
	out << "NK:" << this->m_nK << '\n';
	out << "NREAL:" << this->m_nreal << '\n';
	out << "NDV:" << this->_no_data_value << '\n';
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

QIcon GeoGrid::getIcon()
{
	if( m_nreal == 1)
		return QIcon(":icons32/geogrid32");
	else
		return QIcon(":icons32/geogrid32n");
}

void GeoGrid::save(QTextStream * txt_stream)
{
	(*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
	//also saves the metadata file.
	this->updateMetaDataFile();
}

View3DViewData GeoGrid::build3DViewObjects(View3DWidget * widget3D)
{
	return View3DBuilders::build( this, widget3D );
}

void GeoGrid::getSpatialAndTopologicalCoordinates(int iRecord, double & x, double & y, double & z, int & i, int & j, int & k)
{
	indexToIJK( iRecord, (uint&)i, (uint&)j, (uint&)k );
	IJKtoXYZ( i, j, k, x, y, z );
}

