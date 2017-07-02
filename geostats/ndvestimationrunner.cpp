#include "ndvestimationrunner.h"

#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "gridcell.h"
#include "geostatsutils.h"
#include "ndvestimation.h"

NDVEstimationRunner::NDVEstimationRunner(NDVEstimation *ndvEstimation, Attribute *at, QObject *parent) :
    QObject(parent),
    _at(at),
    _ndvEstimation(ndvEstimation),
    _finished( false )
{
}

void NDVEstimationRunner::doRun()
{
    //gets the Attribute's column in its Cartesian grid's data array (GEO-EAS index - 1)
    uint atIndex = _at->getAttributeGEOEASgivenIndex() - 1;

    //Assumes the partent file of the selected attribut is a Cartesian grid
    CartesianGrid *cg = (CartesianGrid*)_at->getContainingFile();

    //get the grid dimensions
    uint nI = cg->getNX();
    uint nJ = cg->getNY();
    uint nK = cg->getNZ();

    int unvaluedCount = 0;
    for( uint k = 0; k <nK; ++k)
        for( uint j = 0; j <nJ; ++j){
            emit progress( j * nI + k * nI * nJ );
            for( uint i = 0; i <nI; ++i){
                double value = cg->dataIJK( atIndex, i, j, k );
                if( cg->isNDV( value ) ){
                    ++unvaluedCount;
                    krige( GridCell(cg, atIndex, i,j,k), _ndvEstimation->meanForSK() );
                }
            }
        }

    //inform the calling thread computation was finished
    _finished = true;
}

double NDVEstimationRunner::krige(GridCell cell, double meanSK)
{
    double result = std::numeric_limits<double>::quiet_NaN();

    //collects valued n-neighbors ordered by their topological distance with respect
    //to the target cell
    std::multiset<GridCell> vCells = GeostatsUtils::getValuedNeighborsTopoOrdered( cell,
                                                           _ndvEstimation->searchMaxNumSamples(),
                                                           _ndvEstimation->searchNumCols(),
                                                           _ndvEstimation->searchNumRows(),
                                                           _ndvEstimation->searchNumSlices() );

    if( vCells.empty() ){
        if( _ndvEstimation->useDefaultValue() )
            return _ndvEstimation->defaultValue();
        else
            return _ndvEstimation->ndv();
    }

    //get the covariance matrix for the neighbors cell.
    //TODO: improve performance by saving the covariances in a covariance table/cache, since
    //      the covariances from the variogram model are function of sample separation, not their values.
    //      This would take the advantage of the regular grid geometry.
    //  This is a major bottleneck
    MatrixNXM<double> covMat = GeostatsUtils::makeCovMatrix( vCells, _ndvEstimation->vmodel() );

    //invert the covariance matrix.
    covMat.invert();

    //get the gamma matrix (covariances between sample and estimation locations)
    MatrixNXM<double> gammaMat = GeostatsUtils::makeGammaMatrix( vCells, cell, _ndvEstimation->vmodel() );

    //get the kriging weights vector: [w] = [Cov]^-1 * [gamma]
    MatrixNXM<double> weights = covMat * gammaMat;  //covMat is inverted

    //finally, compute the kriging (currently SK only)
    result = meanSK;
    std::multiset<GridCell>::iterator itSamples = vCells.begin();
    for( int i = 0; i < vCells.size(); ++i, ++itSamples){
        result += weights(i,0) * ( (*itSamples).readValueFromGrid() - meanSK );
    }

    return result;
}
