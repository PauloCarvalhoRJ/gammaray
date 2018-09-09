#include "searchellipsoid.h"
#include <utility>
#include <iostream>

SearchEllipsoid::SearchEllipsoid(double hMax, double hMin, double hVert,
                                  double azimuth, double dip, double roll ,
                                  uint numberOfSectors, uint minSamplesPerSector, uint maxSamplesPerSector) :
	m_hMax( hMax ), m_hMin( hMin ), m_hVert( hVert ),
    m_azimuth( azimuth ), m_dip( dip ), m_roll( roll ),
    m_numberOfSectors( numberOfSectors), m_minSamplesPerSector( minSamplesPerSector), m_maxSamplesPerSector( maxSamplesPerSector )
{
	setRotationTransform();
}

SearchEllipsoid::SearchEllipsoid(SearchEllipsoid && right_hand_side) :
	SearchNeighborhood( std::move( right_hand_side ) ), //moves the base part of the object
	m_hMax( right_hand_side.m_hMax ),
	m_hMin( right_hand_side.m_hMin ),
	m_hVert( right_hand_side.m_hVert ),
	m_azimuth( right_hand_side.m_azimuth ),
	m_dip( right_hand_side.m_dip ),
	m_roll( right_hand_side.m_roll )
{
	setRotationTransform();
}

void SearchEllipsoid::getBBox( double centerX, double centerY, double centerZ,
							   double & minX, double & minY, double & minZ,
							   double & maxX, double & maxY, double & maxZ ) const
{
    if( m_azimuth || m_dip || m_roll ){

        //------------------------ The bounding box of a rotated ellipsoid -------------------------
        // Let:
        //    a, b, c : the lengths of the semi-axes
        //    xv, yv, zv : the vector of the position of the center (v)
        //    xa, ya, za : a vector that gives the direction of the major semi-axis with respect to its center
        //    xb, yb, zb : a vector that gives the direction of the minor semi-axis with respect to its center
        //    xc, yc, zc : a vector that gives the direction of the vertical semi-axis with respect to its center
        //    x1, y1, z1 : the eastmost, southmost, bottom bounding box corner
        //    x2, y2, z2 : the westmost, northmost, upper bounding box corner
        //    x : the vector of Cartesian coordinates of a point in space.
        //    A : any matrix
        //    V : matrix of A eigenvectors (as columns)
        //    L : matrix of A eigenvalues (as main diagonal elements)
        //
        // The equation of the ellipsoid (as any quadric) in matrix form is:
        //    ( x - v )^T * A * ( x - v )
        //
        // The eigenvalues of A are: a^-2, b^-2, c^-2
        //
        // The eigenvectors of A are: [xa ya za]^T, [xb yb zb]^T, [xc yc zc]^T
        //
        // Any matrix A can be computed from its eigenvalues and eigenvectors by:
        //    A = V * L * V^-1
        //
        // Invert A -> The elements of the main diagonal are lx^2, ly^2, lz^2.
        //
        // Finally, the bounding box coordinates can be computed with:
        //    x1 = xv - lx
        //    x2 = xv + lx
        //    y1 = yv - ly
        //    y2 = yv + ly
        //    z1 = zv - lz
        //    z2 = Zv + lz
        //

        //TODO: Performance: with non-LVA, the bounding box only need to be computed once.
        // and the limits can be computed by merely shifing it along the center, as the axes and
        // angles are not expected to change.

        //-----------get the center---------------
        double xv = centerX;
        double yv = centerY;
        double zv = centerZ;

        //------------get the axes direction vectors------------
        // By GSLIB convention, zero azimuth means that the semi-major axis points to north (Y axis).
        double xa = 0.0; double ya = 1.0; double za = 0.0;
        double xb = 1.0; double yb = 0.0; double zb = 0.0;
        double xc = 0.0; double yc = 0.0; double zc = 1.0;
        //---apply the rotation to the axes
        GeostatsUtils::transform( m_rotationTransform, xa, ya, za );
        GeostatsUtils::transform( m_rotationTransform, xb, yb, zb );
        GeostatsUtils::transform( m_rotationTransform, xc, yc, zc );

        //-----------make the eigenvalues matrix---------------
        Matrix3X3<double> L;
        L._a11 = 1 / ( m_hMax*m_hMax ); L._a12 = 0.0; L._a13 = 0.0;
        L._a21 = 0.0; L._a22 = 1 / ( m_hMin*m_hMin ); L._a23 = 0.0;
        L._a31 = 0.0; L._a32 = 0.0; L._a33 = 1 / ( m_hVert*m_hVert );

        //-----------make the eigenvectors matrix---------------
        Matrix3X3<double> V;
        V._a11 = xa; V._a12 = xb; V._a13 = xc;
        V._a21 = ya; V._a22 = yb; V._a23 = yc;
        V._a31 = za; V._a32 = zb; V._a33 = zc;

        //-----------invert the eigenvectors matrix--------------
        Matrix3X3<double> V_inv( V );
        V_inv.invert();

        //-----------compute inverse of matrix A----------------------------
        Matrix3X3<double> A_inv( V * L * V_inv );
        A_inv.invert();

        //-----------compute the limits-----------------------
        double l1 = std::sqrt( A_inv._a11 );
        double l2 = std::sqrt( A_inv._a22 );
        double l3 = std::sqrt( A_inv._a33 );
        minX = xv - l1;
        maxX = xv + l1;
        minY = yv - l2;
        maxY = yv + l2;
        minZ = zv - l3;
        maxZ = zv + l3;

    } else {

        //The trivial case (all angles == 0.0)
        //Without azimuth, it is assumed zero.  By convention zero azimuth is north,
        //thus hMax points to north (Y axis).
        maxY = centerY + m_hMax;
        minY = centerY - m_hMax;
        maxX = centerX + m_hMin;
        minX = centerX - m_hMin;
        maxZ = centerZ + m_hVert;
        minZ = centerZ - m_hVert;

    }
}

bool SearchEllipsoid::isInside( double centerX, double centerY, double centerZ,
								double x,       double y,       double z        ) const
{
	double localX = x - centerX;
	double localY = y - centerY;
	double localZ = z - centerZ;
	GeostatsUtils::transform( m_rotationTransform, localX, localY, localZ );
	//Without azimuth, it is assumed zero.  By convention zero azimuth is north, this hMax points to north (Y axis).
	double dx = localX/m_hMin;
	double dy = localY/m_hMax;
	double dz = localZ/m_hVert;
    return dx*dx + dy*dy + dz*dz <= 1.0;
}

void SearchEllipsoid::performSpatialFilter(std::vector<SpatialLocationPtr> &samplesLocations) const
{

}

void SearchEllipsoid::setRotationTransform()
{
    //90 degrees is added to the azimuth because this method subtracts 90 from it (necessary for rotating
    //variograms for kriging).
    m_rotationTransform = GeostatsUtils::getAnisoTransform( 1.0, 1.0, 1.0, 90.0 + m_azimuth, m_dip, m_roll );
}
