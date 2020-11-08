#include <QTextStream>
#include <QFile>
#include "cartesiangrid.h"
#include "../util.h"
#include "attribute.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "geostats/gridcell.h"
#include "imagejockey/svd/svdfactor.h"
#include "viewer3d/view3dviewdata.h"
#include "viewer3d/view3dbuilders.h"
#include "domain/application.h"
#include "geogrid.h"
#include "domain/section.h"
#include "domain/pointset.h"

#include "spectral/spectral.h" //eigen third party library

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/scoped_array.hpp>

CartesianGrid::CartesianGrid( QString path )  : GridFile( path ), IJAbstractCartesianGrid()
{
    this->_dx = 0.0;
    this->_dy = 0.0;
    this->_dz = 0.0;
    this->_no_data_value = "";
	this->m_nreal = 0;
	this->m_nI = 0;
	this->m_nJ = 0;
	this->m_nK = 0;
    this->_rot = 0.0;
    this->_x0 = 0.0;
    this->_y0 = 0.0;
    this->_z0 = 0.0;
}

void CartesianGrid::setInfo(double x0, double y0, double z0,
                            double dx, double dy, double dz,
                            int nx, int ny, int nz, double rot, int nreal, const QString no_data_value,
                            QMap<uint, QPair<uint, QString> > nvar_var_trn_triads,
                            const QList< QPair<uint,QString> > &categorical_attributes)
{
    //updating metadata
    this->_dx = dx;
    this->_dy = dy;
    this->_dz = dz;
    this->_no_data_value = no_data_value;
	this->m_nreal = nreal;
	this->m_nI = nx;
	this->m_nJ = ny;
	this->m_nK = nz;
    this->_rot = rot;
    this->_x0 = x0;
    this->_y0 = y0;
    this->_z0 = z0;
    _nsvar_var_trn.clear();
    _nsvar_var_trn.unite( nvar_var_trn_triads );
    _categorical_attributes.clear();
    _categorical_attributes << categorical_attributes;

    this->updateChildObjectsCollection();
}

void CartesianGrid::setInfoFromMetadataFile()
{
    QString md_file_path( this->_path );
    QFile md_file( md_file_path.append(".md") );
    double x0 = 0.0, y0 = 0.0, z0 = 0.0;
    double dx = 0.0, dy = 0.0, dz = 0.0;
    uint nx = 0, ny = 0, nz = 0;
    double rot = 0.0;
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
           if( line.startsWith( "X0:" ) ){
               QString value = line.split(":")[1];
               x0 = value.toDouble();
           }else if( line.startsWith( "Y0:" ) ){
               QString value = line.split(":")[1];
               y0 = value.toDouble();
           }else if( line.startsWith( "Z0:" ) ){
               QString value = line.split(":")[1];
               z0 = value.toDouble();
           }else if( line.startsWith( "NX:" ) ){
               QString value = line.split(":")[1];
               nx = value.toInt();
           }else if( line.startsWith( "NY:" ) ){
               QString value = line.split(":")[1];
               ny = value.toInt();
           }else if( line.startsWith( "NZ:" ) ){
               QString value = line.split(":")[1];
               nz = value.toInt();
           }else if( line.startsWith( "DX:" ) ){
               QString value = line.split(":")[1];
               dx = value.toDouble();
           }else if( line.startsWith( "DY:" ) ){
               QString value = line.split(":")[1];
               dy = value.toDouble();
           }else if( line.startsWith( "DZ:" ) ){
               QString value = line.split(":")[1];
               dz = value.toDouble();
           }else if( line.startsWith( "ROT:" ) ){
               QString value = line.split(":")[1];
               rot = value.toDouble();
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
        this->setInfo( x0, y0, z0, dx, dy, dz, nx, ny, nz, rot, nreal, ndv, nsvar_var_trn, categorical_attributes);
    }
}

