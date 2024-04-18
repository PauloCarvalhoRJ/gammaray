#ifndef GEOGRID_H
#define GEOGRID_H

#include "gridfile.h"
#include "geometry/face3d.h"
#include "geometry/hexahedron.h"

class CartesianGrid;
class SpatialIndex;
class PointSet;
class SegmentSet;

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


/**
 * Structure used as parameter for multi-zone GeoGrid constructors.
 */
struct GeoGridZone {
    Attribute* top;
    Attribute* base;
    int nHorizonSlices;
};

/////////////////////////////////////////////////  The GeoGrid class ///////////////////////////////////////////////////////////

/** The GeoGrid is an irregular structured grid.  See the program manual for more in-depth
 * details of the GeoGrid and how it can be used with GSLib programs designed for redgular Cartesian grids.*/
class GeoGrid : public GridFile
{

public:

	/** Constructor of an empty GeoGrid (no data, no mesh). **/
	GeoGrid( QString path );

	/** The simplest geometry initializing constructor.  Creates its geometry from top and base values
	 * of a map (2D CartesianGrid).  Use this constructor for new grids only.  Do not use this to load
	 * existing ones.
	 * @param path The file path where the data file (GEO-EAS cartesian grid data file) will be saved
	 * @param nHorizonSlices The number of cell layers between top and base.  Minimum is 1.
	 */
	GeoGrid( QString path, Attribute* atTop, Attribute* atBase, uint nHorizonSlices );

    /** Initializes the geometry as proportional to the tops and bases passed as zones.  The top and base
     * attributes must come from the same map (2D CartesianGrid).  Use this constructor for new grids only.
     * Do not use this to load existing ones.
     * @param path The file path where the data file (GEO-EAS cartesian grid data file) will be saved
     */
    GeoGrid( QString path, std::vector<GeoGridZone> zones );

	/**
	 * Returns (via output parameters) the bounding box of a cell given its cell index.
	 */
    void getCellBoundingBox(uint cellIndex,
                            double& minX, double& minY, double& minZ,
                            double& maxX, double& maxY, double& maxZ ) const;

	/** Returns the path to the file that stores the geometry data, that is,
	 * the contents of the m_vertexesPart and m_cellDefsPart members.
	 */
	QString getMeshFilePath();

	/**
	 * Saves the geometry data to file system.
	 */
	void saveMesh();

	/**
	 * Loads the grid's mesh.
	 */
	void loadMesh();

	/** Sets GeoGrid metadata from the accompaining .md file, if it exists.
	 Nothing happens if the metadata file does not exist.  If it exists, it calls
	 #setInfo() with the metadata read from the .md file.*/
	void setInfoFromMetadataFile();

	/** Sets GeoGrid metadata.  It also populates the file's attribute collection. */
	void setInfo( int nI, int nJ, int nK,
				  int nreal, const QString no_data_value,
				  QMap<uint, QPair<uint, QString> > nvar_var_trn_triads,
				  const QList<QPair<uint, QString> > &categorical_attributes);

	/** Returns the number of vertexes in the GeoGrid's mesh. */
	uint getMeshNumberOfVertexes();

	/** Returns, via output parameters, the coordinates of a mesh vertex given its id (index). */
    void getMeshVertexLocation( uint index, double& x, double& y, double& z ) const;

	/** Returns the number of cells in the GeoGrid's mesh. */
	uint getMeshNumberOfCells();

    /** Returns, via output parameter, the indexes of mesh vertexes of a cell given its id (index). */
    void getMeshCellDefinition( uint index, uint (&vIds)[8] ) const;

    /** Returns a new point set by transforming the input point set from XYZ space to UVW space.
     * Returns null pointer if unfold fails for any reason.
     * Points outside the grid mesh are removed from the result.
     */
    PointSet* unfold( PointSet* inputPS, QString nameForNewPointSet );

    /** Returns a new segment set by transforming the input segment set from XYZ space to UVW space.
     * Returns null pointer if unfold fails for any reason.
     * Segments with an end falling outside the grid mesh are removed from the result.
     */
    SegmentSet* unfold( SegmentSet* inputSS, QString nameForNewSegmentSet );

    /**
     * Converts a global XYZ coordinate into the grid's homogeneous (values between [0, 1])
     * depositional UVW coordinate.  If the input coordinate is outside the grid mesh, then
     * the function returns false.
     */
    bool XYZtoUVW(  double x,  double y,  double z,
                   double& u, double& v, double& w );

