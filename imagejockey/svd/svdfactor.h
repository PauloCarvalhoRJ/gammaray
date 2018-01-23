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
			   SVDFactor* parentFactor = nullptr );
private:
    SVDFactor* m_parentFactor;
    spectral::array m_factorData;
	uint m_number;
	std::vector< SVDFactor > m_childFactors;
	uint getIndexOfChild( SVDFactor* child );

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
