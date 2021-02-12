#include "domain/geogrid.h"
#include "viewer3d/view3dviewdata.h"
#include "viewer3d/view3dbuilders.h"
#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "spatialindex/spatialindex.h"
#include "domain/application.h"
#include "auxiliary/meshloader.h"
#include "domain/pointset.h"
#include "domain/segmentset.h"
#include "util.h"
#include "domain/project.h"
#include "geometry/vector3d.h"
#include "geometry/face3d.h"
#include "exceptions/invalidmethodexception.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QProgressDialog>
#include <QTextStream>
#include <QThread>

#include <cassert>

GeoGrid::GeoGrid( QString path ) :
	GridFile( path ),
	m_spatialIndex( new SpatialIndex() ),
	m_lastModifiedDateTimeLastMeshLoad()
{
	this->_no_data_value = "";
	this->m_nreal = 1;
	this->m_nI = 0;
	this->m_nJ = 0;
	this->m_nK = 0;
}

GeoGrid::GeoGrid(QString path, Attribute * atTop, Attribute * atBase, uint nHorizonSlices) :
	GridFile( path ),
	m_spatialIndex( new SpatialIndex() ),
	m_lastModifiedDateTimeLastMeshLoad()
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

	//initialize member variables
	m_nI = nICells;
	m_nJ = nJCells;
	m_nK = nKCells;
	m_nreal = 1;
    _no_data_value = "";
}

GeoGrid::GeoGrid(QString path, std::vector<GeoGridZone> zones) :
    GridFile( path ),
    m_spatialIndex( new SpatialIndex() ),
    m_lastModifiedDateTimeLastMeshLoad()
{
    //get origin Cartesian grid and do some sanity checks
    CartesianGrid *cg = nullptr;
    for( const GeoGridZone zone : zones ){
        CartesianGrid *cgTop = dynamic_cast<CartesianGrid*>( zone.top->getContainingFile() );
        CartesianGrid *cgBase = dynamic_cast<CartesianGrid*>( zone.base->getContainingFile() );
        assert( cgTop && "GeoGrid(): top attribute is not from a CartesianGrid object." );
        assert( cgBase && "GeoGrid(): base attribute is not from a CartesianGrid object." );
        assert( cgBase == cgTop && "GeoGrid(): top and base attributes must be from the same CartesianGrid object." );
        assert( ! cgBase->isTridimensional() && "GeoGrid(): The CartesianGrid of top and base must be 2D (map)." );
        cg = cgTop;
    }
    assert( cg && "GeoGrid(): no zones passed." );

    //get grid geometry
    uint nIVertexes = cg->getNI();
    uint nJVertexes = cg->getNJ();
    uint nKVertexes = 0;
    for( const GeoGridZone zone : zones )
        nKVertexes += zone.nHorizonSlices + 1;
    uint nVertexes = nIVertexes * nJVertexes * nKVertexes;

    //allocate the vertex list
    m_vertexesPart.reserve( nVertexes );

    //traverse the collection of zones from bottommost to topmost (reverse of user-entered order)
    for ( std::vector<GeoGridZone>::const_reverse_iterator iZone = zones.crbegin(); iZone != zones.crend(); ++iZone ) {
        const GeoGridZone& zone = *(iZone);

        //get the indexes of the properties holding the top and base values.
        uint columnIndexBase = zone.base->getAttributeGEOEASgivenIndex()-1;
        uint columnIndexTop = zone.top->getAttributeGEOEASgivenIndex()-1;

        uint nKVertexesZone = zone.nHorizonSlices + 1;

        //create the vertexes
        for( uint k = 0; k < nKVertexesZone; ++k ){
            for( uint j = 0; j < nJVertexes; ++j ){
                for( uint i = 0; i < nIVertexes; ++i ){
                    //get base and top values
                    double vBase = cg->dataIJK( columnIndexBase, i, j, 0 );
                    double vTop = cg->dataIJK( columnIndexTop, i, j, 0 );
                    //compute the depth (z) of the current vertex.
                    double depth = vBase + ( k / (double)zone.nHorizonSlices ) * ( vTop - vBase );
                    //create and set the position of the vertex
                    VertexRecordPtr vertex( new VertexRecord() );
                    double x, y, z;
                    cg->IJKtoXYZ( i, j, 0, x, y, z );
                    vertex->X = x;
                    vertex->Y = y;
                    vertex->Z = depth;
                    m_vertexesPart.push_back( vertex );
                }
            }
        }
    }

    //define the number of cells (cell centered values, one less than the number of vertexes in each direction )
    uint nICells = cg->getNI()-1;
    uint nJCells = cg->getNJ()-1;
    uint nKCells = 0;
    for( const GeoGridZone zone : zones )
        nKCells += zone.nHorizonSlices;
    uint nCells = nICells * nJCells * nKCells;

    //allocate the cell definition list
    m_cellDefsPart.reserve( nCells );

    uint vertexKoffset = 0;
    for( const GeoGridZone zone : zones ){
        uint nKCellsZone = zone.nHorizonSlices;
        //assign vertexes id's to the cells
        for( uint k = 0; k < nKCellsZone; ++k ){
            for( uint j = 0; j < nJCells; ++j ){
                for( uint i = 0; i < nICells; ++i ){
                    CellDefRecordPtr cellDef( new CellDefRecord() );
                    //see Doxygen of CellDefRecordPtr in geogrid.h for a diagram of vertex arrangement in space
                    // and how they form the edges and faces of the visual representation of the cell.
                    cellDef->vId[0] = ( vertexKoffset + k + 0 ) * nJVertexes * nIVertexes + ( j + 0 ) * nIVertexes + ( i + 0 );
                    cellDef->vId[1] = ( vertexKoffset + k + 0 ) * nJVertexes * nIVertexes + ( j + 0 ) * nIVertexes + ( i + 1 );
                    cellDef->vId[2] = ( vertexKoffset + k + 0 ) * nJVertexes * nIVertexes + ( j + 1 ) * nIVertexes + ( i + 1 );
                    cellDef->vId[3] = ( vertexKoffset + k + 0 ) * nJVertexes * nIVertexes + ( j + 1 ) * nIVertexes + ( i + 0 );
                    cellDef->vId[4] = ( vertexKoffset + k + 1 ) * nJVertexes * nIVertexes + ( j + 0 ) * nIVertexes + ( i + 0 );
                    cellDef->vId[5] = ( vertexKoffset + k + 1 ) * nJVertexes * nIVertexes + ( j + 0 ) * nIVertexes + ( i + 1 );
                    cellDef->vId[6] = ( vertexKoffset + k + 1 ) * nJVertexes * nIVertexes + ( j + 1 ) * nIVertexes + ( i + 1 );
                    cellDef->vId[7] = ( vertexKoffset + k + 1 ) * nJVertexes * nIVertexes + ( j + 1 ) * nIVertexes + ( i + 0 );
                    m_cellDefsPart.push_back( cellDef );
                }
            }
        }
        vertexKoffset += nKCellsZone + 1;
    }

    //initialize member variables
    m_nI = nICells;
    m_nJ = nJCells;
    m_nK = nKCells;
    m_nreal = 1;
    _no_data_value = "";

}

