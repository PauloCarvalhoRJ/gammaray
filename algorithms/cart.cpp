#include "cart.h"
#include "ialgorithmdatasource.h"

CART::CART(const IAlgorithmDataSource &data) :
    m_data( data )
{
    long rowCount = m_data.getRowCount();

    //The root node refers to all data rows.
    for( long iRow = 0; iRow < rowCount; ++iRow )
        m_root.addRowIndex( iRow );
}

void CART::split(const std::list<int> &rowIDs,
                 const CARTSplitCriterion &criterion,
                 std::list<int> &trueSideRowIDs,
                 std::list<int> &falseSideRowIDs)
{

    std::list<int>::const_iterator it = rowIDs.cbegin();
    for(; it != rowIDs.cend(); ++it ){
        if( criterion.matches( *it ))
            trueSideRowIDs.push_back( *it );
        else
            falseSideRowIDs.push_back( *it );
    }
}

void CART::getUniqueDataValues(std::list<DataValue> &result, const std::list<long> &rowIDs,
                                               int columnIndex) const
{
    result.clear();
    std::list<long>::const_iterator it = rowIDs.begin();
    for( ; it != rowIDs.cend(); ++it )
        result.push_back( m_data.getDataValue( *it, columnIndex ) );
    result.sort();
    result.unique();
}


void CART::getCategoriesCounts(std::list<std::pair<DataValue, long> > &result,
                               const std::list<long> &rowIDs,
                               int columnIndex) const
{
    //clears the output list object.
    result.clear();
    //get the categories to be counted.
    std::list<DataValue> categories;
    getUniqueDataValues( categories, rowIDs, columnIndex );
    //mount the output with zero counts.
    for(std::list<DataValue>::iterator it = categories.begin(); it != categories.end(); ++it){
        result.emplace_back( *it, 0 );
    }
    //for each of the categories found.
    for( std::list<std::pair<DataValue, long> >::iterator it = result.begin(); it != result.end(); ++it){
        std::pair<DataValue, long>& pair = *it;
        //for each row
        for( std::list<long>::const_iterator rowIt = rowIDs.cbegin(); rowIt != rowIDs.cend(); ++rowIt ){
            if( m_data.getDataValue( *rowIt, columnIndex ) == pair.first ){
                pair.second++;
            }
        }
    }
}

double CART::giniImpurity(const std::list<int> &rowIDs)
{

}
