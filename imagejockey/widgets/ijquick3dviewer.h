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

	/** Renders the given polytopes with the given color expressed as RGB values. */
	void display( vtkPolyData* polyData, int r=255, int g=255, int b=255 );

	/** Renders the regular grid.  Its values are mapped to a grayscale set between given values. */
	void display( vtkImageData* imageData, double colorScaleMin, double colorScaleMax );

	/** Removes the actor(s) currently being displayed (if any). */
	void clearScene();

private:
	Ui::IJQuick3DViewer *ui;
	// the Qt widget containing a VTK viewport
	QVTKOpenGLWidget *_vtkwidget;

	// the VTK renderer (add VTK actors to it to build the scene).
	vtkSmartPointer<vtkRenderer> _renderer;

	// this must be class variable, otherwise a crash ensues due to smart pointer going
	// out of scope
	vtkSmartPointer<vtkOrientationMarkerWidget> _vtkAxesWidget;

	// List of pointers to the objects being viewed (if any).
	std::vector< vtkSmartPointer<vtkActor> > _currentActors;


private slots:
	void onDismiss();
};

#endif // IJQUICK3DVIEWER_H
