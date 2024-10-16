#include "v3dmouseinteractor.h"
#include "domain/application.h"

#include <vtkRenderWindowInteractor.h>
#include <vtkPropPicker.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkCellData.h>

#include <sstream>

#include "viewer3d/view3dwidget.h"

// Implementation of the New() function for this class.
vtkStandardNewMacro(v3dMouseInteractor);

void v3dMouseInteractor::OnLeftButtonDown()
{
    m_isLBdown = true;

    // Forward the event to the superclass.
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}

void v3dMouseInteractor::OnLeftButtonUp()
{
    // Don't pick during dragging.
    if( ! m_isDragging ){

        //Remove the marker actor anyway.
        if( m_pickMarkerActor ) {
            this->GetDefaultRenderer()->RemoveActor( m_pickMarkerActor );
            m_pickMarkerActor = nullptr;
        }

        // Get pick position in 2D screen coordinates.
        int* clickPos = this->GetInteractor()->GetEventPosition();

        m_ParentView3DWidget->getForegroundRenderer()->SetPreserveDepthBuffer( true );
        m_ParentView3DWidget->getRenderer()->Render();

        // Get the 3D object under the 2D screen coordinates (ray casting).
        vtkSmartPointer<vtkPropPicker>  picker = vtkSmartPointer<vtkPropPicker>::New();
        picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

        // if a vtkActor or subclass was picked
        if( picker->GetActor() ) {

            // Get the cell of a 3D object under the 2D screen coordinates (ray casting).
            vtkSmartPointer<vtkCellPicker>  cellPicker = vtkSmartPointer<vtkCellPicker>::New();
            cellPicker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

            //if an actor's cell was picked
            if( cellPicker->GetCellId() != -1 ) {

                // Get the picked location in world coordinates.
                double* pos = picker->GetPickPosition();

                // Print which actor has been picked.
                std::stringstream str;
                picker->GetActor()->Print( str );
                Application::instance()->logInfo(  "Picked scene object info:\n" + QString( str.str().c_str() ) );
                Application::instance()->logInfo(  "----- end of scene object info ------" );

                // Draw/update the pick marker (small red sphere).
                {
                    // Create a small sphere to mark the picked location on the scene.
                    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
                    //sphereSource->SetCenter(pos[0], pos[1], pos[2]); //center is set via vtkActor::SetPosition() further below.
                    sphereSource->SetRadius(100.0);

                    // Create a mapper for the pick marker.
                    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                    mapper->SetInputConnection(sphereSource->GetOutputPort());

                    // (Re)create an actor for the pick marker.
                    m_pickMarkerActor = vtkSmartPointer<vtkActor>::New();
                    m_pickMarkerActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
                    m_pickMarkerActor->SetPosition( pos );
                    m_pickMarkerActor->SetMapper(mapper);

                    // Adds the pick marker to the scene.
                    this->GetDefaultRenderer()->AddActor( m_pickMarkerActor );

                    // Rescale the actor so it stays with a constant size with respect to the canvas
                    // independently of zoom.
                    rescalePickMarkerActor();
                }

                // Output the picked location.
                Application::instance()->logInfo( "Picked location (world coordinates): X="
                                                  + QString::number(pos[0]) + " Y=" + QString::number(pos[1])
                        + " Z=" + QString::number(pos[2]) );

                // Output the probed value at the picked location.
                vtkDataSet* dataSet = cellPicker->GetDataSet();
                vtkCellData* cellData = dataSet->GetCellData();
                vtkDataArray* dataArray = cellData->GetScalars();
                if( dataArray && dataArray->GetNumberOfComponents() <= 200 ){
                    double values[200]; //200 fields is a fairly large number.
                    dataArray->GetTuple( cellPicker->GetCellId(), values );
                    QString valuesText = "NO VALUES";
                    for( int i = 0; i < dataArray->GetNumberOfComponents(); ++i )
                        if( i == 0)
                            valuesText = QString::number(values[0]);
                        else
                            valuesText = valuesText + "; " + QString::number(values[i]);
                    Application::instance()->logInfo( "Picked cell id: " + QString::number( cellPicker->GetCellId() ) );
                    Application::instance()->logInfo( "Picked vtkDataSet pointer: " + QString::number( (long long)dataSet ) );
                    Application::instance()->logInfo( "Picked value(s): " + valuesText);
                    if( m_ParentView3DWidget )
                        m_ParentView3DWidget->probeFurther( pos[0], pos[1], pos[2], dataSet, cellPicker->GetCellId() );
                    else
                        Application::instance()->logWarn( "v3dMouseInteractor::OnLeftButtonUp(): a parent View3DWidget was not passed to this v3dMouseInteractor.  Probing info is limited." );
                } else
                    Application::instance()->logWarn( "v3dMouseInteractor::OnLeftButtonUp(): probing not possible if VTK object has no fields or more than 200 fields in it." );
            }// if a cell was picked
        } //if a vtkActor was picked
        else{ //something else was picked
            Application::instance()->logError( "v3dMouseInteractor::OnLeftButtonUp(): an unsupported scene element was picked." );
        }
    } //if user is not dragging the mouse

    m_isLBdown = false;
    m_isDragging = false;

    m_ParentView3DWidget->getForegroundRenderer()->SetPreserveDepthBuffer( false );

    // Forward the event to the superclass.
    vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
}