void GeoGrid::getBoundingBox(uint cellIndex,
							 double & minX, double & minY, double & minZ,
							 double & maxX, double & maxY, double & maxZ )
{
	//initialize the results to ensure the returned extrema are those of the cell.
	minX = minY = minZ = std::numeric_limits<double>::max();
	maxX = maxY = maxZ = std::numeric_limits<double>::min();
	//Get the cell
	CellDefRecordPtr cellDef = m_cellDefsPart.at( cellIndex );
	//for each of the eight vertexes of the cell
	for( uint i = 0; i < 8; ++i ){
		//Get the vertex
		VertexRecordPtr vertex = m_vertexesPart.at( cellDef->vId[i] );
		//set the max's and min's
		minX = std::min( minX, vertex->X );
		minY = std::min( minY, vertex->Y );
		minZ = std::min( minZ, vertex->Z );
		maxX = std::max( maxX, vertex->X );
		maxY = std::max( maxY, vertex->Y );
		maxZ = std::max( maxZ, vertex->Z );
	}
}

QString GeoGrid::getMeshFilePath()
{
	QString mesh_file_path( this->_path );
	return mesh_file_path.append(".mesh");
}

void GeoGrid::saveMesh()
{
	if( m_vertexesPart.empty() || m_vertexesPart.empty() ){
		Application::instance()->logInfo("GeoGrid::saveMesh(): No mesh or mesh not loaded.  Nothing done.");
		return;
	}

	//close mesh file and write header
	QFile file( this->getMeshFilePath() );
	file.open( QFile::WriteOnly | QFile::Text );
	QTextStream out(&file);
	out << APP_NAME << " GeoGrid mesh file.  This file is generated automatically.  Do not edit this file.\n";
	out << "version=" << APP_VERSION << '\n';

	out << "VERTEX LOCATIONS:\n";
	{
		std::vector< VertexRecordPtr >::iterator it = m_vertexesPart.begin();
		for( ; it != m_vertexesPart.end(); ++it ){
			//making sure the values are written in GSLib-like precision
			std::stringstream ss;
			ss << std::setprecision( 12 ) << (*it)->X << ';';
			ss << std::setprecision( 12 ) << (*it)->Y << ';';
			ss << std::setprecision( 12 ) << (*it)->Z ;
			out << ss.str().c_str() << '\n';
		}
	}

	out << "CELL VERTEX INDEXES:\n";
	{
		std::vector< CellDefRecordPtr >::iterator it = m_cellDefsPart.begin();
		for( ; it != m_cellDefsPart.end(); ++it ){
			std::stringstream ss;
			for( int i = 0; i < 7; ++i)
				ss << (*it)->vId[i] << ';';
			ss << (*it)->vId[7];
			out << ss.str().c_str() << "\n";
		}
	}

	//close mesh file
	file.close();

	Application::instance()->logInfo("GeoGrid::saveMesh(): Mesh saved.");
}

