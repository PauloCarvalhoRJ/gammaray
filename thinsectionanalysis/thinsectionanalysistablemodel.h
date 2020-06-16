#ifndef THINSECTIONANALYSISTABLEMODEL_H
#define THINSECTIONANALYSISTABLEMODEL_H

#include "thinsectionanalysis/thinsectionanalysisclusterset.h"

#include <QAbstractTableModel>

/** This is a model class used to present cluster data in the QTabelView object present in
 * ThinSectionAnalysisResultsDialog.
 */
class ThinSectionAnalysisTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ThinSectionAnalysisTableModel(QObject *parent = nullptr);

    /** Sets the resulting clusters of pixels as data for this table model. */
    void setClusters(ThinSectionAnalysisClusterSetPtr clusterSet );

    // QAbstractItemModel interface
public:
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    ThinSectionAnalysisClusterSetPtr m_clusterSet;

};

#endif // THINSECTIONANALYSISTABLEMODEL_H
