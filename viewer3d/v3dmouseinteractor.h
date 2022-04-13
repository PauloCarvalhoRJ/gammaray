#ifndef V3DMOUSEINTERACTOR_H
#define V3DMOUSEINTERACTOR_H

#include <vtkObjectFactory.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkMath.h>

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

    void setParentView3DWidget( View3DWidget* parentView3DWidget ){ m_ParentView3DWidget = parentView3DWidget; }
    View3DWidget* getParentView3DWidget( ){ return m_ParentView3DWidget; }

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
    vtkSmartPointer<vtkActor> m_cellPickMarkerActor;

    /** WARNING: this pointer can't be initialized in the constructor
     * because it is defined by the vtkStandardNewMacro macro.
     */
    View3DWidget* m_ParentView3DWidget;

    vtkSmartPointer<vtkMath> m_vtkMathObj;
};

#endif // V3DMOUSEINTERACTOR_H