void GeoGrid::loadMesh()
{
	QFile file( this->getMeshFilePath() );
	file.open(QFile::ReadOnly | QFile::Text);
	uint data_line_count = 0;
	QFileInfo info( this->getMeshFilePath() );

	// if loaded mesh is not empty and was loaded before
	if ( (!m_cellDefsPart.empty() || !m_vertexesPart.empty() ) && !m_lastModifiedDateTimeLastMeshLoad.isNull()) {
		QDateTime currentLastModified = info.lastModified();
		// if modified datetime didn't change since last call to loadMesh
		if (currentLastModified <= m_lastModifiedDateTimeLastMeshLoad) {
			Application::instance()->logInfo(
				QString("Mesh file ")
					.append( this->getMeshFilePath() )
					.append(" already loaded and up to date.  Did nothing."));
			return; // does nothing
		}
	}

	// record the current datetime of file change
	m_lastModifiedDateTimeLastMeshLoad = info.lastModified();

	Application::instance()->logInfo(
		QString("Loading mesh from ").append( this->getMeshFilePath() ).append("..."));

	// make sure mesh data is empty
	m_cellDefsPart.clear();
	m_vertexesPart.clear();
	std::vector< CellDefRecordPtr >().swap( m_cellDefsPart ); //clear() may not actually free memory
	std::vector< VertexRecordPtr >().swap( m_vertexesPart ); //clear() may not actually free memory

	// mesh load takes place in another thread, so we can show and update a progress bar
	//////////////////////////////////
	QProgressDialog progressDialog;
	progressDialog.show();
	progressDialog.setLabelText("Loading and parsing mesh file " + this->getMeshFilePath() + "...");
	progressDialog.setMinimum(0);
	progressDialog.setValue(0);
	progressDialog.setMaximum(getFileSize() / 100); // see MeshLoader::doLoad(). Dividing
													// by 100 allows a max value of ~400GB
													// when converting from long to int
	QThread *thread = new QThread(); // does it need to set parent (a QObject)?
	MeshLoader *ml = new MeshLoader(file, m_vertexesPart, m_cellDefsPart, data_line_count ); // Do not set a parent. The object
																							 // cannot be moved if it has a
																							 // parent.
	ml->moveToThread(thread);
	ml->connect(thread, SIGNAL(finished()), ml, SLOT(deleteLater()));
	ml->connect(thread, SIGNAL(started()), ml, SLOT(doLoad()));
	ml->connect(ml, SIGNAL(progress(int)), &progressDialog, SLOT(setValue(int)));
	thread->start();
	/////////////////////////////////

	// wait for the mesh load to finish
	// not very beautiful, but simple and effective
	while (!ml->isFinished()) {
		thread->wait(200); // reduces cpu usage, refreshes at each 500 milliseconds
		QCoreApplication::processEvents(); // let Qt repaint widgets
	}

	file.close();

	Application::instance()->logInfo("Finished loading mesh.");
}

