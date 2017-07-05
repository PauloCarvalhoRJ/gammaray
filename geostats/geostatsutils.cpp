#include "geostatsutils.h"

#include "gridcell.h"
#include "domain/cartesiangrid.h"
#include "spatiallocation.h"
#include "ijkdelta.h"
#include "util.h"

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

    //convert the angles to radians and to trigonometric convention
    double azimuthRad = (azimuth - 90.0) * Util::PI_OVER_180;
    double dipRad = dip * Util::PI_OVER_180;
    double rollRad = roll * Util::PI_OVER_180;

    //----------rotate the world so the aniso axes are parallel to world axes--------------------------
    Matrix3X3<double> Tyaw( std::cos(azimuthRad), -std::sin(azimuthRad), 0.0,
                            std::sin(azimuthRad), std::cos(azimuthRad),  0.0,
                            0.0,               0.0,                1.0);
    Matrix3X3<double> Tpitch( std::cos(dipRad),  0.0, std::sin(dipRad),
                              0.0,            1.0,           0.0,
                              -std::sin(dipRad), 0.0, std::cos(dipRad));
    Matrix3X3<double> Troll( 1.0,            0.0,             0.0,
                             0.0, std::cos(rollRad), std::sin(rollRad),
                             0.0, -std::sin(rollRad), std::cos(rollRad));
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
    double h_over_a = h/range;
    switch( permissiveModel ){
    case VariogramStructureType::SPHERIC:
        if( h > range )
            return contribution;
        return contribution * ( 1.5*h_over_a - 0.5*(h_over_a*h_over_a*h_over_a) );
    case VariogramStructureType::EXPONENTIAL:
        if( Util::almostEqual2sComplement( h, 0.0, 1 ) )
            return 0.0;
        return contribution * ( 1.0 - std::exp(-3.0 * h_over_a) );
    case VariogramStructureType::GAUSSIAN:
        h_over_a = h/range;
        if( Util::almostEqual2sComplement( h, 0.0, 1 ) )
            return 0.0;
        return contribution * ( 1.0 - std::exp(-9.0*(h_over_a*h_over_a)) );
    case VariogramStructureType::POWER_LAW:
        //TODO: using a constant power (1.5) since I don't know how it entered in GSLib par files.
        Application::instance()->logWarn("GeostatsUtils::getGamma(): Power model using a constant power == 1.5");
        return contribution * std::pow(h, 1.5);
    case VariogramStructureType::COSINE_HOLE_EFFECT:
        return contribution * ( 1.0 - std::cos( h_over_a * Util::PI ) );
    default:
        Application::instance()->logError("GeostatsUtils::getGamma(): Unknown structure type.  Assuming spheric.");
        if( h > range )
            return contribution;
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
                                               VariogramModel *variogramModel,
                                               KrigingType kType)
{
    int append = 0;
    switch( kType ){
    case KrigingType::SK:
        append = 0; break;
    case KrigingType::OK:
        append = 1; break;
    }

    MatrixNXM<double> result( samples.size() + append, samples.size() + append );

    std::multiset<GridCell>::iterator rowsIt = samples.begin();

    for( int i = 0; rowsIt != samples.end(); ++rowsIt, ++i ){
        GridCell rowCell = *rowsIt;
        std::multiset<GridCell>::iterator colsIt = samples.begin();
        for( int j = 0; colsIt != samples.end(); ++colsIt, ++j ){
            GridCell colCell = *colsIt;
            //get semi-variance value
            double gamma = GeostatsUtils::getGamma( variogramModel, rowCell._center, colCell._center );
            //get covariance
            result(i, j) = variogramModel->getSill() - gamma;
        }
    }

    //prepare the cov matrix for an OK system, if this is the case.
    switch( kType ){
    case KrigingType::OK:
        int dim = samples.size();
        for( int i = 0; i < dim; ++i ){
            result( dim, i ) = 1.0; //last row with ones
            result( i, dim ) = 1.0; //last columns with ones
        }
        result( dim, dim ) = 0.0; //last element is zero
    }

    return result;
}

MatrixNXM<double> GeostatsUtils::makeGammaMatrix(std::multiset<GridCell> &samples,
                                                 GridCell &estimationLocation,
                                                 VariogramModel *variogramModel, KrigingType kType)
{
    int append = 0;
    switch( kType ){
    case KrigingType::SK:
        append = 0; break;
    case KrigingType::OK:
        append = 1; break;
    }

    MatrixNXM<double> result( samples.size()+append, 1 );

    std::multiset<GridCell>::iterator rowsIt = samples.begin();

    for( int i = 0; rowsIt != samples.end(); ++rowsIt, ++i ){
        GridCell rowCell = *rowsIt;
        //get semi-variance value
        double gamma = GeostatsUtils::getGamma( variogramModel, rowCell._center, estimationLocation._center );
        //get covariance
        result(i, 0) = variogramModel->getSill() - gamma;
    }

    //prepare the matrix for an OK system, if this is the case.
    switch( kType ){
    case KrigingType::OK:
        result( samples.size(), 0 ) = 1.0; //last element is one
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

    //get the grid limits
    int row_limit = cg->getNY();
    int column_limit = cg->getNX();
    int slice_limit = cg->getNZ();

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

    //for each delta...
    std::set<IJKDelta>::iterator it = deltas.begin();
    for(; it != deltas.end(); ++it){
        IJKDelta delta = (*it);
        //...get all the indexes from a delta (can yield 2, 4 or 8 coordinates, depending on the delta's degrees of freedom).
        std::set<IJKIndex> indexes = delta.getIndexes( cell._indexIJK );
        //for each topological coordinate (IJK index)...
        std::set<IJKIndex>::iterator itIndexes = indexes.begin();
        for(; itIndexes != indexes.end(); ++itIndexes){
            int ii = (*itIndexes)._i;
            int jj = (*itIndexes)._j;
            int kk = (*itIndexes)._k;
            //...if the index is within the grid limits...
            if( ii >= 0 && ii < row_limit &&
                jj >= 0 && jj < column_limit &&
                kk >= 0 && kk < slice_limit ){
                //...get the value corresponding to the cell index.
                double value = cg->dataIJK( cell._dataIndex, ii, jj, kk );
                //if the cell is valued...
                if( ! cg->isNDV( value ) ){
                    //...it is a valid neighbor.
                    GridCell currentCell( cg, cell._dataIndex, ii, jj, kk );
                    currentCell.computeTopoDistance( cell );
                    result.insert( currentCell );
                    //if the number of neighbors is reached...
                    if( result.size() == (unsigned)numberOfSamples )
                        //...interrupt the search
                        goto completed;
                }
            }
        }
    }

completed:  //from goto in the loop above (or if the loops complete)

    //makes sure only the closest n cells at most
    //while( result.size() > numberOfSamples )
    //    result.erase( --result.end() );

    return result;
}
