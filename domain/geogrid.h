#ifndef GEOGRID_H
#define GEOGRID_H

#include "datafile.h"

class CartesianGrid;

/** The data record holding the spatial position of a vertex.
 * It's id is the index in the m_vertexes container.
 */
typedef struct {
	double X;
	double Y;
	double Z;
} VertexRecord;
typedef std::shared_ptr< VertexRecord > VertexRecordPtr;


/**
 * The data record holding the ids of the vertexes forming the geometry of a cell.
 * The cell it refers to is identified by and id.  This id is the index in m_cellDefs
 * container.  The cell index is computed from its topological coordinates following
 * GSLib convention: K * nJ * nI + J * nI + I.  So the first element of index 0 in m_cellDefs
 * is the cell with I = 0; J = 0; K = 0.
 *
 * Vertex id elements in the vId[8] array forming the geometry of a cell:
 *
 *                        J
 *                        |
 *                        |
 *
 *                        3-----------2   face 0 = 0 1 2 3
 *                       /|          /|   face 1 = 4 7 6 5
 *                     /  |        /  |   face 2 = 0 3 7 4
 *                   /    |      /    |   face 3 = 1 5 6 2
 *                  7--------- 6      |   face 4 = 0 4 5 1
 *                  |     |    |      |   face 5 = 3 2 6 7
 *                  |     0----|------1    --- I
 *                  |    /     |     /
 *                  |  /       |   /
 *                  |/         | /
 *                  4--------- 5
 *
 *               /
 *             /
 *           K
 *
 *
 *
 */
typedef struct{
	int vId[8];
} CellDefRecord;
typedef std::shared_ptr< CellDefRecord > CellDefRecordPtr;

/////////////////////////////////////////////////  The GeoGrid class ///////////////////////////////////////////////////////////

class GeoGrid : public DataFile
{

public:
	GeoGrid();


//DataFile interface
public:
	/** GeoGrids never have declustering weights.  At least they are not supposed to have. */
	virtual bool isWeight( Attribute* /*at*/ ) { return false; }
	/** GeoGrids never have declustering weights.  At least they are not supposed to have. */
	virtual Attribute* getVariableOfWeight( Attribute* /*at*/ ) { return nullptr; }
	virtual bool isRegular() { return false; }
	virtual double getDataSpatialLocation( uint line, CartesianCoord whichCoord );
	/** GeoGrids are assumed to be always 3D. */
	virtual bool isTridimensional(){ return true; }

// File interface
public:
	virtual bool canHaveMetaData();
	virtual QString getFileType();
	virtual void updateMetaDataFile();
	virtual bool isDataFile(){ return true; }

// ProjectComponent interface
public:
	virtual QIcon getIcon();
	virtual void save(QTextStream *txt_stream);
	virtual View3DViewData build3DViewObjects( View3DWidget * widget3D );

// ICalcPropertyCollection interface
public:
	virtual void getSpatialAndTopologicalCoordinates(int iRecord, double& x, double& y, double& z, int& i, int& j, int& k );
	virtual double getNeighborValue( int iRecord, int iVar, int dI, int dJ, int dK );

private:
	CartesianGrid* m_cgPart;
	std::vector< VertexRecordPtr > m_vertexesPart;
	std::vector< CellDefRecordPtr > m_cellDefsPart;

};

#endif // GEOGRID_H