void GeoGrid::setInfoFromMetadataFile()
{
	QString md_file_path( this->_path );
	QFile md_file( md_file_path.append(".md") );
	uint nI = 0, nJ = 0, nK = 0;
	uint nreal = 0;
	QString ndv;
	QMap<uint, QPair<uint,QString> > nsvar_var_trn;
	QList< QPair<uint,QString> > categorical_attributes;
	if( md_file.exists() ){
		md_file.open( QFile::ReadOnly | QFile::Text );
		QTextStream in(&md_file);
		for (int i = 0; !in.atEnd(); ++i)
		{
		   QString line = in.readLine();
		   if( line.startsWith( "NI:" ) ){
			   QString value = line.split(":")[1];
			   nI = value.toInt();
		   }else if( line.startsWith( "NJ:" ) ){
			   QString value = line.split(":")[1];
			   nJ = value.toInt();
		   }else if( line.startsWith( "NK:" ) ){
			   QString value = line.split(":")[1];
			   nK = value.toInt();
		   }else if( line.startsWith( "NREAL:" ) ){
			   QString value = line.split(":")[1];
			   nreal = value.toInt();
		   }else if( line.startsWith( "NDV:" ) ){
			   QString value = line.split(":")[1];
			   ndv = value;
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
			   categorical_attributes.append( QPair<uint,QString>(var.toUInt(), catDefName) );
		   }
		}
		md_file.close();
		this->setInfo( nI, nJ, nK, nreal, ndv, nsvar_var_trn, categorical_attributes);
	}
}

void GeoGrid::setInfo(int nI, int nJ, int nK, int nreal, const QString no_data_value,
					  QMap<uint, QPair<uint, QString> > nvar_var_trn_triads,
					  const QList<QPair<uint, QString> > & categorical_attributes)
{
	//updating metadata
	this->_no_data_value = no_data_value;
	this->m_nreal = nreal;
	this->m_nI = nI;
	this->m_nJ = nJ;
	this->m_nK = nK;
	_nsvar_var_trn.clear();
	_nsvar_var_trn.unite( nvar_var_trn_triads );
	_categorical_attributes.clear();
	_categorical_attributes << categorical_attributes;
	//update the attribute fields
	this->updateChildObjectsCollection();
}

uint GeoGrid::getMeshNumberOfVertexes()
{
	this->loadMesh();
	return m_vertexesPart.size();
}

void GeoGrid::getMeshVertexLocation(uint index, double & x, double & y, double & z)
{
	x = m_vertexesPart[index]->X;
	y = m_vertexesPart[index]->Y;
	z = m_vertexesPart[index]->Z;
}

uint GeoGrid::getMeshNumberOfCells()
{
	this->loadMesh();
	return m_cellDefsPart.size();
}

void GeoGrid::getMeshCellDefinition(uint index, uint (&vIds)[8])
{
	vIds[0] = m_cellDefsPart[index]->vId[0];
	vIds[1] = m_cellDefsPart[index]->vId[1];
	vIds[2] = m_cellDefsPart[index]->vId[2];
	vIds[3] = m_cellDefsPart[index]->vId[3];
	vIds[4] = m_cellDefsPart[index]->vId[4];
	vIds[5] = m_cellDefsPart[index]->vId[5];
	vIds[6] = m_cellDefsPart[index]->vId[6];
    vIds[7] = m_cellDefsPart[index]->vId[7];
}

