#ifndef THINSECTIONANALYSISRESULTSDIALOG_H
#define THINSECTIONANALYSISRESULTSDIALOG_H

#include <QDialog>

#include "thinsectionanalysis/thinsectionanalysisclusterset.h"

namespace Ui {
class ThinSectionAnalysisResultsDialog;
}

class BarChartWidget;
class ImageViewerWidget;
class ThinSectionAnalysisTableModel;
class QItemSelection;

class ThinSectionAnalysisResultsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ThinSectionAnalysisResultsDialog(QWidget *parent = nullptr);
    ~ThinSectionAnalysisResultsDialog();

    /** Sets the resulting clusters of pixels for this results dialog. */
    void setClusters( ThinSectionAnalysisClusterSetPtr clusterSet );

    /** Sets the path to the image painted with the clusters' colors. */
    void setOutputImage( const QString path );

private:
    Ui::ThinSectionAnalysisResultsDialog *ui;

    /** The bar chart view to the clusters. */
    BarChartWidget* m_barChartWidget;

    /** The image view to the clustres. */
    ImageViewerWidget* m_imageViewerWidget;

    /** The container of clusters. */
    ThinSectionAnalysisClusterSetPtr m_clusterSet;

    /** The table view to the clusters. */
    ThinSectionAnalysisTableModel* m_tableModel;

    /** The list of selected clusters.  This is repopulated
     * in slots such as onSelectionChangedInTable().
     */
    std::vector<int> m_selectedClusterIndexes;

    /** Updates bar chart view to reflect the current cluster stats. */
    void updateBarChart();

    /** Reconfigure the UI to enable/disable certain features (e.g. the button
     * to join clusters is only enabled if the user selects two or more clusters.
     */
    void reconfigureUI();

private Q_SLOTS:

    /** Called when the user changes cluster data such as names and colors. */
    void onDataEdited();

    /** Called when the user wants to join the currently selected clusters. */
    void onJoinCategories();

    /** Called when the user makes selections on the table view. */
    void onSelectionChangedInTable(const QItemSelection&, const QItemSelection&);

};

#endif // THINSECTIONANALYSISRESULTSDIALOG_H
