#include "geostatsutils.h"

#include "gridcell.h"
#include "domain/cartesiangrid.h"
#include "spatiallocation.h"
#include "ijkdelta.h"
#include "util.h"
#include "ijkdeltascache.h"

#include <cmath>
#include <limits>

//the aniso transforms only change if variogram model changes
struct AnisoCache{
    VariogramModel* vModel;
    std::vector<Matrix3X3<double>> anisoTransforms;
} anisoCache;

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
        //TODO: using a constant power (1.5) since I don't know how it is entered in GSLib par files.
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
        Matrix3X3<double> anisoTransform;
        //improving performance by saving the aniso transforms in a cache, assuming the anisotropy
        //      is the same in all locations.
        if( anisoCache.vModel != model){
            anisoTransform = GeostatsUtils::getAnisoTransform(
                        model->get_a_hMax(i), model->get_a_hMin(i), model->get_a_vert(i),
                        model->getAzimuth(i), model->getDip(i), model->getRoll(i));
            anisoCache.anisoTransforms.push_back( anisoTransform );
        } else {
            anisoTransform = anisoCache.anisoTransforms[i];
        }

        //get the separation corrected by anisotropy
        double h = GeostatsUtils::getH( locA._x, locA._y, locA._z,
                                        locB._x, locB._y, locB._z,
                                        anisoTransform );

        result += GeostatsUtils::getGamma( model->getIt(i),
                                           h,
                                           model->get_a_hMax(i),
                                           model->getCC(i) );
    }

    //sets the cache's key (variogram model pointer) so we know whether we can reuse the aniso transforms
    //saved in the cache
    anisoCache.vModel = model;

    return result;
}

MatrixNXM<double> GeostatsUtils::makeCovMatrix(std::multiset<GridCell> &samples,
                                               VariogramModel *variogramModel,
                                               double variogramSill,
                                               KrigingType kType)
{
    //Define the dimension of cov matrix, which depends on kriging type
    int append = 0;
    switch( kType ){
    case KrigingType::SK:
        append = 0; break;
    case KrigingType::OK:
        append = 1; break;
    }

    //Create the cov matrix.
    MatrixNXM<double> covMatrix( samples.size() + append, samples.size() + append );

    //convert the std::multiset into a std::vector for faster traversal
    //(memory locality and less pointer chasing)
    std::vector<GridCell> samplesV;
    samplesV.reserve( samples.size() );
    std::copy(samples.begin(), samples.end(), std::back_inserter(samplesV));

    //For each sample.
    std::vector<GridCell>::iterator rowsIt = samplesV.begin();
    for( int i = 0; rowsIt != samplesV.end(); ++rowsIt, ++i ){
        GridCell rowCell = *rowsIt;
        //For each sample.
        std::vector<GridCell>::iterator colsIt = samplesV.begin();
        for( int j = 0; colsIt != samplesV.end(); ++colsIt, ++j ){
            GridCell colCell = *colsIt;
            //get semi-variance value from the separation between two samples in a pair
            double gamma = GeostatsUtils::getGamma( variogramModel, rowCell._center, colCell._center );
            //get covariance for the sample pair and assign it the corresponding element in the
            //cov matrix
            covMatrix(i, j) = variogramSill - gamma;
        }
    }

    //prepare the cov matrix for an OK system, if this is the case.
    switch( kType ){
    case KrigingType::SK: break;
    case KrigingType::OK:
        int dim = samples.size();
        for( int i = 0; i < dim; ++i ){
            covMatrix( dim, i ) = 1.0; //last row with ones
			covMatrix( i, dim ) = 1.0; //last column with ones
        }
        covMatrix( dim, dim ) = 0.0; //last element is zero
    }

    return covMatrix;
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
    case KrigingType::SK: break;
    case KrigingType::OK:
        result( samples.size(), 0 ) = 1.0; //last element is one
    }

    return result;
}