PointSet *GeoGrid::unfold( PointSet *inputPS, QString nameForNewPointSet )
{
    if( ! inputPS->is3D() ){
        Application::instance()->logError("GeoGrid::unfold(): input point set is not 3D.");
        return nullptr;
    }

    //create the new point set object
    PointSet* result = new PointSet( Application::instance()->getProject()->getPath() +
                                     '/' + nameForNewPointSet );

    //make a duplicate of the input point set
    {
		//copy the physical data file
		Util::copyFile( inputPS->getPath(), result->getPath() );
		//copy metadata from the input point set
		result->setInfoFromOtherPointSet( inputPS );
    }

    //load the data
    result->loadData();

    //get the number of samples in the input point set
    uint nSamples = result->getDataLineCount();

    //append three new columns to the samples (output): the UVW coordinates
    result->addEmptyDataColumn( "U", nSamples );
    result->addEmptyDataColumn( "V", nSamples );
    result->addEmptyDataColumn( "W", nSamples );
    result->writeToFS();

	uint nColumns = result->getDataColumnCount();

    //iterate over the samples in the point set
    uint xIndex = result->getXindex() - 1; //first GEO-EAS index = 1
    uint yIndex = result->getYindex() - 1;
    uint zIndex = result->getZindex() - 1;
    std::vector<uint> samplesToRemove;
	bool empty = true;
    for( uint iSample = 0; iSample < nSamples; ++iSample ){
        //get the XYZ location of the sample
        double x = result->data( iSample, xIndex );
        double y = result->data( iSample, yIndex );
        double z = result->data( iSample, zIndex );
        //get the UVW coordinates
		double u = -1.0;
		double v = -1.0;
		double w = -1.0;
        if( XYZtoUVW( x, y, z, u, v, w ) ){
			empty = false;
			//assign them to the point set
			result->setData( iSample, nColumns - 3, u );
			result->setData( iSample, nColumns - 2, v );
			result->setData( iSample, nColumns - 1, w );
		} else {
            //a cell was not found (likely the sample is outside the grid)
            //so mark the sample for removal
            samplesToRemove.push_back( iSample );
        }
    }

	//changed the assigned X, Y, Z fields of the unfolded point set to the new U, V, W columns
    result->setInfo( nColumns - 2, nColumns - 1, nColumns, result->getNoDataValue(),
                     result->getWeightsVariablesPairs(), result->getNSVarVarTrnTriads(), result->getCategoricalAttributes() );

	//remove the samples with invalid UVW coordinates
	std::vector<uint>::iterator it = samplesToRemove.begin();
	uint offset = 0; //adjust for previously deleted lines.
	for( ; it != samplesToRemove.end(); ++it, ++offset )
		result->removeDataLine( *it - offset );

	//if no data remained
	if( empty ){
		//remove the file with partial data
		QFile resultFile( result->getPath() );
		resultFile.remove();
		//de-allocate the object
		delete result;
        Application::instance()->logError("GeoGrid::unfold(): Unfolding resulted in an empty point set. Canceled.");
		//return null pointer
		return nullptr;
	}

	//commit changes to filesystem
	result->writeToFS();

    return result;
}

SegmentSet *GeoGrid::unfold(SegmentSet *inputSS, QString nameForNewSegmentSet)
{

    if( ! inputSS->is3D() ){
        Application::instance()->logError("GeoGrid::unfold(): input segment set is not 3D.");
        return nullptr;
    }

    //create the new segment set object
    SegmentSet* result = new SegmentSet( Application::instance()->getProject()->getPath() +
                                        '/' + nameForNewSegmentSet );

    //make a duplicate of the input segment set
    {
        //copy the physical data file
        Util::copyFile( inputSS->getPath(), result->getPath() );
        //copy metadata from the input point set
        result->setInfoFromAnotherSegmentSet( inputSS );
    }

    //load the data
    result->loadData();

    //get the number of samples in the input point set
    uint nSamples = result->getDataLineCount();

    //append six new columns to the samples (output):
    //the initial and final UVW segment coordinates
    result->addEmptyDataColumn( "U_i", nSamples );
    result->addEmptyDataColumn( "V_i", nSamples );
    result->addEmptyDataColumn( "W_i", nSamples );
    result->addEmptyDataColumn( "U_f", nSamples );
    result->addEmptyDataColumn( "V_f", nSamples );
    result->addEmptyDataColumn( "W_f", nSamples );
    result->writeToFS();

    uint nColumns = result->getDataColumnCount();

    //iterate over the samples in the point set
    uint xIIndex = result->getXindex() - 1; //first GEO-EAS index = 1
    uint yIIndex = result->getYindex() - 1;
    uint zIIndex = result->getZindex() - 1;
    uint xFIndex = result->getXFinalIndex() - 1;
    uint yFIndex = result->getYFinalIndex() - 1;
    uint zFIndex = result->getZFinalIndex() - 1;
    std::vector<uint> samplesToRemove;
    bool empty = true;
    for( uint iSample = 0; iSample < nSamples; ++iSample ){
        //get the XYZ locations of the sample
        double xi = result->data( iSample, xIIndex );
        double yi = result->data( iSample, yIIndex );
        double zi = result->data( iSample, zIIndex );
        double xf = result->data( iSample, xFIndex );
        double yf = result->data( iSample, yFIndex );
        double zf = result->data( iSample, zFIndex );
        //get the UVW coordinates
        double ui = -1.0;
        double vi = -1.0;
        double wi = -1.0;
        double uf = -1.0;
        double vf = -1.0;
        double wf = -1.0;
        if( XYZtoUVW( xi, yi, zi, ui, vi, wi ) &&
            XYZtoUVW( xf, yf, zf, uf, vf, wf ) ){
            empty = false;
            //assign them to the point set
            result->setData( iSample, nColumns - 6, ui );
            result->setData( iSample, nColumns - 5, vi );
            result->setData( iSample, nColumns - 4, wi );
            result->setData( iSample, nColumns - 3, uf );
            result->setData( iSample, nColumns - 2, vf );
            result->setData( iSample, nColumns - 1, wf );
        } else {
            //a cell was not found (likely one or both ends of a sample is outside the grid)
            //so mark the sample for removal
            samplesToRemove.push_back( iSample );
        }
    }

    //changed the assigned X, Y, Z fields of the unfolded point set to the new U, V, W columns
    result->setInfo( nColumns - 5, nColumns - 4, nColumns - 3,
                     nColumns - 2, nColumns - 1, nColumns    , result->getNoDataValue(),
                     result->getWeightsVariablesPairs(), result->getNSVarVarTrnTriads(), result->getCategoricalAttributes() );

    //remove the samples with invalid UVW coordinates
    std::vector<uint>::iterator it = samplesToRemove.begin();
    uint offset = 0; //adjust for previously deleted lines.
    for( ; it != samplesToRemove.end(); ++it, ++offset )
        result->removeDataLine( *it - offset );

    //if no data remained
    if( empty ){
        //remove the file with partial data
        QFile resultFile( result->getPath() );
        resultFile.remove();
        //de-allocate the object
        delete result;
        Application::instance()->logError("GeoGrid::unfold(): Unfolding resulted in an empty segment set. Canceled.");
        //return null pointer
        return nullptr;
    }

    //commit changes to filesystem
    result->writeToFS();

    return result;
}

