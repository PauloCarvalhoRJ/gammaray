#ifndef V3DMOUSEINTERACTOR_H
#define V3DMOUSEINTERACTOR_H

#include <vtkObjectFactory.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkMath.h>
#include <memory>

class View3DWidget;

class v3dMouseInteractor : public vtkInteractorStyleTrackballCamera
{
public:
    static v3dMouseInteractor* New();
    vtkTypeMacro(v3dMouseInteractor, vtkInteractorStyleTrackballCamera)

    virtual void OnLeftButtonDown();

    virtual void OnLeftButtonUp();

    virtual void OnMouseMove();

    virtual void OnMouseWheelForward() override;

    virtual void OnMouseWheelBackward() override;

    void setParentView3DWidget( View3DWidget* parentView3DWidget );
    View3DWidget* getParentView3DWidget( ){ return m_ParentView3DWidget.get(); }

    void rescalePickMarkerActor();

    /** This was copied from vtkInteractorStyleTrackballCamera::Pan() and modified to correctly perform
     * panning with a vertical exaggeration applied to Z axis.
     * The only two changes are marked with comments in the .cpp file.
     */
    void Pan() override;

protected:

    bool m_isDragging;
    bool m_isLBdown;
    vtkSmartPointer<vtkActor> m_pickMarkerActor;

    /** WARNING: this pointer can't be initialized in the constructor
     * because it is defined by the vtkStandardNewMacro macro.
     *  TODO: this pointer is managed by Qt.  Check whether the use of a smart pointer
     *        to initialize it empty results in a possible double free (the smart pointer's and
     *        Qt's).
     */
    std::unique_ptr< View3DWidget > m_ParentView3DWidget;

    vtkSmartPointer<vtkMath> m_vtkMathObj;
};

#endif // V3DMOUSEINTERACTOR_H
