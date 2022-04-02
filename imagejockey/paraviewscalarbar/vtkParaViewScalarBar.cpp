#include "vtkParaViewScalarBar.h"

#include <vtkCommand.h>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarWidget.h>

// Copied from ParaView
#include "vtkContext2DScalarBarActor.h"
#include "vtkPVScalarBarRepresentation.h"

vtkStandardNewMacro(vtkParaViewScalarBar)
//----------------------------------------------------------------------------
vtkParaViewScalarBar::vtkParaViewScalarBar() :
    Enabled(true),
    RepresentationObserverTag(0)
{
    this->ScalarBarActor->SetVisibility(1);
    SetTitle("Name");
    SetComponentTitle(""); //in the original: "Component"
    this->ScalarBarActor->SetTitleJustification(1); // centered
    this->ScalarBarActor->SetScalarBarThickness(16);
    // Like in ParaView, when using SetScalarBarLength call
    // ScalarBarLengthToScalarBarWidgetPosition2 to convert its value
    // to Position2 of the widget.
    this->ScalarBarActor->SetScalarBarLength(0.33);
    this->ScalarBarActor->SetAutomaticLabelFormat(1);
    this->ScalarBarActor->SetLabelFormat("%-#6.3g");
    this->ScalarBarActor->SetDrawNanAnnotation(0);
    this->ScalarBarActor->SetDrawAnnotations(1);
    this->ScalarBarActor->SetAddRangeAnnotations(0);
    this->ScalarBarActor->SetAutomaticAnnotations(0);
    this->ScalarBarActor->SetAddRangeLabels(1);
    this->ScalarBarActor->SetRangeLabelFormat("%-#6.1g"); //in the original: "%-#6.1e"
    this->ScalarBarActor->SetNanAnnotation("NaN");
    this->ScalarBarActor->SetDrawTickMarks(1);
    this->ScalarBarActor->SetDrawTickLabels(1);
    this->ScalarBarActor->SetUseCustomLabels(0);
    this->ScalarBarActor->SetTextPosition(1); // Ticks right/top, annotations left/bottom

    //this->ScalarBarActor->GetLabelTextProperty();
    //this->ScalarBarActor->GetTitleTextProperty();

    this->Representation->SetVisibility(1);
    double pos[2] = {0.89, 0.02};
    this->Representation->SetPosition(pos);
    this->Representation->SetOrientation(1); // vertical - 0: hortizontal
    this->Representation->SetWindowLocation(2); // LowerRightCorner
    this->Representation->SetScalarBarActor(ScalarBarActor.GetPointer());

    this->Widget->SetRepresentation(this->Representation.GetPointer());

    this->RepresentationObserverTag = this->Representation->AddObserver(
        vtkCommand::ModifiedEvent, this, &vtkParaViewScalarBar::OnRepresentationModified);

    // Initialize the scalar bar widget from the ScalarBarLength value.
    this->ScalarBarLengthToScalarBarWidgetPosition2();

    this->Widget->AddObserver(vtkCommand::InteractionEvent, this,
                              &vtkParaViewScalarBar::ExecuteEvent);

    SetEnabled(true); // will call this->UpdateEnabled();
}

//----------------------------------------------------------------------------
vtkParaViewScalarBar::~vtkParaViewScalarBar()
{
    //this->Representation->RemoveObserver(this->RepresentationObserverTag);
}

//----------------------------------------------------------------------------
void vtkParaViewScalarBar::SetRenderer(vtkRenderer* renderer)
{
    if (renderer == nullptr)
    {
        if (this->Renderer)
        {
            this->Widget->SetEnabled(0);
            this->Widget->SetDefaultRenderer(nullptr);
            this->Widget->SetCurrentRenderer(nullptr);
            this->Widget->SetInteractor(nullptr);

            vtkRenderer* renderer = this->Representation->GetRenderer();
            if (renderer)
            {
                renderer->RemoveActor(this->Representation.GetPointer());
                // NOTE: this will modify the Representation and call
                // this->OnRepresentationModified().
                this->Representation->SetRenderer(nullptr);
            }

            this->Renderer = nullptr;
        }
    }
    else
    {
        this->Renderer = renderer;

        // If DefaultRenderer is non-null, SetCurrentRenderer() will have no
        // effect.
        this->Widget->SetDefaultRenderer(nullptr);
        this->Widget->SetCurrentRenderer(renderer);
        // Set the DefaultRenderer to ensure that it doesn't get overridden by the
        // Widget. The Widget should use the specified renderer. Period.
        this->Widget->SetDefaultRenderer(renderer);

        this->Representation->SetRenderer(renderer);
        renderer->AddActor(this->Representation.GetPointer());

        this->UpdateEnabled();
    }
}

//----------------------------------------------------------------------------
void vtkParaViewScalarBar::SetInteractor(vtkRenderWindowInteractor* interactor)
{
    if (interactor && interactor != this->Widget->GetInteractor())
    {
        this->Interactor = interactor;
        this->UpdateEnabled();
    }
}

