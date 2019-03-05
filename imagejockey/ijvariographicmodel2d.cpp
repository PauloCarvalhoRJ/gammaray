#include "ijvariographicmodel2d.h"
#include "imagejockey/ijabstractcartesiangrid.h"

#include <cassert>


IJVariographicStructure2D::IJVariographicStructure2D(double pRange,
                                                     double pRangeRatio,
                                                     double pAzimuth,
                                                     double pContribution) :
    range        ( pRange ),
    rangeRatio   ( pRangeRatio ),
    azimuth      ( pAzimuth ),
    contribution ( pContribution )
{
}

int IJVariographicStructure2D::getNumberOfParameters()
{
    return 4;
}

double IJVariographicStructure2D::getParameter( int index ) const
{
    assert( index >= 0 && index < 4 && "IJVariographicModel2D::getParameter(): parameter index must be between 0 and 3." );
    switch( index ){
    case 0: return range;
    case 1: return rangeRatio;
    case 2: return azimuth;
    case 3: return contribution;
    }
    return -999.999;
}

void IJVariographicStructure2D::setParameter( int index, double value )
{
    assert( index >= 0 && index < 4 && "IJVariographicModel2D::setParameter(): parameter index must be between 0 and 3." );
    switch( index ){
    case 0: range = value; return;
    case 1: rangeRatio = value; return;
    case 2: azimuth = value; return;
    case 3: contribution = value; return;
    }
}

void IJVariographicStructure2D::addContributionToModelGrid(IJAbstractCartesianGrid& gridWithGeometry,
                                                            spectral::array &targetGrid,
                                                            IJVariogramPermissiveModel model,
                                                            bool invert )
{
    //assess grid geometry
    int nI = gridWithGeometry.getNI();
    int nJ = gridWithGeometry.getNJ();
    int nK = gridWithGeometry.getNK();
    double xc = gridWithGeometry.getCenterX();
    double yc = gridWithGeometry.getCenterY();

    //get variographic parameters
    double a     = range;
    double b     = a * rangeRatio;
    double c     = contribution;
    double theta = azimuth;

    //for each cell in the variographic surface
    for( int k = 0; k < nK; ++k )
        for( int j = 0; j < nJ; ++j )
            for( int i = 0; i < nI; ++i ){
                //evaluate the variographic model at the cell location
                double cellX, cellY, cellZ;
                gridWithGeometry.getCellLocation( i, j, k, cellX, cellY, cellZ );
                double x = (cellX - xc) * std::cos(theta) - (cellY - yc) * std::sin(theta);
                double y = (cellX - xc) * std::sin(theta) + (cellY - yc) * std::cos(theta);
                double h = std::sqrt((x/a)*(x/a) + (y/b)*(y/b));
                double semivariance;
                //invert the semivariance values to match the geometry of the varmap
                assert( model == IJVariogramPermissiveModel::SPHERIC && "IJVariographicStructure2D::addContributionToModelGrid(): Currently only spheric model was implemented." );
                if( h >= 0.0 and h <= 1.0 )
                   semivariance = c * ( 3.0 * h/2.0 - h*h*h/2.0 ); //spheric model
                else
                   semivariance = c ;
                if( invert )
                   semivariance = c - semivariance;
                targetGrid( i, j, k ) += semivariance;
            }
}


IJVariographicModel2D::IJVariographicModel2D( double pNugget ) :
    nugget( pNugget )
{
}
