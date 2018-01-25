#include "svdfactor.h"

SVDFactor::SVDFactor(spectral::array &&factorData, uint number, double weight, SVDFactor *parentFactor) :
    m_parentFactor( parentFactor ),
	m_factorData( std::move( factorData) ),
	m_number( number ),
	m_selected( true ),
	m_weight( weight )
{
}

SVDFactor::SVDFactor() :
	m_parentFactor( nullptr ),
	//m_factorData( ), //initialized by default constructor
	m_number( 0 ),
	m_selected( false ),
	m_weight( 0.0 )
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
	for(int i = 0; i < m_childFactors.size(); ++it, ++i)
		m_childFactors[i]->setWeight( *it );
	return true;
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
		return "Factor " + QString::number( m_number ) + " (" + QString::number( m_weight ) + ")";
}

QIcon SVDFactor::getIcon()
{
	return QIcon(":icons32/svd32");
}
