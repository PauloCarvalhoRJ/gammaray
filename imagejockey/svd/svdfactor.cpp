#include "svdfactor.h"
#include <QInputDialog>
#include <QMessageBox>
#include <algorithm>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/scoped_array.hpp>
#include "../ijabstractvariable.h"
#include "../imagejockeyutils.h"
#include "spectral/spectral.h"

//Implementing IJAbstractVariable to use the IJAbstractCartesianGrid interface,
//so it is possible to use Image Jockey
class CartesianGridVariable : public IJAbstractVariable{
public:
    CartesianGridVariable( SVDFactor* parent ) : m_parent( parent ){
    }
    // IJAbstractVariable interface
public:
    virtual IJAbstractCartesianGrid *getParentGrid()
    {
        return m_parent;
    }
    virtual int getIndexInParentGrid()
    {
        return 0; //SVDFactors have only one variable
    }
    virtual QString getVariableName()
    {
        return "values";
    }
    virtual QIcon getVariableIcon()
    {
        return QIcon(":ijicons32/ijvariable32");
    }
private:
    SVDFactor* m_parent;
};

double SVDFactor::getSVDFactorTreeSplitThreshold(bool reset)
{
    static double setting = -1.0;
    if( setting < 0.0 || reset ){
        //ask the user once for the default tree split threshold
        bool ok;
        int percentage = QInputDialog::getInt(nullptr, "Further SVD factoring threshold",
                                     "Split information content in percentage parts (%):", 50, 1, 50, 5, &ok);
        if (ok)
            setting = percentage / 100.0;
        else
            setting = 0.5; //default to 50%
    }
    return setting;
}

SVDFactor::SVDFactor(spectral::array &&factorData, uint number,
                     double weight, double x0, double y0,
                     double z0, double dx, double dy, double dz,
                     double mergeThreshold,
                     SVDFactor *parentFactor) :
    IJAbstractCartesianGrid(),
    m_parentFactor( parentFactor ),
    m_factorData( new spectral::array( std::move( factorData ) ) ),
	m_number( number ),
	m_selected( true ),
	m_weight( weight ),
	m_currentPlaneOrientation( SVDFactorPlaneOrientation::XY ),
	m_currentSlice( 0 ),
	m_x0( x0 ),
	m_y0( y0 ),
	m_z0( z0 ),
	m_dx( dx ),
	m_dy( dy ),
	m_dz( dz ),
	m_isMinValueDefined( false ),
    m_isMaxValueDefined( false ),
    m_mergeThreshold( mergeThreshold ),
    m_type( SVDFactorType::UNSPECIFIED )
{
    m_variableProxy = new CartesianGridVariable( this );
}

SVDFactor::SVDFactor() :
	m_parentFactor( nullptr ),
    m_factorData( new spectral::array() ),
	m_number( 0 ),
	m_selected( false ),
	m_weight( 0.0 ),
	m_currentPlaneOrientation( SVDFactorPlaneOrientation::XY ),
	m_currentSlice( 0 ),
	m_x0( 0.0 ),
	m_y0( 0.0 ),
	m_z0( 0.0 ),
	m_dx( 1.0 ),
	m_dy( 1.0 ),
	m_dz( 1.0 ),
    m_isMinValueDefined( false ),
    m_isMaxValueDefined( false ),
    m_mergeThreshold( -1.0 ),
    m_type( SVDFactorType::UNSPECIFIED )
{
    m_variableProxy = new CartesianGridVariable( this );
}

SVDFactor::~SVDFactor()
{
    //delete the proxy variable object.
    delete m_variableProxy;
	//delete the child factors (if any).
    deleteChildren();
    //delete the data array
    delete m_factorData;
}

void SVDFactor::addChildFactor(SVDFactor * child)
{
    //get the lastly added child factor
    SVDFactor* lastChildFactor = nullptr;
    if( ! m_childFactors.empty() )
        lastChildFactor = m_childFactors.back();

    //merges the new factor into the last one or add as a new child factor (depends on the merge threshold).
    if( lastChildFactor && lastChildFactor->getWeight() < m_mergeThreshold )
        lastChildFactor->merge( child );
    else{
        m_childFactors.push_back( child );
        child->setParentFactor( this );
    }
}

QString SVDFactor::getHierarchicalNumber()
{
	QString result = QString::number( m_number );
	if( ! isTopLevel() )
		result = m_parentFactor->getHierarchicalNumber() + "." + result;
	return result;
}

double SVDFactor::valueAtCurrentPlane(double localX, double localY)
{

	uint i, j;
	if( ! XYtoIJinCurrentPlane( localX, localY, i, j ) )
		return std::numeric_limits<double>::quiet_NaN();

	//get the value
	switch( m_currentPlaneOrientation ){
		case SVDFactorPlaneOrientation::XY: return this->dataIJK( i, j, m_currentSlice );
		case SVDFactorPlaneOrientation::XZ: return this->dataIJK( i, m_currentSlice, j );
		case SVDFactorPlaneOrientation::YZ: return this->dataIJK( m_currentSlice, i, j );
		default: return this->dataIJK( i, j, m_currentSlice );
	}
}

