#include "thinsectionanalysistablemodel.h"

#include <QBrush>

void ThinSectionAnalysisTableModel::setClusters(ThinSectionAnalysisClusterSetPtr clusterSet)
{
    m_clusterSet = clusterSet;
}

ThinSectionAnalysisTableModel::ThinSectionAnalysisTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

int ThinSectionAnalysisTableModel::rowCount(const QModelIndex &parent) const
{
    if( m_clusterSet )
        return m_clusterSet->getClusterCount();
    return 0;
}

int ThinSectionAnalysisTableModel::columnCount(const QModelIndex &parent) const
{
    //Columns are fixed: cluster color, cluster name, cluster count/proportion.
    return 3;
}

QVariant ThinSectionAnalysisTableModel::data(const QModelIndex &index, int role) const
{
    if( ! m_clusterSet || m_clusterSet->isEmpty() )
        return QVariant();

    if (!index.isValid() || ( role != Qt::DisplayRole && role != Qt::BackgroundRole ) ) {
        return QVariant();
    }

    if (index.column() == 0) {
        if( role == Qt::BackgroundRole)
            return QBrush( m_clusterSet->getCluster( index.row() )->getColor() );
        else
            return "";
    } else if (index.column() == 1) {
        if( role == Qt::DisplayRole )
            return m_clusterSet->getCluster( index.row() )->getName();
        else
            return QVariant();
    } else if (index.column() == 2) {
        if( role == Qt::DisplayRole )
            return m_clusterSet->getCluster( index.row() )->getProportion() * 100.0;
        else
            return QVariant();
    }
    return QVariant();
}

QVariant ThinSectionAnalysisTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section == 0) {
            return QString("Color");
        } else if (section == 1) {
            return QString("Name");
        } else if (section == 2) {
            return QString("Proportion (%)");
        }
    }
    return QVariant();
}
