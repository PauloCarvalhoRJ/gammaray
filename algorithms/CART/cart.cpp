#include "cart.h"
#include "../ialgorithmdatasource.h"
#include "cartdecisionnode.h"
#include "cartleafnode.h"
#include <tuple>

CART::CART(const IAlgorithmDataSource &trainingData,
           IAlgorithmDataSource &outputData,
           const std::list<int> &trainingFeatureIDs,
           const std::list<int> &outputFeatureIDs ) :
    m_trainingData( trainingData ),
    m_outputData( outputData )
{
    //Create the list with all row IDs.
    std::list<long> rowIDs;
    long rowCount = trainingData.getRowCount();
    for( long iRow = 0; iRow < rowCount; ++iRow )
        rowIDs.push_back( iRow );

    //creates the training-to-output data sets feature column IDs.
    std::list<int>::const_iterator itTrainingIDs = trainingFeatureIDs.cbegin();
    std::list<int>::const_iterator itOutputIDs = outputFeatureIDs.cbegin();
    for( ; itTrainingIDs != trainingFeatureIDs.cend(); ++itTrainingIDs, //hopefully both lists have the same size
                                                       ++itOutputIDs ){
        m_training2outputFeatureIndexesMap[ *itTrainingIDs ] = *itOutputIDs;
    }

    //Built the CART tree, getting the pointer to the root node.
    m_root.reset( makeCART( rowIDs, trainingFeatureIDs ) );
}

void CART::classify(long rowIdOutput,
                    int dependentVariableColumnID,
                    std::list<std::pair<DataValue, long> > &result) const
{
    classify( rowIdOutput, dependentVariableColumnID, nullptr, result );
}

void CART::regress(long rowIdOutput, int dependentVariableColumnID, DataValue &mean, double &percent) const
{
    regress( rowIdOutput, dependentVariableColumnID, nullptr, mean, percent );
}

void CART::getUniqueDataValues(std::list<DataValue> &result,
                               const std::list<long> &rowIDs,
                               int columnIndex) const
{
    result.clear();
    std::list<long>::const_iterator it = rowIDs.begin();
    for( ; it != rowIDs.cend(); ++it )
        result.push_back( m_trainingData.getDataValue( *it, columnIndex ) );
    result.sort();
    result.unique();
}


void CART::getUniqueValuesCounts(std::list<std::pair<DataValue, long> > &result,
                               const std::list<long> &rowIDs,
                               int columnIndex ) const
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
    //for each of the values found.
    for( std::list<std::pair<DataValue, long> >::iterator it = result.begin(); it != result.end(); ++it){
        std::pair<DataValue, long>& pair = *it;
        //for each row
        for( std::list<long>::const_iterator rowIt = rowIDs.cbegin(); rowIt != rowIDs.cend(); ++rowIt ){
            if( m_trainingData.getDataValue( *rowIt, columnIndex ) == pair.first ){
                pair.second++;
            }
        }
    }
}

double CART::getGiniImpurity(const std::list<long> &rowIDs, int columnIndex) const
{
    //get the counts for each category/value found in the column (variable)
    std::list<std::pair<DataValue, long> > valuesCounts;
    getUniqueValuesCounts( valuesCounts, rowIDs, columnIndex );
    //get the number of rows
    long numberOfRows = rowIDs.size();
    //assumes total impurity
    double factor = 1.0d;
    //for each pair DataValue/count
    std::list<std::pair<DataValue, long> >::iterator it = valuesCounts.begin();
    for(; it != valuesCounts.end(); ++it){
        double categoryProportion = (*it).second / (double)numberOfRows;
        factor -= ( categoryProportion * categoryProportion );
    }
    return factor;
}

double CART::getSplitInformationGain(const std::list<long> &rowIDsTrueSide,
                                     const std::list<long> &rowIDsFalseSide,
                                     int columnIndex,
                                     double impurityFactorBeforeTheSplit) const
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

void CART::split(const std::list<long> &rowIDs,
                 const CARTSplitCriterion &criterion,
                 std::list<long> &trueSideRowIDs,
                 std::list<long> &falseSideRowIDs) const
{
    trueSideRowIDs.clear();
    falseSideRowIDs.clear();
    std::list<long>::const_iterator it = rowIDs.cbegin();
    for(; it != rowIDs.cend(); ++it ){
        if( criterion.trainingMatches( *it ))
            trueSideRowIDs.push_back( *it );
        else
            falseSideRowIDs.push_back( *it );
    }
}

