#include "cart.h"
#include "ialgorithmdatasource.h"
#include <tuple>

CART::CART(const IAlgorithmDataSource &data) :
    m_data( data )
{
    long rowCount = m_data.getRowCount();

    //The root node refers to all data rows.
    for( long iRow = 0; iRow < rowCount; ++iRow )
        m_root.addRowIndex( iRow );
}

void CART::split(const std::list<long> &rowIDs,
                 const CARTSplitCriterion &criterion,
                 std::list<long> &trueSideRowIDs,
                 std::list<long> &falseSideRowIDs)
{
    trueSideRowIDs.clear();
    falseSideRowIDs.clear();
    std::list<long>::const_iterator it = rowIDs.cbegin();
    for(; it != rowIDs.cend(); ++it ){
        if( criterion.matches( *it ))
            trueSideRowIDs.push_back( *it );
        else
            falseSideRowIDs.push_back( *it );
    }
}

void CART::getUniqueDataValues(std::list<DataValue> &result,
                               const std::list<long> &rowIDs,
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
                               int columnIndexWithCategoricalValues) const
{
    //clears the output list object.
    result.clear();
    //get the categories to be counted.
    std::list<DataValue> categories;
    getUniqueDataValues( categories, rowIDs, columnIndexWithCategoricalValues );
    //mount the output with zero counts.
    for(std::list<DataValue>::iterator it = categories.begin(); it != categories.end(); ++it){
        result.emplace_back( *it, 0 );
    }
    //for each of the categories found.
    for( std::list<std::pair<DataValue, long> >::iterator it = result.begin(); it != result.end(); ++it){
        std::pair<DataValue, long>& pair = *it;
        //for each row
        for( std::list<long>::const_iterator rowIt = rowIDs.cbegin(); rowIt != rowIDs.cend(); ++rowIt ){
            if( m_data.getDataValue( *rowIt, columnIndexWithCategoricalValues ) == pair.first ){
                pair.second++;
            }
        }
    }
}

double CART::getGiniImpurity(const std::list<long> &rowIDs, int columnIndex) const
{
    //get the counts for each category/value found in the column (variable)
    std::list<std::pair<DataValue, long> > categoriesCounts;
    getCategoriesCounts( categoriesCounts, rowIDs, columnIndex ); //using the CategoriesCounts to count unique valuse in continuous variables
    //get the number of rows
    long numberOfRows = rowIDs.size();
    //assumes total impurity
    double factor = 1.0d;
    //for each pair DataValue/count
    std::list<std::pair<DataValue, long> >::iterator it = categoriesCounts.begin();
    for(; it != categoriesCounts.end(); ++it){
        double categoryProportion = (*it).second / (double)numberOfRows;
        factor -= ( categoryProportion * categoryProportion );
    }
    return factor;
}

double CART::getSplitInformationGain(const std::list<long> &rowIDsTrueSide,
                                     const std::list<long> &rowIDsFalseSide,
                                     int columnIndex,
                                     double impurityFactorBeforeTheSplit)
{
    //Get the total number of rows in both sides of the split.
    long totalSizeBothSides = rowIDsTrueSide.size() + rowIDsFalseSide.size();
    //Get the proportion of criterion hits.
    double proportionOfTrue = rowIDsTrueSide.size() / (double)totalSizeBothSides;
    //Get the uncertainties of both sides of the split
    double impurityTrueSide = getGiniImpurity( rowIDsTrueSide, columnIndex );
    double impurityFalseSide = getGiniImpurity( rowIDsFalseSide, columnIndex );
    //Compute the impurity after split by averaging both sides impurity factors, weighted by their proportions.
    double impurityFactorAfterTheSplit =     proportionOfTrue     * impurityTrueSide +
                                         (1.0 - proportionOfTrue) * impurityFalseSide;
    //The information gain is the impurity delta before and after the split.  Hopefully a positive delta.
    return impurityFactorBeforeTheSplit - impurityFactorAfterTheSplit;
}

std::pair<CARTSplitCriterion, double> CART::getSplitCriterionWithMaximumInformationGain(const std::list<long> &rowIDs,
                                                                                        const std::list<int> &featureIDs)
{
    //Starts off with no information gain found.
    double highestInformationGain = 0.0;
    //The split criterion to be returned.
    CARTSplitCriterion finalSplitCriterion( m_data, 0, DataValue(0.0) );
    //Create a list to hold unique data values.
    std::list<DataValue> uniqueValues;
    //Create a list to hold row ids with data that match the split criterion.
    std::list<long> trueSideRowIDs;
    //Create a list to hold row ids with data that don't match the split criterion.
    std::list<long> falseSideRowIDs;
    //for each feature column.
    std::list<int>::const_iterator columnIt = featureIDs.cbegin();
    for(; columnIt != featureIDs.cend(); ++columnIt){
        //compute the impurity (uncertainty) for the current feature in the current row set.
        double impurity = getGiniImpurity( rowIDs, *columnIt );
        //get the unique feature values found in the row set
        getUniqueDataValues( uniqueValues, rowIDs, *columnIt );
        //for each unique feature values.
        std::list<DataValue>::iterator uniqueValuesIt = uniqueValues.begin();
        for(; uniqueValuesIt != uniqueValues.end(); ++uniqueValuesIt){
            //Create a split criterion object
            CARTSplitCriterion splitCriterion( m_data, *columnIt, *uniqueValuesIt );
            //Split the row set using the criterion above
            split( rowIDs, splitCriterion, trueSideRowIDs, falseSideRowIDs );
            //if there is uncertainty (both true and false row id lists have data)
            if( trueSideRowIDs.size() != 0 && falseSideRowIDs.size() != 0 ){
                //get the information gain with the split.
                double informationGain = getSplitInformationGain( trueSideRowIDs, falseSideRowIDs, *columnIt, impurity );
                //if the information gain is greater than found so far, save it, along with the criterion, for the next iteration
                if( informationGain > highestInformationGain ){
                    highestInformationGain = informationGain;
                    finalSplitCriterion = splitCriterion;
                }
            }
        }
    }
    return {finalSplitCriterion, highestInformationGain};
}

void CART::makeCART(const std::list<long> &rowIDs,
                    const std::list<int> &featureIDs)
{
    CARTSplitCriterion splitCriterion( m_data, 0, DataValue(0.0) );
    double informationGain;

    //get the split criterion with maximum information gain for the row set.
    std::tie( splitCriterion, informationGain ) = getSplitCriterionWithMaximumInformationGain( rowIDs, featureIDs );
}