void GeostatsUtils::getValuedNeighborsTopoOrdered(GridCell &cell,
                                                        int numberOfSamples,
                                                        int nColsAround,
                                                        int nRowsAround,
                                                        int nSlicesAround,
                                                        bool hasNDV,
                                                        double NDV,
                                                        std::multiset<GridCell> &list)
{
    CartesianGrid* cg = cell._grid;
    if( ! cg ){
        Application::instance()->logError("GeostatsUtils::getValuedNeighborsTopoOrdered(): null grid.  Returning empty list.");
    }

    //get the grid limits
    int row_limit = cg->getNY();
    int column_limit = cg->getNX();
    int slice_limit = cg->getNZ();

    //generate all possible ijk deltas up to the neighborhood limits
    //the list of deltas is ordered by resulting distance with respect to a target cell
    //////////the block of code below is considered optimal (speed)
    std::vector<IJKDelta> *deltasV = nullptr;
    //try to reuse a list from the cache since making one anew is costly and the neighborhood does not change
    IJKDeltasCacheMap::iterator itcache =
            IJKDeltasCache::cache.find( IJKDeltasCacheKey( nColsAround, nRowsAround, nSlicesAround ) );
    if( itcache != IJKDeltasCache::cache.end() ){ //cache hit
        deltasV = itcache->second;
    } else { //cache miss, have to build the list
        //build the list as a set to get free ordering
        std::set<IJKDelta> *deltas = new std::set<IJKDelta>();
        for( int dk = 0; dk <= nSlicesAround/2; ++dk){
			for( int dj = 0; dj <= nRowsAround/2; ++dj ){
				for( int di = 0; di <= nColsAround/2; ++di){
                    deltas->insert( IJKDelta( di, dj, dk) );
                }
            }
        }
        //the first element is always delta 0,0,0 (target cell itself)
        deltas->erase( deltas->begin() );
        //now that std::set assured an ordered collection, transform it into an std::vector
        //which is faster to iterate because there is no pointer chasing and std::vector elements
        //are stored in continuous blocks of memory (memory locality increases cache hits)
        deltasV = new std::vector<IJKDelta>();
        deltasV->reserve( deltas->size() );
        std::copy(deltas->begin(), deltas->end(), std::back_inserter(*deltasV));
        //store the list in the cache for later reuse.
        IJKDeltasCache::cache.emplace( IJKDeltasCacheKey( nColsAround,
														  nRowsAround,
														  nSlicesAround ), deltasV );
        //we don't need the std::set anymore
        delete deltas;
    }

    if( !deltasV || deltasV->empty() ){ //hope the second is not evaluated if deltas == nullptr
        Application::instance()->logError("GeostatsUtils::getValuedNeighborsTopoOrdered(): null neighborhood.  Returning empty list.");
    }
    //////////////////////////////////////////////////

    //for each delta...
    std::vector<IJKDelta>::const_iterator it = deltasV->cbegin();
    IJKIndex indexes[8]; //eight indexes is the most possible (3 degrees of freedom)
    for(; it != deltasV->end(); ++it){
        const IJKDelta &delta = (*it);
        //...get all the indexes from a delta (can yield 2, 4 or 8 coordinates, depending on the delta's degrees of freedom).
        // TODO: Performance: that getIndexes() function has improved, but maybe there is still some room for improvement
        int countIndexes = delta.getIndexes( cell._indexIJK, indexes );
        //for each topological coordinate (IJK index)...
        for( int iIndex = 0; iIndex < countIndexes; ++iIndex){
            int ii = indexes[iIndex]._i;
            int jj = indexes[iIndex]._j;
            int kk = indexes[iIndex]._k;
            //...if the index is within the grid limits...
			if( ii >= 0 && ii < column_limit &&
				jj >= 0 && jj < row_limit &&
                kk >= 0 && kk < slice_limit ){
                //...get the value corresponding to the cell index.
                double value = cg->dataIJK( cell._dataIndex, ii, jj, kk );
                //if the cell is valued... DataFile::hasNDV() is slow.
                if( !hasNDV || !Util::almostEqual2sComplement( NDV, value, 1 ) ){
                    //...it is a valid neighbor.
                    GridCell currentCell( cg, cell._dataIndex, ii, jj, kk );
                    currentCell.computeTopoDistance( cell );
                    list.insert( currentCell );
                    //if the number of neighbors is reached...
                    if( list.size() == (unsigned)numberOfSamples )
                        //...interrupt the search
                        return;
                }
            }
        }
    }
}