void CartesianGrid::setInfoFromOtherCG(CartesianGrid *other_cg, bool copyCategoricalAttributesList)
{
    double x0 = 0.0, y0 = 0.0, z0 = 0.0;
    double dx = 0.0, dy = 0.0, dz = 0.0;
    uint nx = 0, ny = 0, nz = 0;
    double rot = 0.0;
    uint nreal = 0;
    QString ndv;
    QMap<uint, QPair<uint, QString> > nsvar_var_trn_triads;
    QList< QPair<uint, QString> > categorical_attributes;
    x0 = other_cg->getX0();
    y0 = other_cg->getY0();
    z0 = other_cg->getZ0();
    nx = other_cg->getNX();
    ny = other_cg->getNY();
    nz = other_cg->getNZ();
    dx = other_cg->getDX();
    dy = other_cg->getDY();
    dz = other_cg->getDZ();
    rot = other_cg->getRot();
    nreal = other_cg->getNReal();
    ndv = other_cg->getNoDataValue();
    nsvar_var_trn_triads = other_cg->getNSVarVarTrnTriads();
    if( copyCategoricalAttributesList )
        categorical_attributes = other_cg->getCategoricalAttributes();
    this->setInfo( x0, y0, z0, dx, dy, dz, nx, ny, nz, rot, nreal,
				   ndv, nsvar_var_trn_triads, categorical_attributes);
}

void CartesianGrid::setInfoFromSVDFactor(const SVDFactor * factor)
{
	double x0 = 0.0, y0 = 0.0, z0 = 0.0;
	double dx = 0.0, dy = 0.0, dz = 0.0;
	uint nx = 0, ny = 0, nz = 0;
	double rot = 0.0;
	uint nreal = 0;
	QString ndv;
	QMap<uint, QPair<uint, QString> > nsvar_var_trn_triads;
	QList< QPair<uint, QString> > categorical_attributes;
	x0 = factor->getOriginX();
	y0 = factor->getOriginY();
	z0 = factor->getOriginZ();
	nx = factor->getNI();
	ny = factor->getNJ();
	nz = factor->getNK();
	dx = factor->getCellSizeI();
	dy = factor->getCellSizeJ();
	dz = factor->getCellSizeK();
	rot = factor->getRotation();
	nreal = 1;
	ndv = "";
	this->setInfo( x0, y0, z0, dx, dy, dz, nx, ny, nz, rot, nreal,
				   ndv, nsvar_var_trn_triads, categorical_attributes);
}

void CartesianGrid::setInfoFromGridParameter(GSLibParGrid *pg)
{
    double x0 = 0.0, y0 = 0.0, z0 = 0.0;
    double dx = 0.0, dy = 0.0, dz = 0.0;
    uint nx = 0, ny = 0, nz = 0;
    double rot = 0.0;
    uint nreal = 0;
    QString ndv;
    QMap<uint, QPair<uint, QString> > empty;
    QList< QPair<uint,QString> > empty2;

    nx = pg->_specs_x->getParameter<GSLibParUInt*>(0)->_value; //nx
    x0 = pg->_specs_x->getParameter<GSLibParDouble*>(1)->_value; //min x
    dx = pg->_specs_x->getParameter<GSLibParDouble*>(2)->_value; //cell size x
    ny = pg->_specs_y->getParameter<GSLibParUInt*>(0)->_value; //ny
    y0 = pg->_specs_y->getParameter<GSLibParDouble*>(1)->_value; //min y
    dy = pg->_specs_y->getParameter<GSLibParDouble*>(2)->_value; //cell size y
    nz = pg->_specs_z->getParameter<GSLibParUInt*>(0)->_value; //nz
    z0 = pg->_specs_z->getParameter<GSLibParDouble*>(1)->_value; //min z
    dz = pg->_specs_z->getParameter<GSLibParDouble*>(2)->_value; //cell size z

    rot = 0.0;
    nreal = 1;
    ndv = "";

	this->setInfo( x0, y0, z0, dx, dy, dz, nx, ny, nz, rot, nreal, ndv, empty, empty2);
}

void CartesianGrid::setInfoFromGeoGrid(GeoGrid * gg)
{

	//computes cell size so we have a 1.0 x 1.0 x 1.0 cube in depositional space (UVW).
	double dx = 1.0 / gg->getNI();
	double dy = 1.0 / gg->getNJ();
	double dz = 1.0 / gg->getNK();

	setInfo( 0.0, 0.0, 0.0, dx, dy, dz,
			gg->getNI(), gg->getNJ(), gg->getNK(),
			0.0, gg->getNumberOfRealizations(), gg->getNoDataValue(),
			gg->getNSVarVarTrnTriads(), gg->getCategoricalAttributes() );
}

