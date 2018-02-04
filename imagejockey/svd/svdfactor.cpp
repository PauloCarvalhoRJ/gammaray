#include "svdfactor.h"
#include <algorithm>

SVDFactor::SVDFactor(spectral::array &&factorData, uint number, double weight, double x0, double y0, double z0, double dx, double dy, double dz, SVDFactor *parentFactor) :
    m_parentFactor( parentFactor ),
	m_factorData( std::move( factorData) ),
	m_number( number ),
	m_selected( true ),
	m_weight( weight ),
	m_x0( x0 ),
	m_y0( y0 ),
	m_z0( z0 ),
	m_dx( dx ),
	m_dy( dy ),
	m_dz( dz ),
	m_currentPlaneOrientation( SVDFactorPlaneOrientation::XY ),
	m_currentSlice( 0 ),
	m_isMinValueDefined( false ),
	m_isMaxValueDefined( false )
{
}

SVDFactor::SVDFactor() :
	m_parentFactor( nullptr ),
	//m_factorData( ), //initialized by default constructor
	m_number( 0 ),
	m_selected( false ),
	m_weight( 0.0 ),
	m_x0( 0.0 ),
	m_y0( 0.0 ),
	m_z0( 0.0 ),
	m_dx( 1.0 ),
	m_dy( 1.0 ),
	m_dz( 1.0 ),
	m_currentPlaneOrientation( SVDFactorPlaneOrientation::XY ),
	m_currentSlice( 0 ),
	m_isMinValueDefined( false ),
	m_isMaxValueDefined( false )
{
}

SVDFactor::~SVDFactor()
{
	//delete the child factors (if any).
	while( ! m_childFactors.empty() ){
		delete m_childFactors.back();
		m_childFactors.pop_back();
	}
}

void SVDFactor::addChildFactor(SVDFactor * child)
{
	m_childFactors.push_back( child );
	child->setParentFactor( this );
}

bool SVDFactor::assignWeights(const std::vector<double> & weights)
{
	if( weights.size() < m_childFactors.size() )
		return false;
	std::vector<double>::const_iterator it = weights.cbegin();
	for(uint i = 0; i < m_childFactors.size(); ++it, ++i)
		m_childFactors[i]->setWeight( *it );
	return true;
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
		case SVDFactorPlaneOrientation::XY: return m_factorData.M();
		case SVDFactorPlaneOrientation::XZ: return m_factorData.M();
		case SVDFactorPlaneOrientation::YZ: return m_factorData.N();
		default: return m_factorData.M();
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
		case SVDFactorPlaneOrientation::XY: return m_factorData.N();
		case SVDFactorPlaneOrientation::XZ: return m_factorData.K();
		case SVDFactorPlaneOrientation::YZ: return m_factorData.K();
		default: return m_factorData.N();
	}
}

double SVDFactor::getMinValue()
{
	if( m_isMinValueDefined )
		return m_minValue;
	m_minValue = *std::min_element( m_factorData.data().begin(), m_factorData.data().end() ) ;
	m_isMinValueDefined = true;
    return m_minValue;
}

double SVDFactor::getMaxValue()
{
	if( m_isMaxValueDefined )
		return m_maxValue;
	m_maxValue = *std::max_element( m_factorData.data().begin(), m_factorData.data().end() ) ;
	m_isMaxValueDefined = true;
    return m_maxValue;
}

uint SVDFactor::getCurrentPlaneNumberOfSlices()
{
	switch( m_currentPlaneOrientation ){
		case SVDFactorPlaneOrientation::XY: return m_factorData.K();
		case SVDFactorPlaneOrientation::XZ: return m_factorData.N();
		case SVDFactorPlaneOrientation::YZ: return m_factorData.M();
		default: return m_factorData.N();
    }
}

void SVDFactor::setPlaneOrientation(SVDFactorPlaneOrientation orientation)
{
    m_currentPlaneOrientation = orientation;
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
	return m_factorData(i, j, k);
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
	if( ! m_parentFactor ) //root factor
		return "ROOT";
	else
		return "Factor " + getHierarchicalNumber() + " (" + QString::number( m_weight ) + ")";
}

QIcon SVDFactor::getIcon()
{
	return QIcon(":icons32/svd32");
}
