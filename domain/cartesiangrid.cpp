#include <QTextStream>
#include <QFile>
#include "cartesiangrid.h"
#include "../util.h"
#include "attribute.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "geostats/gridcell.h"

#include "spectral/spectral.h" //eigen third party library

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/scoped_array.hpp>

CartesianGrid::CartesianGrid( QString path )  : DataFile( path ), IJAbstractCartesianGrid()
{
    this->_dx = 0.0;
    this->_dy = 0.0;
    this->_dz = 0.0;
    this->_no_data_value = "";
    this->_nreal = 0;
    this->_nx = 0;
    this->_ny = 0;
    this->_nz = 0;
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
    this->_nreal = nreal;
    this->_nx = nx;
    this->_ny = ny;
    this->_nz = nz;
    this->_rot = rot;
    this->_x0 = x0;
    this->_y0 = y0;
    this->_z0 = z0;
    _nsvar_var_trn.clear();
    _nsvar_var_trn.unite( nvar_var_trn_triads );
    _categorical_attributes.clear();
    _categorical_attributes << categorical_attributes;

    this->updatePropertyCollection();
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

void CartesianGrid::setCellGeometry(int nx, int ny, int nz, double dx, double dy, double dz)
{
    this->setInfo( _x0, _y0, _z0, dx, dy, dz, nx, ny, nz, _rot, _nreal,
                   _no_data_value, _nsvar_var_trn, _categorical_attributes);
}

void CartesianGrid::setDataIJK(uint column, uint i, uint j, uint k, double value)
{
    //TODO: verify any data update flags (specially in DataFile class)
    uint dataRow = i + j*_nx + k*_ny*_nx;
    _data[dataRow][column] = value;
}

std::vector<std::complex<double> > CartesianGrid::getArray(int indexColumRealPart, int indexColumImaginaryPart)
{
    std::vector< std::complex<double> > result( _nx * _ny * _nz ); //[_nx][_ny][_nz]

    for( uint k = 0; k < _nz; ++k)
        for( uint j = 0; j < _ny; ++j)
            for( uint i = 0; i < _nx; ++i){
                double real = 0.0d;
                double im = 0.0d;
                if( indexColumRealPart >= 0 )
                    real = dataIJK( indexColumRealPart, i, j, k);
                if( indexColumImaginaryPart >= 0 )
                    im = dataIJK( indexColumImaginaryPart, i, j, k);
                result[i + j*_nx + k*_ny*_nx] = std::complex<double>(real, im);
            }

    return result;
}

std::vector<std::vector<double> > CartesianGrid::getResampledValues(int rateI, int rateJ, int rateK,
                                                                    int &finalNI, int &finalNJ, int &finalNK)
{
    std::vector< std::vector<double> > result;

    uint totDataLinesPerRealization = _nx * _ny * _nz;

    //load just the first line to get the number of columns (assuming the file is right)
    setDataPage( 0, 0 );
    uint nDataColumns = getDataColumnCount();

    result.reserve( _nreal * (_nx/rateI) * (_ny/rateJ) * (_nz/rateK) * nDataColumns );

    //for each realization (at least one)
    for( uint r = 0; r < _nreal; ++r ){
        //compute the first and last data lines to load (does not need to load everything at once)
        ulong firstDataLine = r * totDataLinesPerRealization;
        ulong lastDataLine = firstDataLine + totDataLinesPerRealization - 1;
        //load the data corresponding to a realization
        setDataPage( firstDataLine, lastDataLine );
        loadData();
        //perform the resampling for a realization
        finalNK = 0;
        for( uint k = 0; k < _nz; k += rateK, ++finalNK ){
            finalNJ = 0;
            for( uint j = 0; j < _ny; j += rateJ, ++finalNJ ){
                finalNI = 0;
                for( uint i = 0; i < _nx; i += rateI, ++finalNI ){
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

double CartesianGrid::valueAt(uint dataColumn, double x, double y, double z, bool logOnError )
{
    uint i, j, k;
    if( ! XYZtoIJK( x, y, z, i, j, k ) ){
        if( logOnError )
            Application::instance()->logError("CartesianGrid::valueAt(): conversion from grid coordinates to spatial coordinates failed.  Returning NDV or NaN.");
        if( this->hasNoDataValue() )
            return getNoDataValueAsDouble();
        else
            return std::numeric_limits<double>::quiet_NaN();
    }

    //get the value
    return this->dataIJK( dataColumn, i, j, k);
}

void CartesianGrid::setDataPageToRealization(uint nreal)
{
    if( nreal >= _nreal ){
        Application::instance()->logError("CartesianGrid::setDataPageToRealization(): invalid realization number: " +
                                          QString::number( nreal ) + " (max. == " + QString::number( _nreal ) + "). Nothing done.");
        return;
    }
    ulong firstLine = nreal * _nx * _ny * _nz;
    ulong lastLine = (nreal+1) * _nx * _ny * _nz - 1; //the interval in DataFile::setDataPage() is inclusive.
    setDataPage( firstLine, lastLine );
}

double CartesianGrid::getRotation()
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

QString CartesianGrid::getGridName()
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
    if( pc->isAttribute() )
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
        ProjectComponent* pc = (ProjectComponent*)(*it);
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
    double x0 = _x0 - _dx / 2.0d;
    double y0 = _y0 - _dy / 2.0d;
    double z0 = _z0 - _dz / 2.0d;
    double xf = x0 + _dx * _nx;
    double yf = y0 + _dy * _ny;
    double zf = z0 + _dz * _nz;
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
                    value = std::pow( 10.0d, value_dB / DECIBEL_SCALE_FACTOR ) * dB_reference;
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
    if( _nz > 1 )
        k = (z - zBottom) / _dz;

    //check whether the location is outside the grid
    if( /*i < 0 ||*/ i >= _nx || /*j < 0 ||*/ j >= _ny || /*k < 0 ||*/ k >= _nz ){
        return false;
    }
    return true;
}

void CartesianGrid::setNReal(uint n)
{
    _nreal = n;
}

bool CartesianGrid::canHaveMetaData()
{
    return true;
}

QString CartesianGrid::getFileType()
{
    return "CARTESIANGRID";
}

void CartesianGrid::updateMetaDataFile()
{
    QFile file( this->getMetaDataFilePath() );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);
    out << APP_NAME << " metadata file.  This file is generated automatically.  Do not edit this file.\n";
    out << "version=" << APP_VERSION << '\n';
    out << "X0:" << QString::number( this->_x0, 'g', 12) << '\n';
    out << "Y0:" << QString::number( this->_y0, 'g', 12) << '\n';
    out << "Z0:" << QString::number( this->_z0, 'g', 12) << '\n';
    out << "NX:" << this->_nx << '\n';
    out << "NY:" << this->_ny << '\n';
    out << "NZ:" << this->_nz << '\n';
    out << "DX:" << this->_dx << '\n';
    out << "DY:" << this->_dy << '\n';
    out << "DZ:" << this->_dz << '\n';
    out << "ROT:" << this->_rot << '\n';
    out << "NREAL:" << this->_nreal << '\n';
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
    if( _nreal == 1){
        if(_nz == 1){
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
        if(_nz == 1){
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

spectral::array *CartesianGrid::createSpectralArray(int nDataColumn)
{
    spectral::array* data = new spectral::array( _nx, _ny, _nz, 0.0 );
    long idx = 0;
    for (ulong i = 0; i < _nx; ++i) {
        for (ulong j = 0; j < _ny; ++j) {
            for (ulong k = 0; k < _nz; ++k) {
                data->d_[idx] = dataIJK(nDataColumn, i, j, k);
                ++idx;
            }
        }
    }
    return data;
}

spectral::complex_array *CartesianGrid::createSpectralComplexArray(int variableIndex1, int variableIndex2)
{
    spectral::complex_array* data = new spectral::complex_array( _nx, _ny, _nz );
    long idx = 0;
    for (ulong i = 0; i < _nx; ++i) {
        for (ulong j = 0; j < _ny; ++j) {
            for (ulong k = 0; k < _nz; ++k) {
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

long CartesianGrid::append(const QString columnName, const spectral::array &array)
{
    long index = addEmptyDataColumn( columnName, _nx * _ny * _nz );

    ulong idx = 0;
    for (ulong i = 0; i < _nx; ++i) {
        for (ulong j = 0; j < _ny; ++j) {
            for (ulong k = 0; k < _nz; ++k) {
                double value = array.d_[idx];
                setDataIJK( index, i, j, k, value );
                ++idx;
            }
        }
    }

    if( idx != _nx * _ny * _nz )
        Application::instance()->logError("CartesianGrid::append(): mismatch between number of data values added and Cartesian grid cell count.");

    writeToFS();

    //update the project tree in the main window.
    Application::instance()->refreshProjectTree();

    return index;
}
