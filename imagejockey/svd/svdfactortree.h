#ifndef SVDFACTORTREE_H
#define SVDFACTORTREE_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QModelIndexList>
#include <vector>
#include "svdfactor.h"

/**
 * @brief The SVDFactorTree class is a collection of an SVD decomposition hierarchy (SVD factors can be decomposed into
 * SVD factors themselves).
 */
class SVDFactorTree : public QAbstractItemModel
{
public:
	/** @param mergeThreshold the minimum ammount of information content per top-level factor, e.g. 0.1 == 10%.
	 *         Low information factors are merged until reaching this value.  Setting a value less than or equal to zero
	 *         causes all factors to become top level.
	 */
	SVDFactorTree( double mergeThreshold );
	virtual ~SVDFactorTree();

	/** Adds a SVD Factor under the root node of this tree. All factors are deleted upon destruction of this object. */
	void addFirstLevelFactor( SVDFactor* factor );

    /** Returns a new array object containing the sum of the selected leaf factors in the tree.
     * The caller is responsible to deallocate the created object.
     * Returns null pointer if there are no factors in the tree.
     * Returns an array filled with zeroes if no factors are selected.
     */
    spectral::array *getSumOfSelectedFactors();

    /**
     * Returns one of the top level factors in the tree given its index.
     * Returns null pointer if the tree is empty, the root factor is undefined or the index is invalid.
     */
    SVDFactor* getOneTopLevelFactor( uint index );

private:
	SVDFactor *m_rootFactor;
	double m_mergeThreshold;

	// QAbstractItemModel interface
public:
	QModelIndex index(int row, int column, const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &child) const;
	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation,
						int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
};

#endif // SVDFACTORTREE_H