void CartesianGrid::setCellGeometry(int nx, int ny, int nz, double dx, double dy, double dz)
{
	this->setInfo( _x0, _y0, _z0, dx, dy, dz, nx, ny, nz, _rot, m_nreal,
                   _no_data_value, _nsvar_var_trn, _categorical_attributes);
}

std::vector<std::vector<double> > CartesianGrid::getResampledValues(int rateI, int rateJ, int rateK,
                                                                    int &finalNI, int &finalNJ, int &finalNK)
{
    std::vector< std::vector<double> > result;

	uint totDataLinesPerRealization = m_nI * m_nJ * m_nK;

    //load just the first line to get the number of columns (assuming the file is right)
    setDataPage( 0, 0 );
    uint nDataColumns = getDataColumnCount();

	result.reserve( m_nreal * (m_nI/rateI) * (m_nJ/rateJ) * (m_nK/rateK) * nDataColumns );

    //for each realization (at least one)
	for( uint r = 0; r < m_nreal; ++r ){
        //compute the first and last data lines to load (does not need to load everything at once)
        ulong firstDataLine = r * totDataLinesPerRealization;
        ulong lastDataLine = firstDataLine + totDataLinesPerRealization - 1;
        //load the data corresponding to a realization
        setDataPage( firstDataLine, lastDataLine );
        loadData();
        //perform the resampling for a realization
        finalNK = 0;
		for( uint k = 0; k < m_nK; k += rateK, ++finalNK ){
            finalNJ = 0;
			for( uint j = 0; j < m_nJ; j += rateJ, ++finalNJ ){
                finalNI = 0;
				for( uint i = 0; i < m_nI; i += rateI, ++finalNI ){
                    std::vector<double> dataLine;
                    dataLine.reserve( nDataColumns );
                    for( uint d = 0; d < nDataColumns; ++d){
                        dataLine.push_back( dataIJK( d, i, j, k ) );
                    }
                    result.push_back( dataLine );
                }
            }
        }
    }

    //TODO: remove this when all GammaRay features become realization-aware.
    setDataPageToAll();

    return result;
}


double CartesianGrid::getRotation() const
{
    return getRot();
}

double CartesianGrid::getData(int variableIndex, int i, int j, int k)
{
	return dataIJK( variableIndex, i, j, k );
}

bool CartesianGrid::isNoDataValue(double value)
{
    return isNDV( value );
}

double CartesianGrid::getDataAt(int dataColumn, double x, double y, double z)
{
    return valueAt( dataColumn, x, y, z );
}

double CartesianGrid::absMin(int column)
{
	return minAbs( column );
}

void CartesianGrid::dataWillBeRequested()
{
    loadData();
}

QString CartesianGrid::getGridName() const
{
    return getName();
}

QIcon CartesianGrid::getGridIcon()
{
    return getIcon();
}

int CartesianGrid::getVariableIndexByName(QString variableName)
{
    return getFieldGEOEASIndex( variableName ) - 1;
}

IJAbstractVariable *CartesianGrid::getVariableByName(QString variableName)
{
    ProjectComponent* pc = getChildByName( variableName );
	if( pc && pc->isAttribute() )
        return dynamic_cast<Attribute*>(pc);
    else
        return nullptr;
}

void CartesianGrid::getAllVariables(std::vector<IJAbstractVariable *> &result)
{
    std::vector<ProjectComponent*> all_contained_objects;
    getAllObjects( all_contained_objects );
    std::vector<ProjectComponent*>::iterator it = all_contained_objects.begin();
    for(; it != all_contained_objects.end(); ++it){
        ProjectComponent* pc = *it;
		if( pc->isAttribute() ){
            result.push_back( dynamic_cast<Attribute*>(pc) );
        }
    }
}

IJAbstractVariable *CartesianGrid::getVariableByIndex(int variableIndex)
{
    return getAttributeFromGEOEASIndex( variableIndex+1 );
}

double CartesianGrid::absMax(int column)
{
    return maxAbs( column );
}

