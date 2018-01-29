#ifndef SVDFACTOR_H
#define SVDFACTOR_H

#include "spectral/spectral.h"

#include <QString>
#include <QIcon>

//third-party library Eigen
namespace spectral{
   class array;
}

/**
 * @brief The SVDFactor class represents one factor obtained from Singular Value Decomposition (SVD).
 */
class SVDFactor
{
public:
    /**
     * @param parentFactor If set, this SVD factor is a decomposition of another SVD factor.
	 * @param number The factor number in the series. This number is used to make the factor name.
     */
	SVDFactor( spectral::array&& factorData,
			   uint number,
			   double weight,
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

private:
    SVDFactor* m_parentFactor;
    spectral::array m_factorData;
	uint m_number;
	std::vector< SVDFactor* > m_childFactors;
	bool m_selected;
	double m_weight;
	uint getIndexOfChild( SVDFactor* child );
	bool isRoot();
	void setParentFactor( SVDFactor* parent );
	void setWeight( double weight ){ m_weight = weight; }
	bool isTopLevel();

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