bool SVDFactor::isNDV(double value)
{
	Q_UNUSED(value);
	//TODO: ASSUMES THERE IS NO UNINFORMED CELLS.
	return false;
}

double SVDFactor::getCurrentPlaneX0()
{
	switch( m_currentPlaneOrientation ){
		case SVDFactorPlaneOrientation::XY: return m_x0;
		case SVDFactorPlaneOrientation::XZ: return m_x0;
		case SVDFactorPlaneOrientation::YZ: return m_y0;
		default: return m_x0;
	}
}

double SVDFactor::getCurrentPlaneDX()
{
	switch( m_currentPlaneOrientation ){
		case SVDFactorPlaneOrientation::XY: return m_dx;
		case SVDFactorPlaneOrientation::XZ: return m_dx;
		case SVDFactorPlaneOrientation::YZ: return m_dy;
		default: return m_dx;
	}
}

uint SVDFactor::getCurrentPlaneNX()
{
	switch( m_currentPlaneOrientation ){
        case SVDFactorPlaneOrientation::XY: return m_factorData->M();
        case SVDFactorPlaneOrientation::XZ: return m_factorData->M();
        case SVDFactorPlaneOrientation::YZ: return m_factorData->N();
        default: return m_factorData->M();
	}
}

double SVDFactor::getCurrentPlaneY0()
{
	switch( m_currentPlaneOrientation ){
		case SVDFactorPlaneOrientation::XY: return m_y0;
		case SVDFactorPlaneOrientation::XZ: return m_z0;
		case SVDFactorPlaneOrientation::YZ: return m_z0;
		default: return m_y0;
	}
}

double SVDFactor::getCurrentPlaneDY()
{
	switch( m_currentPlaneOrientation ){
		case SVDFactorPlaneOrientation::XY: return m_dy;
		case SVDFactorPlaneOrientation::XZ: return m_dz;
		case SVDFactorPlaneOrientation::YZ: return m_dz;
		default: return m_dy;
	}
}

uint SVDFactor::getCurrentPlaneNY()
{
	switch( m_currentPlaneOrientation ){
        case SVDFactorPlaneOrientation::XY: return m_factorData->N();
        case SVDFactorPlaneOrientation::XZ: return m_factorData->K();
        case SVDFactorPlaneOrientation::YZ: return m_factorData->K();
        default: return m_factorData->N();
	}
}

double SVDFactor::getMinValue()
{
	if( m_isMinValueDefined )
		return m_minValue;
    m_minValue = *std::min_element( m_factorData->data().begin(), m_factorData->data().end() ) ;
	m_isMinValueDefined = true;
    return m_minValue;
}

double SVDFactor::getMaxValue()
{
	if( m_isMaxValueDefined )
		return m_maxValue;
    m_maxValue = *std::max_element( m_factorData->data().begin(), m_factorData->data().end() ) ;
	m_isMaxValueDefined = true;
    return m_maxValue;
}

uint SVDFactor::getCurrentPlaneNumberOfSlices()
{
	switch( m_currentPlaneOrientation ){
        case SVDFactorPlaneOrientation::XY: return m_factorData->K();
        case SVDFactorPlaneOrientation::XZ: return m_factorData->N();
        case SVDFactorPlaneOrientation::YZ: return m_factorData->M();
        default: return m_factorData->N();
    }
}

void SVDFactor::setPlaneOrientation(SVDFactorPlaneOrientation orientation)
{
    m_currentPlaneOrientation = orientation;
}

void SVDFactor::addTo(spectral::array *array, bool ifSelected )
{
    if( m_childFactors.size() == 0 && ( !ifSelected || m_selected ) )
        *array += *m_factorData;
    else{
        std::vector<SVDFactor*>::iterator it = m_childFactors.begin();
        for(; it != m_childFactors.end(); ++it)
            (*it)->addTo( array, ifSelected );
	}
}

void SVDFactor::merge( SVDFactor * &other )
{
    *m_factorData += *(other->m_factorData);
	m_weight += other->m_weight;
	m_isMaxValueDefined = false;
	m_isMinValueDefined = false;
	delete other;
    other = nullptr;
}

double SVDFactor::getInfoContent()
{
    if( isTopLevel() )
        return m_weight;
    else
        return m_weight * m_parentFactor->getInfoContent();
}

void SVDFactor::deleteChildren()
{
    while( ! m_childFactors.empty() ){
        delete m_childFactors.back();
        m_childFactors.pop_back();
    }
}

