#include "gridfile.h"
#include "application.h"
#include "spectral/spectral.h"

GridFile::GridFile( QString path ) : DataFile( path )
{
}


std::vector<std::complex<double> > GridFile::getArray(int indexColumRealPart, int indexColumImaginaryPart)
{
	std::vector< std::complex<double> > result( m_nI * m_nJ * m_nK ); //[_nx][_ny][_nz]

	for( uint k = 0; k < m_nK; ++k)
		for( uint j = 0; j < m_nJ; ++j)
			for( uint i = 0; i < m_nI; ++i){
				double real = 0.0;
				double im = 0.0;
				if( indexColumRealPart >= 0 )
					real = dataIJK( indexColumRealPart, i, j, k);
				if( indexColumImaginaryPart >= 0 )
					im = dataIJK( indexColumImaginaryPart, i, j, k);
				result[i + j*m_nI + k*m_nJ*m_nI] = std::complex<double>(real, im);
			}

	return result;
}


double GridFile::valueAt(uint dataColumn, double x, double y, double z )
{
	uint i, j, k;
	if( ! XYZtoIJK( x, y, z, i, j, k ) ){
		if( this->hasNoDataValue() )
			return getNoDataValueAsDouble();
		else
			return std::numeric_limits<double>::quiet_NaN();
	}

	//get the value
	return this->dataIJK( dataColumn, i, j, k);
}


void GridFile::setDataPageToRealization(uint nreal)
{
	if( nreal >= m_nreal ){
		Application::instance()->logError("GridFile::setDataPageToRealization(): invalid realization number: " +
										  QString::number( nreal ) + " (max. == " + QString::number( m_nreal ) + "). Nothing done.");
		return;
	}
	ulong firstLine = nreal * m_nI * m_nJ * m_nK;
	ulong lastLine = (nreal+1) * m_nI * m_nJ * m_nK - 1; //the interval in DataFile::setDataPage() is inclusive.
	setDataPage( firstLine, lastLine );
}


void GridFile::setNReal(uint n)
{
	m_nreal = n;
}


long GridFile::append(const QString columnName, const spectral::array &array)
{
	long index = addEmptyDataColumn( columnName, m_nI * m_nJ * m_nK );

	ulong idx = 0;
	for (ulong i = 0; i < m_nI; ++i) {
		for (ulong j = 0; j < m_nJ; ++j) {
			for (ulong k = 0; k < m_nK; ++k) {
				double value = array.d_[idx];
				setDataIJK( index, i, j, k, value );
				++idx;
			}
		}
	}

	if( idx != m_nI * m_nJ * m_nK )
		Application::instance()->logError("GridFile::append(): mismatch between number of data values added and grid cell count.");

	writeToFS();

	//update the project tree in the main window.
	Application::instance()->refreshProjectTree();

	return index;
}


void GridFile::setDataIJK(uint column, uint i, uint j, uint k, double value)
{
	//TODO: verify any data update flags (specially in DataFile class)
	uint dataRow = i + j*m_nI + k*m_nJ*m_nI;
	_data[dataRow][column] = value;
}

void GridFile::indexToIJK(uint index, uint & i, uint & j, uint & k)
{
	uint val = index;
	uint nynx = m_nJ * m_nI;
	k = val / nynx;
	val -= (k*nynx);
	j = val / m_nI;
	i = val % m_nI;
}

void GridFile::setColumnData(uint dataColumn, spectral::array & array)
{
	loadData();

	double NDV;
	if( hasNoDataValue() )
		NDV = getNoDataValueAsDouble();
	else
		NDV = -99999.0;

	//data replacement loop
	ulong idx = 0;
	for (ulong i = 0; i < m_nI; ++i) {
		for (ulong j = 0; j < m_nJ; ++j) {
			for (ulong k = 0; k < m_nK; ++k) {
				double value = array.d_[idx];
				if( std::isnan( value ) )
					value = NDV;
				setDataIJK( dataColumn, i, j, k, value );
				++idx;
			}
		}
	}

	if( idx != m_nI * m_nJ * m_nK )
		Application::instance()->logError("GridFile::setColumnData(): mismatch between number of data values added and grid cell count.");

	writeToFS();

	//update the project tree in the main window.
	Application::instance()->refreshProjectTree();
}

uint GridFile::IJKtoIndex(uint i, uint j, uint k)
{
	return k * m_nJ * m_nI + j * m_nI + i;
}

double GridFile::getNeighborValue(int iRecord, int iVar, int dI, int dJ, int dK)
{
	uint i, j, k;
	indexToIJK( iRecord, i, j, k );
	i += dI;
	j += dJ;
	k += dK;
	if( i >= m_nI || j >= m_nJ || k >= m_nK ) //unsigned ints become huge if converted from negative integers
		return std::numeric_limits<double>::quiet_NaN();
	double value = dataIJK( iVar, i, j, k );
	if( isNDV( value ) )
		value = std::numeric_limits<double>::quiet_NaN();
	return value;
}