SpatialLocation CartesianGrid::getCenter()
{
    SpatialLocation result;
    //TODO: add support for rotations
    if( ! Util::almostEqual2sComplement( this->_rot, 0.0, 1) ){
        Application::instance()->logError("CartesianGrid::getCenter(): rotation not supported yet.  Returning NDV or NaN.");
        double errorValue;
        if( this->hasNoDataValue() )
            errorValue = getNoDataValueAsDouble();
        else
            errorValue = std::numeric_limits<double>::quiet_NaN();
        result._x = errorValue;
        result._y = errorValue;
        result._z = errorValue;
        return result;
    }
	double x0 = _x0 - _dx / 2.0;
	double y0 = _y0 - _dy / 2.0;
	double z0 = _z0 - _dz / 2.0;
	double xf = x0 + _dx * m_nI;
	double yf = y0 + _dy * m_nJ;
	double zf = z0 + _dz * m_nK;
    result._x = (xf + x0) / 2.0;
    result._y = (yf + y0) / 2.0;
    result._z = (zf + z0) / 2.0;
    return result;
}

void CartesianGrid::equalizeValues(QList<QPointF> &area, double delta_dB, int dataColumn, double dB_reference,
                                   const QList<QPointF> &secondArea)
{
    //some typedefs to shorten code
    typedef boost::geometry::model::d2::point_xy<double> boostPoint2D;
    typedef boost::geometry::model::polygon<boostPoint2D> boostPolygon;

    //define a Boost polygon from the area geometry points
    const std::size_t n = area.size();
    boost::scoped_array<boostPoint2D> points(new boostPoint2D[n]); //scoped_array frees memory when its scope ends.
    for(std::size_t i = 0; i < n; ++i)
        points[i] = boostPoint2D( area[i].x(), area[i].y() );
    boostPolygon poly;
    boost::geometry::assign_points( poly, std::make_pair(&points[0], &points[0] + n));

    //define a Boost polygon from the second area (if not empty)
    boostPolygon secondPoly;
    if( ! secondArea.empty() ){
        const std::size_t n = secondArea.size();
        boost::scoped_array<boostPoint2D> secondPoints(new boostPoint2D[n]); //scoped_array frees memory when its scope ends.
        for(std::size_t i = 0; i < n; ++i)
            secondPoints[i] = boostPoint2D( secondArea[i].x(), secondArea[i].y() );
        boost::geometry::assign_points( secondPoly, std::make_pair(&secondPoints[0], &secondPoints[0] + n));
    }

    //get the 2D bounding box of the polygon
    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();
    QList<QPointF>::iterator it = area.begin();
    for( ; it != area.end(); ++it){
        minX = std::min<double>( minX, (*it).x() );
        maxX = std::max<double>( maxX, (*it).x() );
        minY = std::min<double>( minY, (*it).y() );
        maxY = std::max<double>( maxY, (*it).y() );
    }

    //scan the grid, testing each cell whether it lies within the area.
    //TODO: this code assumes no grid rotation and that the grid is 2D.
    for( uint k = 0; k < getNZ(); ++k ){
        // z coordinate is ignored in 2D spectrograms
        for( uint j = 0; j < getNY(); ++j ){
            double cellCenterY = getY0() + j * getDY();
            for( uint i = 0; i < getNX(); ++i ){
                double cellCenterX = getX0() + i * getDX();
                boostPoint2D p(cellCenterX, cellCenterY);
                // if the cell center lies within the area
                // The bounding box test is a faster test to promplty discard cells obviously outside.
                if(     Util::isWithinBBox( cellCenterX, cellCenterY, minX, minY, maxX, maxY )
                        &&
                        boost::geometry::within(p, poly)
                        &&
                        ( secondArea.isEmpty() || boost::geometry::within(p, secondPoly) )   ){
                    // get the grid value as is
                    double value = dataIJK( dataColumn, i, j, k );
                    // determine whether the value is negative
                    bool isNegative = value < 0.0;
                    // get the absolute value
                    value = std::abs(value);
                    // get the absolute value in dB
                    double value_dB = Util::dB( value, dB_reference, 0.00001);
                    // apply adjustment in dB
                    value_dB += delta_dB;
                    // attenuate/amplify the absolute value
					value = std::pow( 10.0, value_dB / DECIBEL_SCALE_FACTOR ) * dB_reference;
                    // add negative sign if the original value was negative
                    if( isNegative )
                        value = -value;
                    // set the amplified/attenuated value to the grid
                    setDataIJK( dataColumn, i, j, k, value );
                }
            }
        }
    }
}

void CartesianGrid::saveData()
{
    writeToFS();
}

