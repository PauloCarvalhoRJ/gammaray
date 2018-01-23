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

    void addFactor( SVDFactor&& factor );

private:
    std::vector< SVDFactor > m_rootFactors;

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
};

#endif // SVDFACTORTREE_H
