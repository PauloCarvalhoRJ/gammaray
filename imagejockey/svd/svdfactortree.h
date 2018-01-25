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
    SVDFactorTree();
	virtual ~SVDFactorTree();

	/** Adds a SVD Factor under the root node of this tree. All factors are deleted upon destruction of this object. */
	void addFirstLevelFactor( SVDFactor* factor );

	/** Assigns weights to the top level factors of the tree. The default weight is 1.0 for all factors.
	 * The number of weights must match the number of top level factors.
	 * @return Whether assignment was successful.
	 */
	bool assignWeights( const std::vector<double>& weights );

private:
	SVDFactor *m_rootFactor;

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
