#ifndef GABORFILTERDIALOG_H
#define GABORFILTERDIALOG_H

#include <QDialog>
#include "spectral/spectral.h"
#include <vtkSmartPointer.h>
#include "imagejockey/gabor/gaborscandialog.h"

namespace Ui {
class GaborFilterDialog;
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
class vtkParaViewScalarBar;

class GaborFilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GaborFilterDialog(IJAbstractCartesianGrid* inputGrid,
                               uint inputVariableIndex,
                               QWidget *parent = nullptr);
    ~GaborFilterDialog();

    /** Returns the spectrogram cube generated in the last computation.
     * It returns an empty pointer if the user didn't perform any calculation.
     */
    spectral::arrayPtr getSpectrogram();

private:
    Ui::GaborFilterDialog *ui;
    IJAbstractCartesianGrid* m_inputGrid;
    uint m_inputVariableIndex;
    spectral::arrayPtr m_spectrogram;
    IJQuick3DViewer* m_kernelViewer1;
    IJQuick3DViewer* m_kernelViewer2;
    GaborScanDialog* m_gsd;
    GaborFrequencyAzimuthSelections m_freqAzSelections;
    spectral::array m_filteredResult;

    ////////-----members used for 3D display-------------------
    // the Qt widget containing a VTK viewport
    QVTKOpenGLNativeWidget *_vtkwidget;
    // the VTK renderer (add VTK actors to it to build the scene).
    vtkSmartPointer<vtkRenderer> _renderer;
    // this must be class variable, otherwise a crash ensues due to smart pointer going
    // out of scope
    vtkSmartPointer<vtkOrientationMarkerWidget> _vtkAxesWidget;
    // List of pointers to the objects being viewed (if any).
    std::vector< vtkSmartPointer<vtkActor> > _currentActors;
    // Pointer to the axes scales actor
    vtkSmartPointer<vtkCubeAxesActor2D> _axes;
    // Client-owned pointer to the custom scalar bar 2D widget.
    vtkNew<vtkParaViewScalarBar> _scalarBar;
    ///////////////////////////////////////////////////////////

    void clearDisplay();
    void performFiltering();

    /** Call this to inspect results in grid objects. */
    void debugGrid( const spectral::array& grid );

private Q_SLOTS:
    void onPerformGaborFilter();
    void updateDisplay();
    void onScan();
    void updateKernelDisplays();
    void onFreqAzSelectionsUpdated( const GaborFrequencyAzimuthSelections& freqAzSelections );
    void onUserEditedAFrequency( QString freqValue );
    void onPreviewFilteredResult();
    void onSaveFilteredResult();
};

#endif // GABORFILTERDIALOG_H
