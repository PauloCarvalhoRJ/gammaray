#ifndef GABORFILTERDIALOG_H
#define GABORFILTERDIALOG_H

#include <QDialog>
#include "spectral/spectral.h"
#include <vtkSmartPointer.h>

namespace Ui {
class GaborFilterDialog;
}

class IJAbstractCartesianGrid;
class QVTKOpenGLWidget;
class vtkRenderer;
class vtkOrientationMarkerWidget;
class vtkPolyData;
class vtkImageData;
class vtkActor;
class vtkActor2D;

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

Q_SIGNALS:
    /** Client code can use this to be notified when the spectrogram is read.
     * Retrieve results with getSpectrogram().
     */
    void spectrogramGenerated();

private:
    Ui::GaborFilterDialog *ui;
    IJAbstractCartesianGrid* m_inputGrid;
    uint m_inputVariableIndex;
    spectral::arrayPtr m_spectrogram;

    ////////-----members used for 3D display-------------------
    // the Qt widget containing a VTK viewport
    QVTKOpenGLWidget *_vtkwidget;
    // the VTK renderer (add VTK actors to it to build the scene).
    vtkSmartPointer<vtkRenderer> _renderer;
    // this must be class variable, otherwise a crash ensues due to smart pointer going
    // out of scope
    vtkSmartPointer<vtkOrientationMarkerWidget> _vtkAxesWidget;
    // List of pointers to the objects being viewed (if any).
    std::vector< vtkSmartPointer<vtkActor> > _currentActors;
    // Pointer to the scale bar actor
    vtkSmartPointer<vtkActor2D> _scaleBarActor;
    ///////////////////////////////////////////////////////////

    void updateDisplay();
    void clearDisplay();

private Q_SLOTS:
    void onPerformGaborFilter();
};

#endif // GABORFILTERDIALOG_H
