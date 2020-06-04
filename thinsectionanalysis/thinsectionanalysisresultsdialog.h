#ifndef THINSECTIONANALYSISRESULTSDIALOG_H
#define THINSECTIONANALYSISRESULTSDIALOG_H

#include <QDialog>

#include "thinsectionanalysis/thinsectionanalysiscluster.h"

namespace Ui {
class ThinSectionAnalysisResultsDialog;
}

class BarChartWidget;
class ImageViewerWidget;

class ThinSectionAnalysisResultsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ThinSectionAnalysisResultsDialog(QWidget *parent = nullptr);
    ~ThinSectionAnalysisResultsDialog();

    /** Sets the resulting clusters of pixels for this results dialog. */
    void setClusters(const std::vector<ThinSectionAnalysisClusterPtr> &clusters );

    /** Sets the path to the image painted with the clusters' colors. */
    void setOutputImage( const QString path );

private:
    Ui::ThinSectionAnalysisResultsDialog *ui;

    BarChartWidget* m_barChartWidget;

    ImageViewerWidget* m_imageViewerWidget;
};

#endif // THINSECTIONANALYSISRESULTSDIALOG_H
