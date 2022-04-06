#ifndef WAVELETTRANSFORMDIALOG_H
#define WAVELETTRANSFORMDIALOG_H

#include "imagejockey/wavelet/waveletutils.h"
#include "spectral/spectral.h"
#include <vtkSmartPointer.h>

#include <QDialog>

namespace Ui {
class WaveletTransformDialog;
}

namespace QtCharts{
    class QChart;
    class QLineSeries;
    class QValueAxis;
}

class IJAbstractCartesianGrid;
class IJQuick3DViewer;
class QVTKOpenGLNativeWidget;
class vtkRenderer;
class vtkOrientationMarkerWidget;
class vtkPolyData;
class vtkImageData;
class vtkActor;
class vtkActor2D;
class vtkCubeAxesActor2D;

class WaveletTransformDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WaveletTransformDialog(IJAbstractCartesianGrid* inputGrid,
                                    uint inputVariableIndex,
                                    QWidget *parent = 0);
    ~WaveletTransformDialog();

Q_SIGNALS:
    /**
     * Signal emitted when the user wants to save a grid with the DWT result.
     * @param DWTtransform The coefficients.
     * @param scaleField The scale values (0 through log2(grid_size)).
     * @param orientationField The orientation values (1=N-S, 2=diagonals, 3=E-W).
     */
    void saveDWTTransform( const QString name,
                           const spectral::array& DWTtransform,
                           const spectral::array& scaleField,
                           const spectral::array& orientationField );
    /**
     * This unusual signal is emitted when this dialog wants some grid with the given name.
     * If there is a grid by the given name, its pointer is set to the passed pointer
     * reference, otherwise, the context must set it to nullptr.
     */
    void requestGrid( const QString name, IJAbstractCartesianGrid*& pointer );

private:
    Ui::WaveletTransformDialog *ui;
    IJAbstractCartesianGrid* m_inputGrid;
    uint m_inputVariableIndex;
    spectral::array m_DWTbuffer;
    WaveletFamily getSelectedWaveletFamily();
    static void debugGrid( const spectral::array &grid );

    ////////-----members used for 3D display-------------------
    // the Qt widget containing a VTK viewport
    QVTKOpenGLNativeWidget*                     _vtkwidget;
    // the VTK renderer (add VTK actors to it to build the scene).
    vtkSmartPointer<vtkRenderer>                _renderer;
    // this must be class variable, otherwise a crash ensues due to smart pointer going
    // out of scope
    vtkSmartPointer<vtkOrientationMarkerWidget> _vtkAxesWidget;
    // List of pointers to the objects being viewed (if any).
    std::vector< vtkSmartPointer<vtkActor> >    _currentActors;
    // Pointer to the scale bar actor
    vtkSmartPointer<vtkActor2D>                 _scaleBarActor;
    // Pointer to the axes scales actor
    vtkSmartPointer<vtkCubeAxesActor2D>         _axes;
    ///////////////////////////////////////////////////////////

    ////////-----members used for wavelet chart display-------------------
    QtCharts::QChart      *m_waveletChart;
    QtCharts::QLineSeries *m_chartSeriesWavelet;
    QtCharts::QLineSeries *m_chartSeriesScalet;
    ///////////////////////////////////////////////////////////

    void clearDisplay();


private Q_SLOTS:
    void onPerformTransform();
    void onWaveletFamilySelected( QString waveletFamilyName );
    void onSaveDWTResultAsGrid();
    void onReadDWTResultFromGrid();
    void onPreviewBacktransformedResult();
    void updateDisplay();
    void onUpdateWaveletDisplays();
    void onSaveBacktransformedResult();
};

#endif // WAVELETTRANSFORMDIALOG_H
