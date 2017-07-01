#include "ndvestimation.h"

#include "domain/cartesiangrid.h"
#include "domain/attribute.h"
#include "gridcell.h"
#include "geostatsutils.h"

NDVEstimation::NDVEstimation(Attribute *at) :
    _at(at),
    _searchMaxNumSamples(16),
    _searchNumCols(10),
    _searchNumRows(10),
    _searchNumSlices(1)
{}

void NDVEstimation::run()
{
    //Assumes the partent file of the selected attribut is a Cartesian grid
    CartesianGrid *cg = (CartesianGrid*)_at->getContainingFile();

    //gets the Attribute's column in its Cartesian grid's data array (GEO-EAS index - 1)
    uint atIndex = _at->getAttributeGEOEASgivenIndex() - 1;

    //get the grid dimensions
    uint nI = cg->getNX();
    uint nJ = cg->getNY();
    uint nK = cg->getNZ();

    //traverse the grid cells to run the estimation
    //on unvalued ones
    for( uint k = 0; k <nK; ++k)
        for( uint j = 0; j <nJ; ++j)
            for( uint i = 0; i <nI; ++i){
                double value = cg->dataIJK( atIndex, i, j, k );
                if( cg->isNDV( value ) ){
                    krige( GridCell(cg, atIndex, i,j,k) );
                }
            }
}

void NDVEstimation::setSearchParameters(int searchMaxNumSamples,
                                        int searchNumCols,
                                        int searchNumRows,
                                        int searchNumSlices)
{
    _searchMaxNumSamples=searchMaxNumSamples;
    _searchNumCols=searchNumCols;
    _searchNumRows=searchNumRows;
    _searchNumSlices=searchNumSlices;
}

void NDVEstimation::krige(GridCell cell)
{
    //Assumes the partent file of the selected attribut is a Cartesian grid
    CartesianGrid *cg = (CartesianGrid*)_at->getContainingFile();

    //collects valued n-neighbors ordered by their topological distance with respect
    //to the target cell
    std::multiset<GridCell> vCells = GeostatsUtils::getValuedNeighborsTopoOrdered( cell,
                                                           _searchMaxNumSamples,
                                                           _searchNumCols,
                                                           _searchNumRows,
                                                           _searchNumSlices );

    //get the covariance matrix for the neighbors cell.
    //TODO: imprve performance by saving the covariances in a covariance table/cache, since
    //      the covariances from the variogram model are function of sample separation, not their values.
    //      This would take de advantage of the regular grid geometry
    GeostatsUtils::makeCovMatrix( vCells, anisoTransform, variogramModel);
}