	/**
	 * Creates and returns a vector containing the geometries of the six faces of a cell.
         * The vertexes in the returned faces are ordered so the faces are all counter-clockwise
         * as seen from the inside, which is the original order per the convention of the GeoGrid.
	 */
	std::vector<Face3D> getFaces( uint cellIndex );

        /**
         * Does the same as getFaces(), but the returned faces are all counter-clockwise
         * as seen from the OUTSIDE, which is the opposite of the original order per the
         * convention of the GeoGrid.
         */
        std::vector<Face3D> getFacesInvertedWinding( uint cellIndex );


	/**
	 * Adds a new variable containing the volumes of the cells.  The values can be useful
	 * for debiasing, for instance.
	 */
	void computeCellVolumes( QString variable_name );

	/**
	 * Returns the pointer to the CartesianGrid object that holds GeoGrid's data.
	 * Returns a null pointer if no CartesianGrid is found among its child objects.
	 * Returns the first CartesianGrid object in _children container if there is more
	 * than one attached to this object.
	 */
	CartesianGrid* getUnderlyingCartesianGrid();

	/**
	 * Creates a Hexahedron object from a cell's geometry data given the cell's index.
	 */
    Hexahedron makeHexahedron( uint cellIndex ) const;

    /**
     * Exports this GeoGrid as ASCII Eclipse Grid (.grdecl format).
     * This format is broadly supported by most geomodeling software.
     * @param invertSignZ if true (default), inverts the sign of the Z coordinates. The Eclipse Grid format
     *        Works with the inverted Z axis, that is, positive depth values mean bellow sea level, being a standard designed
     *        for the petroleum industry.  GammaRay, being a more general framework, works with the more usual
     *        Z axis pointing upwards, that is, depths below sea level translate to negative depth values.
     * @note This method currently assumes the geometry is pillar-grid like, that is,
     *       the cells are arranged in stacks.  A prior test for odd geometries should
     *       be implemented when new GeoGrid constructors or importers are made available.
     */
    void exportToEclipseGridGRDECL( const QString filePath, bool invertSignZ );

//GridFile interface
	virtual void IJKtoXYZ( uint i,    uint j,    uint k,
                           double& x, double& y, double& z ) const;
	virtual SpatialLocation getCenter();
	virtual bool XYZtoIJK( double x, double y, double z, uint &i, uint &j, uint &k );

//DataFile interface
public:
	/** GeoGrids never have declustering weights.  At least they are not supposed to have. */
	virtual bool isWeight( Attribute* /*at*/ ) { return false; }
	/** GeoGrids never have declustering weights.  At least they are not supposed to have. */
	virtual Attribute* getVariableOfWeight( Attribute* /*at*/ ) { return nullptr; }
	virtual bool isRegular() { return false; }
	virtual double getDataSpatialLocation( uint line, CartesianCoord whichCoord );
    virtual void   getDataSpatialLocation( uint line, double& x, double& y, double& z );
    /** GeoGrids are assumed to be always 3D. */
	virtual bool isTridimensional(){ return true; }
    /** NOTE: override the default counting-only behavior of DataFile::getProportion(). */
    virtual double getProportion(int variableIndex, double value0, double value1 );
    virtual void freeLoadedData();
    virtual BoundingBox getBoundingBox( ) const;

// File interface
public:
	virtual bool canHaveMetaData();
    virtual QString getFileType() const;
	virtual void updateMetaDataFile();
	virtual bool isDataFile(){ return true; }
	virtual void writeToFS();
	virtual void deleteFromFS();
    virtual bool isDistribution(){ return false; }
    virtual File* duplicatePhysicalFiles( const QString new_file_name );

// ProjectComponent interface
public:
	virtual QIcon getIcon();
	virtual void save(QTextStream *txt_stream);
	virtual View3DViewData build3DViewObjects( View3DWidget * widget3D );

// ICalcPropertyCollection interface
public:
	virtual void getSpatialAndTopologicalCoordinates(int iRecord, double& x, double& y, double& z, int& i, int& j, int& k );

private:
	//--------------mesh data-----------------------
	std::vector< VertexRecordPtr > m_vertexesPart;
	std::vector< CellDefRecordPtr > m_cellDefsPart;
	//----------------------------------------------
    std::unique_ptr< SpatialIndex > m_spatialIndex;

	/**
	 * Stores the file timestamp in the last call to loadMesh().
	 * This time is used to detect whether there is a change in the mesh file, to prevent
	 * unnecessary data reloads.
	 */
	QDateTime m_lastModifiedDateTimeLastMeshLoad;
};

typedef std::shared_ptr<GeoGrid> GeoGridPtr;

#endif // GEOGRID_H
