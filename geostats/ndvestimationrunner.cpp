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

    //prepare the vector with the results (to not overwrite the original data)
    _results.clear();
    _results.reserve( nI * nJ * nK );

    //for all grid cells
    for( uint k = 0; k <nK; ++k)
        for( uint j = 0; j <nJ; ++j){
            emit progress( j * nI + k * nI * nJ );
            for( uint i = 0; i <nI; ++i){
                double value = cg->dataIJK( atIndex, i, j, k );
                if( cg->isNDV( value ) )
                    _results.push_back( krige( GridCell(cg, atIndex, i,j,k), _ndvEstimation->meanForSK() ) );
                else
                    _results.push_back( value ); //simple copy from valued cells
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

    //if no sample was found, either...
    if( vCells.empty() ){
        if( _ndvEstimation->useDefaultValue() )
            //...return a default value (e.g. a global mean or expected value).
            return _ndvEstimation->defaultValue();
        else
            //...or return a no-data-value.
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
    MatrixNXM<double> weightsSK = covMat * gammaMat;  //covMat is inverted

    //finally, compute the kriging
    if( _ndvEstimation->ktype() == KrigingType::SK ){
        //for SK mode
        result = meanSK;
        std::multiset<GridCell>::iterator itSamples = vCells.begin();
        for( int i = 0; i < vCells.size(); ++i, ++itSamples){
            result += weightsSK(i,0) * ( (*itSamples).readValueFromGrid() - meanSK );
        }
    } else {
        //for OK mode
        //compute the OK weights (follows the same rationale of SK)
        //TODO: improve performance.  Just expand SK matrices with the 1.0s and 0.0s instead of computing new ones.
        MatrixNXM<double> covMatOK = GeostatsUtils::makeCovMatrix( vCells, _ndvEstimation->vmodel(), KrigingType::OK );
        covMatOK.invert();
        MatrixNXM<double> gammaMatOK = GeostatsUtils::makeGammaMatrix( vCells, cell, _ndvEstimation->vmodel(), KrigingType::OK );
        MatrixNXM<double> weightsOK = covMatOK * gammaMatOK;  //covMat is inverted
        //Estimate the OK local mean (use OK weights)
        double mOK = 0.0;
        std::multiset<GridCell>::iterator itSamples = vCells.begin();
        for( int i = 0; i < weightsOK.getN()-1; ++i, ++itSamples){ //the last element in weightsOK is the Lagrangian (mu)
            mOK += weightsOK(i,0) * (*itSamples).readValueFromGrid();
        }
        //compute the kriging weight for the local OK mean (use SK weights)
        double wmOK = 1.0;
        for( int i = 0; i < weightsSK.getN(); ++i){
            wmOK -= weightsSK(i,0);
        }
        //krige (with SK weights plus the OK mean (with OK mean weight))
        result = 0.0;
        itSamples = vCells.begin();
        for( int i = 0; i < vCells.size(); ++i, ++itSamples){
            result += weightsSK(i,0) * ( (*itSamples).readValueFromGrid() );
        }
        result += wmOK * mOK;
    }

    return result;
}