bool GeoGrid::XYZtoUVW(double x, double y, double z, double &u, double &v, double &w)
{
    // https://math.stackexchange.com/questions/13404/mapping-irregular-quadrilateral-to-a-rectangle

	//Obtain the topological coordinates (IJK) of the cell that contains the location.
	uint i, j, k;
	if( ! XYZtoIJK( x, y, z, i, j, k ) )
		return false;

	//Obtain the index of the cell.
	uint cellIndex = IJKtoIndex( i, j, k );

	//Make a point object corresponding to the query location.
	Vertex3D location{ x, y, z };

	//Get the faces' geometries of the cell.
	std::vector<Face3D> cellFaces = getFaces( cellIndex );

	//faces 2 and 3 are along I direction, thus are respect to U depositional coordinate
	//faces 4 and 5 are along J direction, thus are respect to V depositional coordinate
	//faces 0 and 1 are along K direction, thus are respect to W depositional coordinate
	//compute the six distances between the location and the faces
	double dU0 = cellFaces[2].distance( location );
	double dU1 = cellFaces[3].distance( location );
	double dV0 = cellFaces[4].distance( location );
	double dV1 = cellFaces[5].distance( location );
	double dW0 = cellFaces[0].distance( location );
	double dW1 = cellFaces[1].distance( location );

	//compute the UVW within the cell (min = 0.0, max = 1.0)
	double local_u = dU0 / (dU0 + dU1);
	double local_v = dV0 / (dV0 + dV1);
	double local_w = dW0 / (dW0 + dW1);

	//compute the global UVW steps.
	double du = 1.0 / m_nI;
	double dv = 1.0 / m_nJ;
	double dw = 1.0 / m_nK;

	//compute the UVW coordinates.
	u = du * i + du * local_u;
	v = dv * j + dv * local_v;
	w = dw * k + dw * local_w;

	return true;
}