void v3dMouseInteractor::OnMouseMove()
{
    if( m_isLBdown )
        m_isDragging = true;

    // Forward the event to the superclass.
    vtkInteractorStyleTrackballCamera::OnMouseMove();
}

void v3dMouseInteractor::OnMouseWheelForward()
{
    // Forward the event to the superclass.
    vtkInteractorStyleTrackballCamera::OnMouseWheelForward();

    rescalePickMarkerActor();
}

void v3dMouseInteractor::OnMouseWheelBackward()
{
    // Forward the event to the superclass.
    vtkInteractorStyleTrackballCamera::OnMouseWheelBackward();

    rescalePickMarkerActor();
}

void v3dMouseInteractor::setParentView3DWidget(View3DWidget *parentView3DWidget)
{
    m_ParentView3DWidget = std::unique_ptr<View3DWidget>( parentView3DWidget );
}

void v3dMouseInteractor::rescalePickMarkerActor()
{
    double cameraFocalPoint[3], cameraPosition[3];
    double objectPosition[3];
    double newScale = 0;
    double totalSquared = 0;

    if( ! m_vtkMathObj )
        m_vtkMathObj = vtkSmartPointer<vtkMath>::New();

    // Keeping the pick marker actor
    // the same size relative to the viewport. This is done by
    // changing the scale of the object to be proportional to
    // the distance between the current camera and the object.
    if( m_pickMarkerActor ){
        // Get a pointer to the current camera according to the main renderer.
        vtkCamera *currentCamera = static_cast<vtkCamera *>( this->GetDefaultRenderer()->GetActiveCamera() );
        currentCamera->GetPosition( cameraPosition );
        currentCamera->GetFocalPoint( cameraFocalPoint );
        m_pickMarkerActor->GetPosition( objectPosition );

        totalSquared = std::sqrt(m_vtkMathObj->Distance2BetweenPoints( objectPosition, cameraPosition ));
        newScale = totalSquared / 20000;    // The denominator value is an abitary number that gives the desired size of the objects on screen.
        Application::instance()->logInfo( "Distance to picked location: " + QString::number(totalSquared) );
        m_pickMarkerActor->SetScale( newScale,
                                     newScale,
                                     newScale / m_ParentView3DWidget->getVerticalExaggeration() );

        this->GetDefaultRenderer()->Render();
    }
}

void v3dMouseInteractor::Pan()
{
    if (this->CurrentRenderer == nullptr)
    {
      return;
    }

    vtkRenderWindowInteractor* rwi = this->Interactor;

    double viewFocus[4], focalDepth, viewPoint[3];
    double newPickPoint[4], oldPickPoint[4], motionVector[3];

    // Calculate the focal depth since we'll be using it a lot

    vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
    camera->GetFocalPoint(viewFocus);
    this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2], viewFocus);
    focalDepth = viewFocus[2];

    this->ComputeDisplayToWorld(
      rwi->GetEventPosition()[0], rwi->GetEventPosition()[1], focalDepth, newPickPoint);

    // Has to recalc old mouse point since the viewport has moved,
    // so can't move it outside the loop

    this->ComputeDisplayToWorld(
      rwi->GetLastEventPosition()[0], rwi->GetLastEventPosition()[1], focalDepth, oldPickPoint);

    // Camera motion is reversed

    motionVector[0] = oldPickPoint[0] - newPickPoint[0];
    motionVector[1] = oldPickPoint[1] - newPickPoint[1];
    motionVector[2] = oldPickPoint[2] - newPickPoint[2];

    camera->GetFocalPoint(viewFocus);
    camera->GetPosition(viewPoint);

    /***** All code above this point was copied from vtkInteractorStyleTrackballCamera::Pan(). ****/

    //Changed from the original Pan() implementation in vtkInteractorStyleTrackballCamera
    //to account for vertical exaggeration along Z axis
    camera->SetFocalPoint(
      motionVector[0] + viewFocus[0], motionVector[1] + viewFocus[1],
            m_ParentView3DWidget->getVerticalExaggeration() * motionVector[2] + viewFocus[2]);

    //Changed from the original Pan() implementation in vtkInteractorStyleTrackballCamera
    //to account for vertical exaggeration along Z axis
    camera->SetPosition(
      motionVector[0] + viewPoint[0], motionVector[1] + viewPoint[1],
            m_ParentView3DWidget->getVerticalExaggeration() * motionVector[2] + viewPoint[2]);

    /***** All code balow this point was copied from vtkInteractorStyleTrackballCamera::Pan(). ****/

    if (rwi->GetLightFollowCamera())
    {
      this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }

    rwi->Render();
}
