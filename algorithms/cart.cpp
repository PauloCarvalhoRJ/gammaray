#include "cart.h"
#include "ialgorithmdatasource.h"
#include "cartnode.h"

CART::CART(const IAlgorithmDataSource &data) :
    m_data( data )
{
    long rowCount = m_data.getRowCount();

    //The root node refers to all data rows.
    for( long iRow = 0; iRow < rowCount; ++iRow )
        m_root.addRowIndex( iRow );
}
