#include "svdfactortree.h"

SVDFactorTree::SVDFactorTree() : QAbstractItemModel()
{
	m_rootFactor = new SVDFactor();
}

SVDFactorTree::~SVDFactorTree()
{
	delete m_rootFactor;
}

void SVDFactorTree::addFirstLevelFactor(SVDFactor * factor)
{
	m_rootFactor->addChildFactor( factor );
}

bool SVDFactorTree::assignWeights(const std::vector<double> & weights)
{
	return m_rootFactor->assignWeights( weights );
}

//-------------- QAbstractItemModel interface------------
QModelIndex SVDFactorTree::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	SVDFactor *parentItem;

	if (!parent.isValid())
		parentItem = m_rootFactor;
	else
		parentItem = static_cast<SVDFactor*>(parent.internalPointer());

	SVDFactor *childItem = parentItem->getChildByIndex( row );
	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}
QModelIndex SVDFactorTree::parent(const QModelIndex &child) const
{
	if (!child.isValid())
		return QModelIndex();
	SVDFactor *childItem = static_cast<SVDFactor*>(child.internalPointer());
	SVDFactor *parentItem = childItem->getParent();
	if (parentItem == m_rootFactor)
		return QModelIndex();
	return createIndex(parentItem->getIndexInParent(), 0, parentItem);
}
int SVDFactorTree::rowCount(const QModelIndex &parent) const
{
	SVDFactor *parentItem;
	if (parent.column() > 0)
		return 0;
	if (!parent.isValid())
		parentItem = m_rootFactor;
	else
		parentItem = static_cast<SVDFactor*>(parent.internalPointer());
	return parentItem->getChildCount();
}
int SVDFactorTree::columnCount(const QModelIndex &/*parent*/) const
{
	return 1;
}
QVariant SVDFactorTree::data(const QModelIndex &index, int role) const
{
	 if (!index.isValid())
		 return QVariant();
	 SVDFactor *item = static_cast<SVDFactor*>(index.internalPointer());
	 if( role == Qt::DisplayRole )
		return QVariant( item->getPresentationName() );
	 if( role == Qt::DecorationRole )
		return QVariant( item->getIcon() );
	 if( role == Qt::CheckStateRole )
		 return static_cast<int>( item->isSelected() ? Qt::Checked : Qt::Unchecked );
	 return QVariant();
}
Qt::ItemFlags SVDFactorTree::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable ;
}
QVariant SVDFactorTree::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
	//if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	//    return rootItem->data(section);
	return QVariant();
}
bool SVDFactorTree::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if( role == Qt::EditRole )
		return false;
	if( role == Qt::CheckStateRole ){
		SVDFactor *item = static_cast<SVDFactor*>(index.internalPointer());
		item->setSelected( value.toBool() );
		emit dataChanged(index, index);
		return true;
	}
	return QAbstractItemModel::setData(index, value, role);
}
//-------------------------end of QAbstractItemModel interface------------------------------------