void SVDFactor::setChildMergeThreshold(double threshold)
{
    if( ! hasChildren() )
        m_mergeThreshold = threshold;
}

void SVDFactor::aggregate(std::vector<SVDFactor *>& factors_to_aggregate)
{
    std::vector<SVDFactor *>::iterator it = factors_to_aggregate.begin();
    SVDFactor* firstCome = nullptr;
    for(; it != factors_to_aggregate.end(); ++it){
        //ignore factors whose parent is not this factor
        if( (*it)->m_parentFactor != this )
            continue;
        //define the first-come child
        if( ! firstCome ) {
            firstCome = (*it);
            //the previously computed children will not represent the new ammount of information after aggregation
            firstCome->deleteChildren();
        }
        //aggregate the following children in the list
        else {
            SVDFactor* currentFactor = *it;
            firstCome->merge( *it ); //SVDFactor::merge() deletes and sets the pointer to nullptr.
            //remove the item from the list of child factores
            m_childFactors.erase( std::remove_if (m_childFactors.begin(), m_childFactors.end(),
                                   [currentFactor](SVDFactor* i) { return i == currentFactor; }), m_childFactors.end());
            //when at least one factor is aggregated, this factor becomes geological (non-fundamental)
            firstCome->setType( SVDFactorType::GEOLOGICAL );
        }
    }
}

uint SVDFactor::getIndexOfChild(SVDFactor* child)
{
	int i = 0;
	for (std::vector<SVDFactor*>::iterator it = m_childFactors.begin() ; it != m_childFactors.end(); ++it, ++i)
	   if ( *it == child )
		  return i;
	return -1;
}

bool SVDFactor::isRoot()
{
	return ! m_parentFactor;
}

void SVDFactor::setParentFactor(SVDFactor * parent)
{
	m_parentFactor = parent;
}

bool SVDFactor::isTopLevel()
{
	if( isRoot() )
		return false;
	return m_parentFactor->isRoot();
}

bool SVDFactor::XYtoIJinCurrentPlane(double localX, double localY, uint & i, uint & j)
{
	double x0 = getCurrentPlaneX0();
	double y0 = getCurrentPlaneY0();
	double dx = getCurrentPlaneDX();
	double dy = getCurrentPlaneDY();
	uint nx = getCurrentPlaneNX();
	uint ny = getCurrentPlaneNY();

	//compute the indexes from the spatial location.
	double xWest = x0 - dx/2.0;
	double ySouth = y0 - dy/2.0;
	i = (localX - xWest) / dx;
	j = (localY - ySouth) / dy;

	//check whether the location is outside the grid
	if( /*i < 0 ||*/ i >= nx || /*j < 0 ||*/ j >= ny ){
		return false;
	}
	return true;
}

double SVDFactor::dataIJK(uint i, uint j, uint k)
{
    return (*m_factorData)(i, j, k);
}

SVDFactor *SVDFactor::getChildByIndex(uint index)
{
	return m_childFactors[index];
}

SVDFactor *SVDFactor::getParent()
{
	return m_parentFactor;
}

uint SVDFactor::getIndexInParent()
{
	if( m_parentFactor )
		return m_parentFactor->getIndexOfChild( this );
	else
		return -1;
}

uint SVDFactor::getChildCount()
{
	return m_childFactors.size();
}

QString SVDFactor::getPresentationName()
{
    if( ! m_customName.isEmpty() )
        return m_customName;

    if( ! m_parentFactor ) //root factor
		return "ROOT";
    else{
        return "Factor " + getHierarchicalNumber() + " (" + QString::number( getInfoContent() * 100 ) + "%)";
    }
}

QIcon SVDFactor::getIcon()
{
    switch( m_type ){
    case SVDFactorType::GEOLOGICAL:
        return QIcon(":ijicons32/ijgeofactor32");
        break;
    case SVDFactorType::FUNDAMENTAL:
        return QIcon(":ijicons32/ijfundfactor32");
        break;
    default:
        return QIcon(":ijicons32/ijsvd32");
    }
}

double SVDFactor::getData(int variableIndex, int i, int j, int k)
{
    Q_UNUSED( variableIndex ); //SVDFactors have just one variable
    return dataIJK( i, j, k );
}

bool SVDFactor::XYZtoIJK(double x, double y, double z, uint &i, uint &j, uint &k)
{
    //TODO: add support for rotations
    if( ! ImageJockeyUtils::almostEqual2sComplement( getRotation(), 0.0, 1) ){
        return false;
    }

    //compute the indexes from the spatial location.
    double xWest = getOriginX() - getCellSizeI()/2.0;
    double ySouth = getOriginY() - getCellSizeJ()/2.0;
    double zBottom = getOriginZ() - getCellSizeK()/2.0;
    i = (x - xWest) / getCellSizeI();
    j = (y - ySouth) / getCellSizeJ();
    k = 0;
    if( getNK() > 1 )
        k = (z - zBottom) / getCellSizeK();

    //check whether the location is outside the grid
    if( /*i < 0 ||*/ i >= (uint)getNI() || /*j < 0 ||*/ j >= (uint)getNJ() || /*k < 0 ||*/ k >= (uint)getNK() ){
        return false;
    }
    return true;
}


