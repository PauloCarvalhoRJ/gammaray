#ifndef IJQUICK3DVIEWER_H
#define IJQUICK3DVIEWER_H

#include <QWidget>
#include <vtkSmartPointer.h>
#include <thread>

namespace Ui {
class IJQuick3DViewer;
}

class QVTKOpenGLNativeWidget;
class vtkRenderer;
class vtkOrientationMarkerWidget;
class vtkPolyData;
class vtkImageData;
class vtkActor;
namespace spectral {
	class array;
}

/**
 * The IJQuick3DViewer class allows quick inspection of 3D data (e.g. during debugging).
 */
class IJQuick3DViewer : public QWidget
{
    Q_OBJECT

public:
    explicit IJQuick3DViewer( QWidget *parent = 0 );
    ~IJQuick3DViewer();

    /** Renders the given polytopes as lines/surfaces with the given color expressed as RGB values. */
	void display( vtkPolyData* polyData, int r=255, int g=255, int b=255 );

    /** Renders the given polytopes as a point cloud.
     * The points are rendered white if no scalars are provided to render with a color scale.
     */
    void display( vtkPolyData* polyData, float pointSize );

	/** Renders the passed regular grid.  Its values are mapped to a grayscale set between given values. */
	void display( vtkImageData* imageData, double colorScaleMin, double colorScaleMax );

	/** Renders the passed regular grid.  Its values are mapped to a grayscale set between given values. */
	void display( const spectral::array& grid, double colorScaleMin, double colorScaleMax );

    /** Renders the passed regular grid with cell sizes given.
     * Its values are mapped to a grayscale set between given values. */
    void display( const spectral::array& grid, double colorScaleMin, double colorScaleMax,
                  double cell_size_I, double cell_size_J, double cell_size_K );

    /** Renders the passed regular grid.  Its values are mapped to a grayscale set between min. and max. values. */
    void display( const spectral::array& grid );

	/** Removes the actor(s) currently being displayed (if any). */
	void clearScene();

    /** Hides the dismiss button.  Useful for when this widget is part of another widget. */
    void hideDismissButton();

private:
	Ui::IJQuick3DViewer *ui;
	// the Qt widget containing a VTK viewport
    QVTKOpenGLNativeWidget *_vtkwidget;

	// the VTK renderer (add VTK actors to it to build the scene).
	vtkSmartPointer<vtkRenderer> _renderer;

	// this must be class variable, otherwise a crash ensues due to smart pointer going
	// out of scope
	vtkSmartPointer<vtkOrientationMarkerWidget> _vtkAxesWidget;

	// List of pointers to the objects being viewed (if any).
	std::vector< vtkSmartPointer<vtkActor> > _currentActors;

	// The id of the thread that created the 3D viewer.
	// This prevents crashes when calling the display() methods from other threads.
	std::thread::id _ownerThreadId;

	/** Prevents OpenGL calls from different threads, which lead to crashes.
	 * This must be checked in all methods that result in rendering operations.
	 */
	bool threadCheck();

private slots:
	void onDismiss();
};

#endif // IJQUICK3DVIEWER_H