std::vector<Face3D> GeoGrid::getFaces( uint cellIndex )
{
	//get the cell geometry definition (vertexes' indexes).
	CellDefRecordPtr cellDef = m_cellDefsPart.at( cellIndex );

	//get the vertex data of the cell
	VertexRecordPtr vd[8];
	vd[0] = m_vertexesPart.at( cellDef->vId[0] );
	vd[1] = m_vertexesPart.at( cellDef->vId[1] );
	vd[2] = m_vertexesPart.at( cellDef->vId[2] );
	vd[3] = m_vertexesPart.at( cellDef->vId[3] );
	vd[4] = m_vertexesPart.at( cellDef->vId[4] );
	vd[5] = m_vertexesPart.at( cellDef->vId[5] );
	vd[6] = m_vertexesPart.at( cellDef->vId[6] );
	vd[7] = m_vertexesPart.at( cellDef->vId[7] );

	//------------make the six face geometries------------
	std::vector<Face3D> fs( 6 );
	fs[0].v[0] = Vertex3D{ vd[0]->X, vd[0]->Y, vd[0]->Z };
	fs[0].v[1] = Vertex3D{ vd[1]->X, vd[1]->Y, vd[1]->Z };
	fs[0].v[2] = Vertex3D{ vd[2]->X, vd[2]->Y, vd[2]->Z };
	fs[0].v[3] = Vertex3D{ vd[3]->X, vd[3]->Y, vd[3]->Z };

	fs[1].v[0] = Vertex3D{ vd[4]->X, vd[4]->Y, vd[4]->Z };
	fs[1].v[1] = Vertex3D{ vd[7]->X, vd[7]->Y, vd[7]->Z };
	fs[1].v[2] = Vertex3D{ vd[6]->X, vd[6]->Y, vd[6]->Z };
	fs[1].v[3] = Vertex3D{ vd[5]->X, vd[5]->Y, vd[5]->Z };

	fs[2].v[0] = Vertex3D{ vd[0]->X, vd[0]->Y, vd[0]->Z };
	fs[2].v[1] = Vertex3D{ vd[3]->X, vd[3]->Y, vd[3]->Z };
	fs[2].v[2] = Vertex3D{ vd[7]->X, vd[7]->Y, vd[7]->Z };
	fs[2].v[3] = Vertex3D{ vd[4]->X, vd[4]->Y, vd[4]->Z };

	fs[3].v[0] = Vertex3D{ vd[1]->X, vd[1]->Y, vd[1]->Z };
	fs[3].v[1] = Vertex3D{ vd[5]->X, vd[5]->Y, vd[5]->Z };
	fs[3].v[2] = Vertex3D{ vd[6]->X, vd[6]->Y, vd[6]->Z };
	fs[3].v[3] = Vertex3D{ vd[2]->X, vd[2]->Y, vd[2]->Z };

	fs[4].v[0] = Vertex3D{ vd[0]->X, vd[0]->Y, vd[0]->Z };
	fs[4].v[1] = Vertex3D{ vd[4]->X, vd[4]->Y, vd[4]->Z };
	fs[4].v[2] = Vertex3D{ vd[5]->X, vd[5]->Y, vd[5]->Z };
	fs[4].v[3] = Vertex3D{ vd[1]->X, vd[1]->Y, vd[1]->Z };

	fs[5].v[0] = Vertex3D{ vd[3]->X, vd[3]->Y, vd[3]->Z };
	fs[5].v[1] = Vertex3D{ vd[2]->X, vd[2]->Y, vd[2]->Z };
	fs[5].v[2] = Vertex3D{ vd[6]->X, vd[6]->Y, vd[6]->Z };
	fs[5].v[3] = Vertex3D{ vd[7]->X, vd[7]->Y, vd[7]->Z };
	//----------------------------------------------------

	return fs;
}

void GeoGrid::computeCellVolumes(QString variable_name)
{
	//Get pointer to the Cartesian grid object used as data store
	CartesianGrid* myCartesianGrid = getUnderlyingCartesianGrid();

	//load the data
	myCartesianGrid->loadData();

	//appends a new variable
	myCartesianGrid->addEmptyDataColumn( variable_name, myCartesianGrid->getDataLineCount() );

	//get the current data column count
	uint columnCount = myCartesianGrid->getDataColumnCount();

	//load grid mesh
	loadMesh();


	//compute the volumes
	for( uint k = 0; k < m_nK; ++k )
		for( uint j = 0; j < m_nJ; ++j )
			for( uint i = 0; i < m_nI; ++i ){
				uint cellIndex = IJKtoIndex( i, j, k );
				Hexahedron hexa = makeHexahedron( cellIndex );
				double cellVolume = hexa.getVolume();
				myCartesianGrid->setData( cellIndex, columnCount-1, cellVolume );
			}

	//commit results to file system
	myCartesianGrid->writeToFS();
}

CartesianGrid *GeoGrid::getUnderlyingCartesianGrid()
{
	for( int i = 0; i < getChildCount(); ++i ){
		ProjectComponent* pc = getChildByIndex( i );
		if( pc->getTypeName() == "CARTESIANGRID" )
			return dynamic_cast<CartesianGrid*>( pc );
	}
	return nullptr;
}