//-----------------------------------------------------------------------------
void vtkParaViewScalarBar::SetEnabled(bool enable)
{
    if (this->Enabled != enable)
    {
        this->Enabled = enable;
        this->UpdateEnabled();
    }
}

//-----------------------------------------------------------------------------
void vtkParaViewScalarBar::UpdateEnabled()
{
    bool enable_widget = (this->Renderer) ? this->Enabled : false;

    if (this->Representation->GetVisibility() == 0)
    {
        enable_widget = false;
    }

    // Not all processes have the interactor setup. Enable 3D widgets only on
    // those processes that have an interactor.
    if (this->Renderer == nullptr || this->Interactor == nullptr)
    {
        enable_widget = false;
    }

    if (bool(this->Widget->GetEnabled()) != enable_widget &&
            this->Interactor != nullptr)
    {
        this->Widget->SetInteractor(this->Interactor);
        this->Widget->SetEnabled(enable_widget ? 1 : 0);
    }
}

//----------------------------------------------------------------------------
void vtkParaViewScalarBar::OnRepresentationModified()
{
    // This will be called everytime the representation is modified, but since the
    // work done in this->UpdateEnabled() is minimal, we let it do it.
    this->UpdateEnabled();
}

//----------------------------------------------------------------------------
void vtkParaViewScalarBar::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
    os << indent << "Enabled: " << this->Enabled << endl;
}

//----------------------------------------------------------------------------
void vtkParaViewScalarBar::SetTitle(const std::string& title)
{
    this->ScalarBarActor->SetTitle(title.c_str());
}

//----------------------------------------------------------------------------
const std::string& vtkParaViewScalarBar::GetTitle()
{
    //this is necessary because this function returns a reference to an object
    //that must live while the program is running (be it static, global, member or dynamically allocated)
    static std::string local_non_temporary_string_object;
    local_non_temporary_string_object = (this->ScalarBarActor->GetTitle()) ? this->ScalarBarActor->GetTitle() : "";
    return local_non_temporary_string_object;
}

//----------------------------------------------------------------------------
void vtkParaViewScalarBar::SetComponentTitle(const std::string& title)
{
    this->ScalarBarActor->SetComponentTitle(title.c_str());
}

//----------------------------------------------------------------------------
const std::string& vtkParaViewScalarBar::GetComponentTitle()
{
    //this is necessary because this function returns a reference to an object
    //that must live while the program is running (be it static, global, member or dynamically allocated)
    static std::string local_non_temporary_string_object;
    local_non_temporary_string_object = (this->ScalarBarActor->GetComponentTitle()) ? this->ScalarBarActor->GetComponentTitle() : "";
    return local_non_temporary_string_object;
}

//----------------------------------------------------------------------------
void vtkParaViewScalarBar::SetLookupTable(vtkDiscretizableColorTransferFunction *lut)
{
    this->ScalarBarActor->SetLookupTable(lut);
}

//----------------------------------------------------------------------------
void vtkParaViewScalarBar::ExecuteEvent(vtkObject* pCaller,
                                        unsigned long eventId,
                                        void* pCallData)
{
    (void) pCaller;
    (void) pCallData;

    if (eventId == vtkCommand::InteractionEvent)
    {
        // If the widget's position is beyond the viewport, fix it.
        double position[2];
        position[0] = this->Representation->GetPosition()[0];
        position[1] = this->Representation->GetPosition()[1];
        if (position[0] < 0.0)
        {
            position[0] = 0.0;
        }
        if (position[0] > 0.97)
        {
            position[0] = 0.97;
        }
        if (position[1] < 0.0)
        {
            position[1] = 0.0;
        }
        if (position[1] > 0.97)
        {
            position[1] = 0.97;
        }
        this->Representation->SetPosition(position);
        this->Representation->SetWindowLocation(0);

        ScalarBarWidgetPosition2ToScalarBarLength();
    }
}

//----------------------------------------------------------------------------
void vtkParaViewScalarBar::ScalarBarWidgetPosition2ToScalarBarLength()
{
    int index = this->Representation->GetOrientation() == VTK_ORIENT_HORIZONTAL ? 0 : 1;
    double length = this->Representation->GetPosition2Coordinate()->GetValue()[index];
    this->ScalarBarActor->SetScalarBarLength(length);
}

//----------------------------------------------------------------------------
void vtkParaViewScalarBar::ScalarBarLengthToScalarBarWidgetPosition2()
{
    int index = this->Representation->GetOrientation() == VTK_ORIENT_HORIZONTAL ? 0 : 1;
    double length = this->ScalarBarActor->GetScalarBarLength();
    double pos2[3];
    this->Representation->GetPosition2Coordinate()->GetValue(pos2);
    pos2[index] = length;
    this->Representation->SetPosition2(pos2);
}
