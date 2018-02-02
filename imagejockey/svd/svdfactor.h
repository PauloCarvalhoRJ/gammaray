#ifndef SVDFACTOR_H
#define SVDFACTOR_H

#include "spectral/spectral.h"

#include <QString>
#include <QIcon>

//third-party library Eigen
namespace spectral{
   class array;
}

enum class SVDFactorPlaneOrientation : int {
	XY,
	XZ,
	YZ
};

/**
 * @brief The SVDFactor class represents one factor obtained from Singular Value Decomposition (SVD).
 */
class SVDFactor
{
public:
    /**
	 * @param factorData The array of values of the factor.
	 * @param number The factor number in the series. This number is used to make the factor name.
	 * @param weight The information contribution of the factor, normally a value between 0.0 and 1.0.
	 * @param x0 Grid origin (useful for viewers).
	 * @param y0 Grid origin (useful for viewers).
	 * @param z0 Grid origin (useful for viewers).
	 * @param dx Cell size (useful for viewers).
	 * @param dy Cell size (useful for viewers).
	 * @param dz Cell size (useful for viewers).
	 * @param parentFactor If set, this SVD factor is a decomposition of another SVD factor.
	 */
	SVDFactor(spectral::array&& factorData,
			   uint number,
			   double weight,
			   double x0, double y0, double z0,
			   double dx, double dy, double dz,
			   SVDFactor* parentFactor = nullptr );

	/** Default constructor used for the root "factor" in SVDFactorTree. */
	SVDFactor();

	virtual ~SVDFactor();

	void addChildFactor( SVDFactor* child );

	bool isSelected(){ return m_selected; }
	void setSelected( bool value ){ m_selected = value; }

	/** Assigns weights to the child factors. The default weight is 1.0 for all factors.
	 * The number of weights must be greater than or equal to the number of child factors.
	 * @return Whether assignment was successful.
	 */
	bool assignWeights( const std::vector<double>& weights );

    spectral::array& getFactorData(){ return m_factorData; }

	/** Returns a text showing the factor number reflecting its hierarchy, e.g. 4.2 meaning that it
	 * is the second factor of the fourth root factor.
	 */
	QString getHierarchicalNumber();

	/** Returns a value at the current X,Y position with respect to the currently selected plane.
	 * The select plane depends on m_currentPlaneOrientation and m_currentPlane settings.
	 * The passed coordinates are with respect to the plane and not world coordinates, for example,
	 * localY is actually a Z coordinate if current plane orientation is XZ.
	 */
	double valueAtCurrentPlane( double localX, double localY );

	/** Returns whether the passed value represents a non-informed datum. */
	bool isNDV( double value );

	/** Getters for the geometry of the currently selected plane. */
	//@{
	double getCurrentPlaneX0();
	double getCurrentPlaneDX();
	uint getCurrentPlaneNX();
	double getCurrentPlaneY0();
	double getCurrentPlaneDY();
	uint getCurrentPlaneNY();
	//@}

	double getMinValue();
	double getMaxValue();

	//@{
	/** Getters for the absolute grid geometry . */
	double getX0(){ return m_x0; }
	double getY0(){ return m_y0; }
	double getZ0(){ return m_z0; }
	double getDX(){ return m_dx; }
	double getDY(){ return m_dy; }
	double getDZ(){ return m_dz; }
	//@}

	/** Returns the number of slices.  This depens on the current plane orientation (XY, XZ, YZ). */
	uint getCurrentPlaneNumberOfSlices();

private:
    SVDFactor* m_parentFactor;
    spectral::array m_factorData;
	uint m_number;
	std::vector< SVDFactor* > m_childFactors;
	bool m_selected;
	double m_weight;
	SVDFactorPlaneOrientation m_currentPlaneOrientation;
	uint m_currentPlane;
	double m_x0, m_y0, m_z0;
	double m_dx, m_dy, m_dz;
	bool m_isMinValueDefined, m_isMaxValueDefined;
	double m_minValue, m_maxValue;
	uint getIndexOfChild( SVDFactor* child );
	bool isRoot();
	void setParentFactor( SVDFactor* parent );
	void setWeight( double weight ){ m_weight = weight; }
	bool isTopLevel();
	bool XYtoIJinCurrentPlane(double localX, double localY, uint& i, uint& j );
	double dataIJK( uint i, uint j, uint k);

	// Methods to support the QAbstractItemModel interface
public:
	SVDFactor* getChildByIndex( uint index );
	SVDFactor* getParent( );
	uint getIndexInParent( );
	uint getChildCount( );
	QString getPresentationName( );
	QIcon getIcon( );
};

#endif // SVDFACTOR_H