Hexahedron GeoGrid::makeHexahedron( uint cellIndex )
{
	Hexahedron hexa;
	uint vertexIndexes[8];
	getMeshCellDefinition( cellIndex, vertexIndexes );
	double x, y, z;
	for( uint i = 0; i < 8; ++i ){
		getMeshVertexLocation( vertexIndexes[i], x, y, z );
		hexa.v[i].x = x;
		hexa.v[i].y = y;
		hexa.v[i].z = z;
	}
	return hexa;
}

void GeoGrid::IJKtoXYZ(uint i, uint j, uint k, double & x, double & y, double & z) const
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
	//TODO: Performance: maybe we can compute the center once and cache it to avoid repetive
	// cell center calculations
	double meanX = 0.0;
	double meanY = 0.0;
	double meanZ = 0.0;
	for( uint k = 0; k > m_nK; ++k )
		for( uint j = 0; j > m_nJ; ++j )
			for( uint i = 0; i > m_nI; ++i ){
				double x, y, z;
				IJKtoXYZ( i, j, k, x, y, z );
				meanX += x;
				meanY += y;
				meanZ += z;
			}
	int nCells = m_nI * m_nJ * m_nK;
	return SpatialLocation( meanX / nCells, meanY / nCells, meanZ / nCells );
}

bool GeoGrid::XYZtoIJK( double x, double y, double z, uint& i, uint& j, uint& k )
{
	if( m_spatialIndex->isEmpty() )
		m_spatialIndex->fill( this );

    assert( ! m_spatialIndex->isEmpty() && "GeoGrid::XYZtoIJK(): the spatial index is not supposed to be"
                                           " empty when calling this method." );

	//Get the nearest cells.
	QList<uint> cellIndexes = m_spatialIndex->getNearest( x, y, z, 5 );

	//if the spatial search failed, assumes it fell outside the grid
	if( cellIndexes.empty() )
		return false;

	//the test location
	Vertex3D p{ x, y, z };

	//for each one of the closest cells returned by the spatial index
	QList<uint>::iterator itCellIndex = cellIndexes.begin();
	for( ; itCellIndex != cellIndexes.end(); ++itCellIndex ){
		//get the cell's faces geometry .
		std::vector<Face3D> fs = getFaces( *itCellIndex );
		//if the location is inside the cell
		if( Util::isInside( p, fs ) ){
			//return the cell's topological coordinates
			this->indexToIJK( *itCellIndex, i, j, k );
			return true;
		}
	}

	//if execution reaches this point, the search failed.
	return false;
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

void GeoGrid::getDataSpatialLocation(uint line, double &x, double &y, double &z)
{
    uint i, j, k;
    indexToIJK( line, i, j, k );
    IJKtoXYZ( i, j, k, x, y, z);
}

double GeoGrid::getProportion(int variableIndex, double value0, double value1)
{
    throw new InvalidMethodException();
}

void GeoGrid::freeLoadedData()
{
    // free the sample data from the underlying cartesian grid (data in UVW domain)
    CartesianGrid* UVW_aspect = getUnderlyingCartesianGrid();
    if( UVW_aspect )
        UVW_aspect->clearLoadedData();

    // free cell geometry data
    m_vertexesPart.clear();
    m_cellDefsPart.clear();
    //clear() does not guarantee memory is actually freed.
    std::vector< VertexRecordPtr >().swap( m_vertexesPart );
    std::vector< CellDefRecordPtr >().swap( m_cellDefsPart );

    // free spatial index data
    m_spatialIndex->clear();

    // call superclass's free data method.
    DataFile::freeLoadedData();
}

bool GeoGrid::canHaveMetaData()
{
	return true;
}

QString GeoGrid::getFileType() const
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

void GeoGrid::writeToFS()
{
	//Save the GEO-EAS data (Cartesian grid part)
	DataFile::writeToFS();
	//Save the mesh data (cell geometry part)
	saveMesh();
}

void GeoGrid::deleteFromFS()
{
	DataFile::deleteFromFS(); //delete data and metadata files
	//in addition, GeoGrids have a mesh file, which also needs to be deleted.
	QFile file(this->getMeshFilePath());
	file.remove(); // TODO: throw exception if remove() returns false (fails).  Also see
				   // QIODevice::errorString() to see error message.
}

QIcon GeoGrid::getIcon()
{
	if( m_nreal < 2)
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