std::pair<CARTSplitCriterion, double> CART::getSplitCriterionWithMaximumInformationGain(const std::list<long> &rowIDs,
                                                                                        const std::list<int> &featureIDs) const
{
    //Starts off with no information gain found.
    double highestInformationGain = 0.0;
    //The split criterion to be returned.
    CARTSplitCriterion finalSplitCriterion( m_trainingData, m_outputData, 0, DataValue(0.0), m_training2outputFeatureIndexesMap );
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
        //for each unique feature value.
        std::list<DataValue>::iterator uniqueValuesIt = uniqueValues.begin();
        for(; uniqueValuesIt != uniqueValues.end(); ++uniqueValuesIt){
            //Create a split criterion object
            CARTSplitCriterion splitCriterion( m_trainingData, m_outputData,
                                               *columnIt, *uniqueValuesIt, m_training2outputFeatureIndexesMap );
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

CARTNode *CART::makeCART(const std::list<long> &rowIDs,
                         const std::list<int> &featureIDs) const
{
    CARTSplitCriterion splitCriterion( m_trainingData, m_outputData, 0, DataValue(0.0), m_training2outputFeatureIndexesMap );
    double informationGain;

    //get the split criterion with maximum information gain for the row set.
    std::tie( splitCriterion, informationGain ) = getSplitCriterionWithMaximumInformationGain( rowIDs, featureIDs );

    //if there were no information gain, return a leaf node.
    if( informationGain <= 0.0 )
        return new CARTLeafNode( m_trainingData, rowIDs );

    //split the row set using the split criterion found with the highest information gain.
    std::list<long> trueSideRowIDs;
    std::list<long> falseSideRowIDs;
    split( rowIDs, splitCriterion, trueSideRowIDs, falseSideRowIDs );

    //make child nodes by recursing this function.
    CARTNode* trueSideChildNode = makeCART( trueSideRowIDs, featureIDs );
    CARTNode* falseSideChildNode = makeCART( falseSideRowIDs, featureIDs );

    //return a non-leaf node.
    return new CARTDecisionNode( splitCriterion, trueSideChildNode, falseSideChildNode, m_outputData );
}

void CART::classify(long rowIdOutput,
                    int dependentVariableColumnID,
                    const CARTNode *decisionTreeNode,
                    std::list<std::pair<DataValue, long> > &result) const
{

    //if the passed node pointer is null, then use the tree's root node.
    if( !decisionTreeNode )
        decisionTreeNode = m_root.get();

    //upon reaching a leaf node, no decision to make: simply return the values of the predicted variable from
    //the training data rows stored in the node
    if( decisionTreeNode->isLeaf() ){
        CARTLeafNode* leafNode = (CARTLeafNode*)decisionTreeNode;
        leafNode->getUniqueTrainingValuesWithCounts( dependentVariableColumnID, result );
        return;
    }

    //If execution reaches this point, the node is a decision one.
    CARTDecisionNode* decisionNode = (CARTDecisionNode*)decisionTreeNode;

    //Test the output data row against the node's split criterion.
    //Recursion goes to a child node depending whether the row satisfies the decision criterion.
    if( decisionNode->criterionMatches( rowIdOutput ) )
        return classify( rowIdOutput, dependentVariableColumnID, decisionTreeNode->getTrueSideChildNode(), result );
    else
        return classify( rowIdOutput, dependentVariableColumnID, decisionTreeNode->getFalseSideChildNode(), result );
}

void CART::regress(long rowIdOutput, int dependentVariableColumnID, const CARTNode *decisionTreeNode,
                   DataValue &mean, double &percent) const
{
    //if the passed node pointer is null, then use the tree's root node.
    if( !decisionTreeNode )
        decisionTreeNode = m_root.get();

    //upon reaching a leaf node, no decision to make: simply return the values of the predicted variable from
    //the training data rows stored in the node
    if( decisionTreeNode->isLeaf() ){
        CARTLeafNode* leafNode = (CARTLeafNode*)decisionTreeNode;
        leafNode->getMeanOfTrainingValuesWithPercentage( dependentVariableColumnID, mean, percent );
        return;
    }

    //If execution reaches this point, the node is a decision one.
    CARTDecisionNode* decisionNode = (CARTDecisionNode*)decisionTreeNode;

    //Test the output data row against the node's split criterion.
    //Recursion goes to a child node depending whether the row satisfies the decision criterion.
    if( decisionNode->criterionMatches( rowIdOutput ) )
        regress( rowIdOutput, dependentVariableColumnID, decisionTreeNode->getTrueSideChildNode(), mean, percent );
    else
        regress( rowIdOutput, dependentVariableColumnID, decisionTreeNode->getFalseSideChildNode(), mean, percent );
}