bool CartesianGrid::XYZtoIJK(double x, double y, double z, uint &i, uint &j, uint &k)
{
    //TODO: add support for rotations
    if( ! Util::almostEqual2sComplement( this->_rot, 0.0, 1) ){
        Application::instance()->logError("CartesianGrid::XYZtoIJK(): rotation not supported yet.  Returning false (invalid return values).");
        return false;
    }

    //compute the indexes from the spatial location.
    double xWest = _x0 - _dx/2.0;
    double ySouth = _y0 - _dy/2.0;
    double zBottom = _z0 - _dz/2.0;
    i = (x - xWest) / _dx;
    j = (y - ySouth) / _dy;
    k = 0;
	if( m_nK > 1 )
        k = (z - zBottom) / _dz;

    //check whether the location is outside the grid
	if( /*i < 0 ||*/ i >= m_nI || /*j < 0 ||*/ j >= m_nJ || /*k < 0 ||*/ k >= m_nK ){
        return false;
    }
	return true;
}

void CartesianGrid::IJKtoXYZ(uint i, uint j, uint k, double &x, double &y, double &z) const
{
    x = _x0 + _dx * i;
    y = _y0 + _dy * j;
	if( m_nK > 1 )
        z = _z0 + _dz * k;
	else
		z = 0.0; //2D grids are positioned at Z=0.0 by convention
}

bool CartesianGrid::canHaveMetaData()
{
    return true;
}

QString CartesianGrid::getFileType() const
{
    return "CARTESIANGRID";
}

void CartesianGrid::updateMetaDataFile()
{
	//If this object does not have a physical file on its own, it doesn't have a metadata file.
	//Instead, its metadata is set on the fly by its parent GeoGrid (see DataFile::updatePropertyCollection() method).
	//The Cartesian grid physical file belongs to the parent GeoGrid object and this one holds its metadata file.
	if( isUVWOfAGeoGrid() )
		return;

    QFile file( this->getMetaDataFilePath() );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);
    out << APP_NAME << " metadata file.  This file is generated automatically.  Do not edit this file.\n";
    out << "version=" << APP_VERSION << '\n';
    out << "X0:" << QString::number( this->_x0, 'g', 12) << '\n';
    out << "Y0:" << QString::number( this->_y0, 'g', 12) << '\n';
    out << "Z0:" << QString::number( this->_z0, 'g', 12) << '\n';
	out << "NX:" << this->m_nI << '\n';
	out << "NY:" << this->m_nJ << '\n';
	out << "NZ:" << this->m_nK << '\n';
    out << "DX:" << this->_dx << '\n';
    out << "DY:" << this->_dy << '\n';
    out << "DZ:" << this->_dz << '\n';
    out << "ROT:" << this->_rot << '\n';
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

QIcon CartesianGrid::getIcon()
{
	if( m_nreal == 1){
		if(m_nK == 1){
            if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
                return QIcon(":icons/cartesiangrid16");
            else
                return QIcon(":icons32/cartesiangrid32");
        }else{
            if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
                return QIcon(":icons/cg3D16");
            else
                return QIcon(":icons32/cg3D32");
        }
    } else {
		if(m_nK == 1){
            return QIcon(":icons32/cartesiangridN32");
        }else{
            return QIcon(":icons32/cartesiangrid_3DN_32");
        }
    }
}

void CartesianGrid::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
    //also saves the metadata file.
    this->updateMetaDataFile();
}

View3DViewData CartesianGrid::build3DViewObjects(View3DWidget *widget3D)
{
	return View3DBuilders::build( this, widget3D );
}

QString CartesianGrid::getPresentationName()
{
	if( this->getParent()->isFile() ){
		File* file = dynamic_cast<File*>( this->getParent() );
		if( file->getFileType() == "GEOGRID")
			return GridFile::getPresentationName() + " (UVW)";
	}
	return GridFile::getPresentationName();
}

spectral::array *CartesianGrid::createSpectralArray(int nDataColumn)
{
	spectral::array* data = new spectral::array( m_nI, m_nJ, m_nK, 0.0 );
    long idx = 0;
	for (ulong i = 0; i < m_nI; ++i) {
		for (ulong j = 0; j < m_nJ; ++j) {
			for (ulong k = 0; k < m_nK; ++k) {
                double value = dataIJK(nDataColumn, i, j, k);
                if( ! isNDV( value ) )
                    data->d_[idx] = value ;
                else
                    data->d_[idx] =  std::numeric_limits<double>::quiet_NaN();;
                ++idx;
            }
        }
    }
    return data;
}

