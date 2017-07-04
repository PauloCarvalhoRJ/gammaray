#include "geostatsutils.h"

#include "gridcell.h"
#include "domain/cartesiangrid.h"
#include "spatiallocation.h"
#include "ijkdelta.h"

#include <cmath>
#include <limits>

GeostatsUtils::GeostatsUtils()
{
}

Matrix3X3<double> GeostatsUtils::getAnisoTransform(double aSemiMajor,
                                                   double aSemiMinor,
                                                   double aSemiVert,
                                                   double azimuth,
                                                   double dip,
                                                   double roll)
{
    //----------rotate the world so the aniso axes are parallel to world axes--------------------------
    Matrix3X3<double> Tyaw( std::cos(azimuth), -std::sin(azimuth), 0.0,
                            std::sin(azimuth), std::cos(azimuth),  0.0,
                            0.0,               0.0,                1.0);
    Matrix3X3<double> Tpitch( std::cos(dip),  0.0, std::sin(dip),
                              0.0,            1.0,           0.0,
                              -std::sin(dip), 0.0, std::cos(dip));
    Matrix3X3<double> Troll( 1.0,            0.0,             0.0,
                             0.0, std::cos(roll), -std::sin(roll),
                             0.0, std::sin(roll), std::cos(roll));
    //----------stretches the world so the aniso ranges are now equal (spherical) --------------------
    Matrix3X3<double> S( 1.0,                   0.0,                  0.0,
                         0.0, aSemiMajor/aSemiMinor,                  0.0,
                         0.0,                   0.0, aSemiMajor/aSemiVert);
    //     <--------- order of transform application
    //  the final effect is that we transform the world so the anisotropy becomes isotropy
    return       S * Troll * Tpitch * Tyaw;
}

void GeostatsUtils::transform(Matrix3X3<double> &t, double &a1, double &a2, double &a3)
{
    double temp_a1 = t._a11 * a1 + t._a12 * a2 + t._a13 * a3;
    double temp_a2 = t._a21 * a1 + t._a22 * a2 + t._a23 * a3;
    double temp_a3 = t._a31 * a1 + t._a32 * a2 + t._a33 * a3;
    a1 = temp_a1;
    a2 = temp_a2;
    a3 = temp_a3;
}

double GeostatsUtils::getH(double x0, double y0, double z0,
                           double x1, double y1, double z1,
                           Matrix3X3<double> &anisoTransform)
{
    double dx = x1 - x0;
    double dy = y1 - y0;
    double dz = z1 - z0;
    GeostatsUtils::transform( anisoTransform, dx, dy, dz );
    return std::sqrt( dx*dx + dy*dy + dz*dz );
}

