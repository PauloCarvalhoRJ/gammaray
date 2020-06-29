#ifndef THINSECTIONANALYSISTABLEMODEL_H
#define THINSECTIONANALYSISTABLEMODEL_H

#include "thinsectionanalysis/thinsectionanalysisclusterset.h"

#include <QAbstractTableModel>

/** This is a model class used to present cluster data in the QTabelView object present in
 * ThinSectionAnalysisResultsDialog.
 *
 * Good source on working with the model/view design in Qt:
 * https://doc.qt.io/qt-5/modelview.html - Model/View Tutorial
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
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

Q_SIGNALS:
    void dataEdited();

private:
    ThinSectionAnalysisClusterSetPtr m_clusterSet;

};

#endif // THINSECTIONANALYSISTABLEMODEL_H
