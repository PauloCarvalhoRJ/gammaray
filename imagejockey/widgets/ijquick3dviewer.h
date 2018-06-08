#ifndef IJQUICK3DVIEWER_H
#define IJQUICK3DVIEWER_H

#include <QWidget>
#include <vtkSmartPointer.h>

namespace Ui {
class IJQuick3DViewer;
}

class QVTKOpenGLWidget;
class vtkRenderer;
class vtkOrientationMarkerWidget;
class vtkPolyData;
class vtkImageData;
class vtkActor;

/**
 * The IJQuick3DViewer class allows quick inspection of 3D data (e.g. during debugging).
 */
class IJQuick3DViewer : public QWidget
{
    Q_OBJECT

public:
    /** @param show_not_set If true, adds a "NOT SET" item as the first item. */
    explicit IJQuick3DViewer( QWidget *parent = 0 );
    ~IJQuick3DViewer();

	void display(vtkPolyData* polyData);

	void display(vtkImageData* imageData, double colorScaleMin, double colorScaleMax);

private:
	Ui::IJQuick3DViewer *ui;
	// the Qt widget containing a VTK viewport
	QVTKOpenGLWidget *_vtkwidget;

	// the VTK renderer (add VTK actors to it to build the scene).
	vtkSmartPointer<vtkRenderer> _renderer;

	// this must be class variable, otherwise a crash ensues due to smart pointer going
	// out of scope
	vtkSmartPointer<vtkOrientationMarkerWidget> _vtkAxesWidget;

	// Points to the object being viewed (if any) so it is removed in the next call to display().
	vtkSmartPointer<vtkActor> _currentActor;

	/** Removes the actor currently being displayed (if any). */
	void clearScene();

private slots:
	void onDismiss();
	void onContinue();
};

#endif // IJQUICK3DVIEWER_H