double SVDFactor::getDataAt(int variableIndex, double x, double y, double z)
{
    //try to convert spatial coordinates into topological coordinates
    uint i, j, k;
    if( ! XYZtoIJK( x, y, z, i, j, k ) ){
        //the passed spatial coordinates may fall outside the grid,
        //so return NaN if the conversion fails.
        return std::numeric_limits<double>::quiet_NaN();
    }
    //get the value
    return getData( variableIndex, i, j, k );
}

double SVDFactor::absMax(int variableIndex)
{
    Q_UNUSED( variableIndex ); //SVDFactors have just one variable
    double result = 0.0d;
    for (uint i = 0; i < m_factorData->data().size(); ++i) {
        double value = m_factorData->data()[i];
        if ( std::abs<double>(value) > result )
            result = std::abs<double>(value);
    }
    return result;
}

double SVDFactor::absMin(int variableIndex)
{
    Q_UNUSED( variableIndex ); //SVDFactors have just one variable
    double result = std::numeric_limits<double>::max();
    for (uint i = 0; i < m_factorData->data().size(); ++i) {
        double value = m_factorData->data()[i];
        if ( std::abs<double>(value) < result )
            result = std::abs<double>(value);
    }
    return result;
}

void SVDFactor::getAllVariables(std::vector<IJAbstractVariable *> &result)
{
    result.push_back( m_variableProxy );
}

void SVDFactor::equalizeValues(QList<QPointF> &area,
                               double delta_dB,
                               int variableIndex,
                               double dB_reference,
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
    for( int k = 0; k < getNK(); ++k ){
        // z coordinate is ignored in 2D spectrograms
        for( int j = 0; j < getNJ(); ++j ){
            double cellCenterY = getOriginY() + j * getCellSizeJ();
            for( int i = 0; i < getNI(); ++i ){
                double cellCenterX = getOriginX() + i * getCellSizeJ();
                boostPoint2D p(cellCenterX, cellCenterY);
                // if the cell center lies within the area
                // The bounding box test is a faster test to promplty discard cells obviously outside.
                if(     ImageJockeyUtils::isWithinBBox( cellCenterX, cellCenterY, minX, minY, maxX, maxY )
                        &&
                        boost::geometry::within(p, poly)
                        &&
                        ( secondArea.isEmpty() || boost::geometry::within(p, secondPoly) )   ){
                    // get the grid value as is
                    double value = getData( variableIndex, i, j, k );
                    // determine whether the value is negative
                    bool isNegative = value < 0.0;
                    // get the absolute value
                    value = std::abs(value);
                    // get the absolute value in dB
                    double value_dB = ImageJockeyUtils::dB( value, dB_reference, 0.00001);
                    // apply adjustment in dB
                    value_dB += delta_dB;
                    // attenuate/amplify the absolute value
                    value = std::pow( 10.0d, value_dB / 10.0/*DECIBEL_SCALE_FACTOR*/ ) * dB_reference;
                    // add negative sign if the original value was negative
                    if( isNegative )
                        value = -value;
                    // set the amplified/attenuated value to the grid
                    (*m_factorData)(i, j, k) = value;
                }
            }
        }
    }
}

void SVDFactor::saveData()
{
    QMessageBox::critical( nullptr, "Error", QString("SVDFactor::saveData(): Unsupported operation."));
}

spectral::array *SVDFactor::createSpectralArray(int variableIndex)
{
    Q_UNUSED( variableIndex );
    return new spectral::array( *m_factorData );
}

spectral::complex_array *SVDFactor::createSpectralComplexArray(int variableIndex1, int variableIndex2)
{
    Q_UNUSED(variableIndex1);
    Q_UNUSED(variableIndex2);
    QMessageBox::critical( nullptr, "Error", QString("SVDFactor::createSpectralComplexArray(): Unsupported operation."));
    return nullptr;
}

long SVDFactor::appendAsNewVariable(const QString variableName, const spectral::array &array)
{
    Q_UNUSED(variableName);
    Q_UNUSED(array);
    QMessageBox::critical( nullptr, "Error", QString("SVDFactor::appendAsNewVariable(): Unsupported operation."));
    return -1;
}

int SVDFactor::getNI()
{
    return m_factorData->M();
}

int SVDFactor::getNJ()
{
    return m_factorData->N();
}

int SVDFactor::getNK()
{
    return m_factorData->K();
}