spectral::complex_array *CartesianGrid::createSpectralComplexArray(int variableIndex1, int variableIndex2)
{
	spectral::complex_array* data = new spectral::complex_array( m_nI, m_nJ, m_nK );
    long idx = 0;
	for (ulong i = 0; i < m_nI; ++i) {
		for (ulong j = 0; j < m_nJ; ++j) {
			for (ulong k = 0; k < m_nK; ++k) {
                data->d_[idx][0] = dataIJK(variableIndex1, i, j, k);
                data->d_[idx][1] = dataIJK(variableIndex2, i, j, k);
                ++idx;
            }
        }
    }
    return data;
}

void CartesianGrid::clearLoadedData()
{
    freeLoadedData();
}

long CartesianGrid::appendAsNewVariable(const QString variableName, const spectral::array &array)
{
    return append( variableName, array );
}

void CartesianGrid::getCellLocation(int i, int j, int k, double &x, double &y, double &z) const
{
    IJKtoXYZ( i, j, k, x, y, z );
}

double CartesianGrid::getMax(int column)
{
    return max( column );
}

double CartesianGrid::getMin(int column)
{
    return min( column );
}

void CartesianGrid::getSpatialAndTopologicalCoordinates(int iRecord, double & x, double & y, double & z, int & i, int & j, int & k)
{
    if( isUVWOfAGeoGrid() ){
        GeoGrid* parentGeoGrid = dynamic_cast< GeoGrid* >( getParent() );
        parentGeoGrid->getSpatialAndTopologicalCoordinates( iRecord, x, y, z, i, j, k );
    } else if (  isDataStoreOfaGeologicSection() ){
        indexToIJK( iRecord, (uint&)i, (uint&)j, (uint&)k );
        Section* parentSection = dynamic_cast< Section* >( getParent() );
        parentSection->IKtoXYZ( i, k, x, y, z );
    } else {
        indexToIJK( iRecord, (uint&)i, (uint&)j, (uint&)k );
        x = _x0 + i * _dx;
        y = _y0 + j * _dy;
        z = _z0 + k * _dz;
    }
}

void CartesianGrid::computationWillStart()
{
    GridFile::computationWillStart(); //the usual call to loadData() for the grid.

    //If this grid is the data store of a geologic section...
    if( isDataStoreOfaGeologicSection() ){
        //...also loads the point set file with the geometry information (required
        //for the calculator to work such as obtaining spatial coordinates).
        Section* parentSection = dynamic_cast<Section*>( getParent() );
        PointSet* pointSetSibling = parentSection->getPointSet();
        if( pointSetSibling )
            pointSetSibling->loadData();
        else
            Application::instance()->logError("CartesianGrid::computationWillStart(): This grid file belongs to a "
                                              "geologic section.  The point set file defining the geometry was not "
                                              "found.  It is necessary to compute necessary information for the "
                                              "calculator to work.");
    }

    //If this grid is the data store of a geologic grid...
    if( isUVWOfAGeoGrid() ){
        //...also loads the mesh file with the geometry information (required
        //for the calculator to work such as obtaining spatial coordinates).
        GeoGrid* parentGeoGrid = dynamic_cast<GeoGrid*>( getParent() );
        parentGeoGrid->loadMesh();
    }
}


void CartesianGrid::setOrigin(double x0, double y0, double z0)
{
    _x0 = x0;
    _y0 = y0;
	_z0 = z0;
}

bool CartesianGrid::isUVWOfAGeoGrid()
{
	if( this->getParent() && this->getParent()->isFile() ){
		File* parentFileAspect = dynamic_cast<File*>( this->getParent() );
		return parentFileAspect->getFileType() == "GEOGRID";
	}
    return false;
}

bool CartesianGrid::isDataStoreOfaGeologicSection()
{
    if( this->getParent() && this->getParent()->isFile() ){
        File* parentFileAspect = dynamic_cast<File*>( this->getParent() );
        return parentFileAspect->getFileType() == "SECTION";
    }
    return false;
}

double CartesianGrid::getDataSpatialLocation(uint line, CartesianCoord whichCoord)
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

void CartesianGrid::getDataSpatialLocation(uint line, double &x, double &y, double &z)
{
    uint i, j, k;
    indexToIJK( line, i, j, k );
    IJKtoXYZ( i, j, k, x, y, z);
}

bool CartesianGrid::isTridimensional()
{
	return m_nK > 1;
}