double GeostatsUtils::getGamma(VariogramStructureType permissiveModel, double h, double range, double contribution)
{
    double h_over_a;
    switch( permissiveModel ){
    case VariogramStructureType::SPHERIC:
        if( h > range )
            return contribution;
        h_over_a = h/range;
        return contribution * ( 1.5*h_over_a - 0.5*(h_over_a*h_over_a*h_over_a) );
        break;
    default:
        Application::instance()->logError("GeostatsUtils::getGamma(): Unsupported structure type.  Assuming spheric.");
        if( h > range )
            return contribution;
        h_over_a = h/range;
        return contribution * ( 1.5*h_over_a - 0.5*(h_over_a*h_over_a*h_over_a) );
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double GeostatsUtils::getGamma(VariogramModel *model, SpatialLocation &locA, SpatialLocation &locB)
{
    //lesser bottleneck
    double result = model->getNugget();
    int nst = model->getNst();

    for( int i = 0; i < nst; ++i){
        //TODO: improve performance by saving the aniso transforms in a cache, assuming the anisotropy
        //      is the same in all locations.
        //major bottleneck
        Matrix3X3<double> anisoTransform = GeostatsUtils::getAnisoTransform(
                    model->get_a_hMax(i), model->get_a_hMin(i), model->get_a_vert(i),
                    model->getAzimuth(i), model->getDip(i), model->getRoll(i));

        double h = GeostatsUtils::getH( locA._x, locA._y, locA._z,
                                        locB._x, locB._y, locB._z,
                                        anisoTransform );

        //bottleneck
        result += GeostatsUtils::getGamma( model->getIt(i),
                                           h,
                                           model->get_a_hMax(i),
                                           model->getCC(i) );
    }
    return result;
}

MatrixNXM<double> GeostatsUtils::makeCovMatrix(std::multiset<GridCell> &samples,
                                               VariogramModel *variogramModel)
{
    MatrixNXM<double> result( samples.size(), samples.size() );

    std::multiset<GridCell>::iterator rowsIt = samples.begin();

    for( int i = 0; rowsIt != samples.end(); ++rowsIt, ++i ){
        GridCell rowCell = *rowsIt;
        std::multiset<GridCell>::iterator colsIt = samples.begin();
        for( int j = 0; colsIt != samples.end(); ++colsIt, ++j ){
            GridCell colCell = *colsIt;
            double cov = GeostatsUtils::getGamma( variogramModel, rowCell._center, colCell._center );
            result(i, j) = cov;
        }
    }
    return result;
}

MatrixNXM<double> GeostatsUtils::makeGammaMatrix(std::multiset<GridCell> &samples,
                                                 GridCell &estimationLocation,
                                                 VariogramModel *variogramModel)
{
    MatrixNXM<double> result( samples.size(), 1 );

    std::multiset<GridCell>::iterator rowsIt = samples.begin();

    for( int i = 0; rowsIt != samples.end(); ++rowsIt, ++i ){
        GridCell rowCell = *rowsIt;
        double cov = GeostatsUtils::getGamma( variogramModel, rowCell._center, estimationLocation._center );
        result(i, 0) = cov;
    }
    return result;
}

std::multiset<GridCell> GeostatsUtils::getValuedNeighborsTopoOrdered(GridCell &cell,
                                                        int numberOfSamples,
                                                        int nColsAround,
                                                        int nRowsAround,
                                                        int nSlicesAround)
{
    std::multiset<GridCell> result;
    CartesianGrid* cg = cell._grid;
    if( ! cg ){
        Application::instance()->logError("GeostatsUtils::getValuedNeighborsTopoOrdered(): null grid.  Returning empty list.");
        return result;
    }
    int row_limit = cg->getNY();
    int column_limit = cg->getNX();
    int slice_limit = cg->getNZ();
    int i = cell._i;
    int j = cell._j;
    int k = cell._k;
    int shell = 1; //shell of neighbors around the target cell (start with 1 cell distant).

    //generate all possible ijk deltas up to the neighborhood limits
    //the list of deltas is ordered by resulting distance with respect to a target cell
    //TODO: improve performance.  This list can be cached since the search neighborhood does not change.
    std::set<IJKDelta> deltas;
    {
        for( int dk = 0; dk <= nSlicesAround/2; ++dk){
            for( int dj = 0; dj <= nColsAround/2; ++dj ){
                for( int di = 0; di <= nRowsAround/2; ++di){
                    deltas.insert( IJKDelta( di, dj, dk) );
                }
            }
        }
        //the first element is always delta 0,0,0 (target cell itself)
        deltas.erase( deltas.begin() );
    }

    if( deltas.empty() ){
        Application::instance()->logError("GeostatsUtils::getValuedNeighborsTopoOrdered(): null neighborhood.  Returning empty list.");
        return result;
    }

    std::set<IJKDelta>::iterator it = deltas.begin();
    for(; it != deltas.end(); ++it){
        IJKDelta delta = (*it);

    }


    //TODO: this algorithm is wrong.

    //collect the neighbors starting from those immediately adjacent then outwards
    for( ; ; ++shell ){ //shell expanding loop
        for(int kk = std::max({0, k-shell, k-nSlicesAround/2}); kk < std::min({k+shell, slice_limit, k+nSlicesAround/2+1}); ++kk){
            for(int jj = std::max({0, j-shell, j-nColsAround/2}); jj < std::min({j+shell, column_limit, j+nColsAround/2+1}); ++jj){
                for(int ii = std::max({0, i-shell, i-nRowsAround/2}); ii < std::min({i+shell, row_limit, i+nRowsAround/2+1}); ++ii){
                    if(ii != i || jj != j || kk != k ){
                        double value = cg->dataIJK( cell._dataIndex, ii, jj, kk );
                        if( ! cg->isNDV( value ) ){
                            GridCell currentCell( cg, cell._dataIndex, ii, jj, kk );
                            currentCell.computeTopoDistance( cell );
                            result.insert( currentCell );
                            if( result.size() == (unsigned)numberOfSamples * 2 ) //TODO: * 2 is a heuristic (think of a better way to collect the n-closest samples
                                goto completed; //abort collect loop if a suficient number of values is reached (no need for traversing all neighbouring cells
                        }
                    }
                }
            }
        }
    }
completed:  //from goto in the loop above (or if the loops complete)

    //keep only the closest n cells at most
    while( result.size() > numberOfSamples )
        result.erase( --result.end() );

    return result;
}
