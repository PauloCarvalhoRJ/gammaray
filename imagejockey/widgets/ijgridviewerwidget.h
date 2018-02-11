#ifndef IJGRIDVIEWERWIDGET_H
#define IJGRIDVIEWERWIDGET_H

#include <QWidget>

class SVDFactor;

namespace Ui {
	class IJGridViewerWidget;
}

namespace spectral{
    struct array;
}

class ImageJockeyGridPlot;

/** A widget to view gridded data using 2D slices.  This widget is a lightweight alternative to
 * the full 3D viewer in viewer3d subdirectory.
 * Actually it only displays SVDFactors, so you need to create an SVDFactor object from
 * your data.
 */
class IJGridViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IJGridViewerWidget(QWidget *parent = 0);
    ~IJGridViewerWidget();
    void setFactor(SVDFactor* factor );

private:
    Ui::IJGridViewerWidget *ui;
	ImageJockeyGridPlot* m_gridPlot;
    SVDFactor* m_factor;
    void forcePlotUpdate();
    void adjustColorTableWidgets( int cmbIndex );

private slots:
    void onCmbColorScaleValueChanged( int index );
    void onCmbPlaneChanged( int index );
    void onSpinSliceChanged( int value );
};

#endif // IJGRIDVIEWERWIDGET_H
