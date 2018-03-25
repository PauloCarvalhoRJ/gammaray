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
    explicit IJGridViewerWidget( bool deleteFactorOnClose, QWidget *parent = 0);
    ~IJGridViewerWidget();
    void setFactor(SVDFactor* factor );

private:
    Ui::IJGridViewerWidget *ui;
	ImageJockeyGridPlot* m_gridPlot;
    SVDFactor* m_factor;
    bool m_deleteFactorOnClose;
    static QString m_lastOpenedPath;
    void forcePlotUpdate();
    void adjustColorTableWidgets( int cmbIndex );

private slots:
    void onCmbColorScaleValueChanged( int index );
    void onCmbPlaneChanged( int index );
    void onSpinSliceChanged( int value );
    void onDismiss();
    /** Saves the currently viewed grid slice as a 2D grayscale image file in PNG format.
     * The values are re-scaled to 0-255 gray level interval.  Null values are saved as transparent
     * pixels.
     */
	void onExportSliceAsPNG();
    /** Replaces the data of the currently viewed grid slice with data from a 2D grayscale image
     * file in PNG format. The 0-255 values are re-scaled to the global min-max of current grid.
     * Transparent pixels are imported as uninformed values.  Incompatible images or images containing
     * non-gray pixels result in error.
     */
    void onImportSliceDataFromPNG();
};

#endif // IJGRIDVIEWERWIDGET_H
