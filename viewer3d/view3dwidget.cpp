//----------Since we're not building with CMake, we need to init the VTK
// modules------------------
//--------------linking with the VTK libraries is often not
// enough--------------------------------
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2) // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle)
VTK_MODULE_INIT(vtkRenderingFreeType)
//------------------------------------------------------------------------------------------------

#include "ui_view3dwidget.h"
#include "view3dwidget.h"
#include <QVTKOpenGLWidget.h>

#include <QSettings>
#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkFXAAOptions.h>
#include <vtkRendererCollection.h>
#include <vtkCallbackCommand.h>
#include <vtkBillboardTextActor3D.h>
#include <vtkTextProperty.h>

#include "domain/application.h"
#include "domain/project.h"
#include "domain/projectcomponent.h"
#include "view3dbuilders.h"
#include "view3dconfigwidget.h"
#include "view3dverticalexaggerationwidget.h"
#include "viewer3d/v3dmouseinteractor.h"
#include "viewer3d/view3dtextconfigwidget.h"
#include "util.h"

View3DWidget::View3DWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::View3DWidget), _currentCfgWidget(nullptr),
      _verticalExaggWiget(nullptr),
      _textConfigWiget(nullptr)
{
    ui->setupUi(this);

    // restore main splitter position
    {
        QSettings qs;
        QByteArray state;
        if (qs.contains("viewer3dsplitter"))
            state = qs.value("viewer3dsplitter").toByteArray();
        ui->splitter->restoreState(state);
    }
    // restore left splitter position
    {
        QSettings qs;
        QByteArray state;
        if (qs.contains("viewer3dsplitter2"))
            state = qs.value("viewer3dsplitter2").toByteArray();
        ui->splitter_2->restoreState(state);
    }

    // MSAA aliasing (these must be BEFORE creating a QVTKOpenGLWidget.
    if( Util::programWasCalledWithCommandLineArgument("-aa=MSAA") ){
        vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples ( 8 );
        QSurfaceFormat::setDefaultFormat ( QVTKOpenGLWidget::defaultFormat() );
    }

    _vtkwidget = new QVTKOpenGLWidget();

    //===========VTK TEST CODE==========================================
    //    vtkSmartPointer<vtkSphereSource> sphereSource =
    //        vtkSmartPointer<vtkSphereSource>::New();
    //    vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
    //        vtkSmartPointer<vtkPolyDataMapper>::New();
    //    sphereMapper->SetInputConnection( sphereSource->GetOutputPort() );
    //    vtkSmartPointer<vtkActor> sphereActor =
    //        vtkSmartPointer<vtkActor>::New();
    //    sphereActor->SetMapper( sphereMapper );
    //==================================================================

    _rendererMainScene = vtkSmartPointer<vtkRenderer>::New();

    // add a nice sky-like background
    _rendererMainScene->GradientBackgroundOn();
    _rendererMainScene->SetBackground(0.9, 0.9, 1);
    _rendererMainScene->SetBackground2(0.5, 0.5, 1);
    _rendererMainScene->SetLayer( 0 ); //only the renderer of layer 0 have background.

    if( ! ( Util::programWasCalledWithCommandLineArgument("-aa=none") ||
            Util::programWasCalledWithCommandLineArgument("-aa=MSAA") ) ) {
        // enable antialiasing (fast approximate method)
        _rendererMainScene->UseFXAAOn();

        // configure the FXAA antialiasing
        vtkSmartPointer<vtkFXAAOptions> fxaaOptions = _rendererMainScene->GetFXAAOptions();
        fxaaOptions->SetSubpixelBlendLimit( 1/2.0 );
        //fxaaOptions->SetSubpixelContrastThreshold(1/2.0);
        //fxaaOptions->SetRelativeContrastThreshold(0.125);
        //fxaaOptions->SetHardContrastThreshold(0.045);
        //fxaaOptions->SetSubpixelBlendLimit(0.75);
        //fxaaOptions->SetSubpixelContrastThreshold(0.25);
        //fxaaOptions->SetUseHighQualityEndpoints(true);
        //fxaaOptions->SetEndpointSearchIterations(12);
    }

    //    renderer->AddActor( sphereActor );  // VTK TEST CODE
    //    vtkRenderWindow* renwin = vtkRenderWindow::New();
    //	vtkGenericOpenGLRenderWindow* glrw =
    // vtkGenericOpenGLRenderWindow::SafeDownCast(renwin);
    //	_vtkwidget->SetRenderWindow( glrw );

    // Create the renderer for always-on-top objects
    // It attaches to the same camera of the main renderer
    _rendererForeground = vtkSmartPointer<vtkRenderer>::New();
    _rendererForeground->SetActiveCamera( _rendererMainScene->GetActiveCamera() );
    _rendererForeground->SetLayer( 1 ); //layers greater than zero have no background and are rendered last.

    _vtkwidget->SetRenderWindow(vtkGenericOpenGLRenderWindow::New());
    _vtkwidget->GetRenderWindow()->AddRenderer(_rendererMainScene);
    _vtkwidget->GetRenderWindow()->AddRenderer(_rendererForeground);
    _vtkwidget->setFocusPolicy(Qt::StrongFocus);

    //MSAA aliasing
    if( Util::programWasCalledWithCommandLineArgument("-aa=MSAA")){
        _vtkwidget->GetRenderWindow()->SetMultiSamples( 4 );
    }

    //----------------------adding the orientation axes-------------------------
    vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
    _vtkAxesWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    _vtkAxesWidget->SetOutlineColor(0.9300, 0.5700, 0.1300);
    _vtkAxesWidget->SetOrientationMarker(axes);
    _vtkAxesWidget->SetInteractor(_vtkwidget->GetRenderWindow()->GetInteractor());
    _vtkAxesWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
    _vtkAxesWidget->SetEnabled(1);
    _vtkAxesWidget->InteractiveOn();
    //--------------------------------------------------------------------------

    // Customize event handling through a subclass of vtkInteractorStyleTrackballCamera.
    // This allows picking and probing by clicking on objects in the scene, for example.
    m_myInteractor = vtkSmartPointer<v3dMouseInteractor>::New();
    m_myInteractor->setParentView3DWidget( this );
    m_myInteractor->SetDefaultRenderer(_rendererMainScene);
    _vtkwidget->GetRenderWindow()->GetInteractor()->SetInteractorStyle( m_myInteractor );

    // Set callback for any event
    vtkSmartPointer<vtkCallbackCommand> callBackCommand = vtkSmartPointer<vtkCallbackCommand>::New();
    callBackCommand->SetCallback( rendererCallback );
    callBackCommand->SetClientData((void*)this);
    _rendererMainScene->AddObserver( vtkCommand::AnyEvent , callBackCommand );   // mp_ren is the vtkRenderer object.

    // Prepare to render transparency/translucency adequately
    // See: https://stackoverflow.com/questions/47528086/problems-with-rendering-transparent-objects-in-vtk
    //      https://vtk.org/Wiki/VTK/Examples/Cxx/Visualization/CorrectlyRenderTranslucentGeometry
    _rendererMainScene->SetUseDepthPeeling(1);
    _rendererMainScene->SetOcclusionRatio(0.1);
    _rendererMainScene->SetMaximumNumberOfPeels(4);
    _vtkwidget->GetRenderWindow()->SetMultiSamples(0);
    _vtkwidget->GetRenderWindow()->SetAlphaBitPlanes(1);

    // adjusts view so everything fits in the screen
    _rendererMainScene->ResetCamera();

    // add the VTK widget the layout
    ui->frmViewer->layout()->addWidget(_vtkwidget);

    // enable and configure the objects list's drag-and-drop feature.
    // ui->listWidget->setDragEnabled(true);
    // ui->listWidget->setDragDropMode(QAbstractItemView::DragDrop);
    // ui->listWidget->viewport()->setAcceptDrops(true);
    // ui->listWidget->setDropIndicatorShown(true);
    ui->listWidget->setAcceptDrops(true);

    connect(ui->listWidget, SIGNAL(newObject(View3DListRecord)), this,
            SLOT(onNewObject(View3DListRecord)));
    connect(ui->listWidget, SIGNAL(removeObject(View3DListRecord)), this,
            SLOT(onRemoveObject(View3DListRecord)));

    // Creates, but doesn't show, the floating widget to set the vertical exaggeration.
    _verticalExaggWiget = new View3DVerticalExaggerationWidget(this);
    _verticalExaggWiget->hide();
    //_verticalExaggWiget->setWindowFlags( Qt::CustomizeWindowHint );
    //_verticalExaggWiget->setWindowFlags( Qt::FramelessWindowHint );
    connect(_verticalExaggWiget, SIGNAL(valueChanged(double)), this,
            SLOT(onVerticalExaggerationChanged(double)));

    // Creates, but doesn't show, the floating widget to set the text configuration.
    _textConfigWiget = new View3DTextConfigWidget( "inView3DWidget", this);
    _textConfigWiget->hide();
    //_verticalExaggWiget->setWindowFlags( Qt::CustomizeWindowHint );
    //_verticalExaggWiget->setWindowFlags( Qt::FramelessWindowHint );
    connect(_textConfigWiget, SIGNAL(change()), this,
            SLOT(onTextConfigChanged()));

    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btnGlobal->setIconSize( QSize( 64, 64 ) );
        ui->btnGlobal->setIcon( QIcon(":icons32/v3Dglobal32") );
        ui->btnLookAtXY->setIconSize( QSize( 64, 64 ) );
        ui->btnLookAtXY->setIcon( QIcon(":icons32/v3Dxy32") );
        ui->btnLookAtXZ->setIconSize( QSize( 64, 64 ) );
        ui->btnLookAtXZ->setIcon( QIcon(":icons32/v3Dxz32") );
        ui->btnLookAtYZ->setIconSize( QSize( 64, 64 ) );
        ui->btnLookAtYZ->setIcon( QIcon(":icons32/v3Dyz32") );
        ui->btnVerticalExaggeration->setIconSize( QSize( 64, 64 ) );
        ui->btnVerticalExaggeration->setIcon( QIcon(":icons32/vertexag32") );
        ui->btnFont->setIconSize( QSize( 64, 64 ) );
    }
}

View3DWidget::~View3DWidget()
{
    QSettings qs;
    qs.setValue("viewer3dsplitter", ui->splitter->saveState());
    qs.setValue("viewer3dsplitter2", ui->splitter_2->saveState());
    delete ui;
}

double View3DWidget::getVerticalExaggeration() const
{
    return _verticalExaggWiget->getVerticalExaggeration();
}

void View3DWidget::removeCurrentConfigWidget()
{
    // if there is a current config widget
    if (_currentCfgWidget) {
        // removes the current config widget
        ui->frmDataViewOptions->layout()->removeWidget(_currentCfgWidget);
        // resets its parent
        _currentCfgWidget->setParent(nullptr);
        // resets the pointer
        _currentCfgWidget = nullptr;
    }
}

void View3DWidget::applyCurrentTextStyle(vtkSmartPointer<vtkBillboardTextActor3D> textActor)
{
    textActor->GetTextProperty()->SetFontSize ( _textConfigWiget->getFontSize() );
    QColor fontColor = _textConfigWiget->getFontColor();
    textActor->GetTextProperty()->SetColor ( fontColor.redF(),
                                             fontColor.greenF(),
                                             fontColor.blueF() );
    textActor->GetTextProperty()->SetJustificationToCentered();
    textActor->SetVisibility( _textConfigWiget->isShowText() );
}

/*static*/ void View3DWidget::rendererCallback(vtkObject *caller,
                                                 unsigned long vtkNotUsed(QWidget::event),
                                                 void *arg,
                                                 void *vtkNotUsed(whatIsThis))
{
    QVTKOpenGLWidget *qvtkOGLwidget;  // must point to the same object as View3DWidget's _vtkwidget.
    qvtkOGLwidget = static_cast<QVTKOpenGLWidget*>(arg);
    if( ! qvtkOGLwidget ){
        Application::instance()->logWarn("View3DWidget::rendererCallback(): arg is not a QVTKOpenGLWidget.  Check View3DWidget::_vtkwidget's class.");
    } else {
        // Place vtkRenderer event handling code here.
    }
}

void View3DWidget::onNewObject(const View3DListRecord object_info)
{
    Application::instance()->logInfo(
        "View3DWidget::onNewObject(): new object to display: "
        + object_info.getDescription());

    // Builds the VTK objects for the object to display in 3D.
    View3DViewData viewData = Application::instance()
                                  ->getProject()
                                  ->findObject(object_info.objectLocator)
                                  ->build3DViewObjects(this);

    // gets the VTK Actor that represents the domain object
    vtkSmartPointer<vtkProp> actor = viewData.actor;

    // adds the actor for viewing
    _rendererMainScene->AddActor(actor);

    // If object defined a label, adds it to the foreground scene.
    if( viewData.labelActor ) {
        applyCurrentTextStyle( viewData.labelActor );
        _rendererForeground->AddActor ( viewData.labelActor );
    }

    // redraw the scene
    _vtkwidget->GetRenderWindow()->Render();

    // keeps a list of locator-actor pairs to allow management
    _currentObjects.insert(object_info, viewData);
}

void View3DWidget::onRemoveObject(const View3DListRecord object_info)
{
    // Pop from the list the visual objects associated with the object to be removed.
    View3DViewData viewData = _currentObjects.take(object_info);

    // Removes the VTK actor matching the object locator.
    vtkSmartPointer<vtkProp> actor = viewData.actor;
    _rendererMainScene->RemoveActor(actor);

    // Removes the VTK actor of the label (if any).
    vtkSmartPointer<vtkProp> labelActor = viewData.labelActor;
    if( labelActor )
        _rendererForeground->RemoveActor( labelActor );

    // redraw the scene
    _vtkwidget->GetRenderWindow()->Render();

    removeCurrentConfigWidget();

    // removes and deletes the config widget (if any) associated with the object
    View3DConfigWidget *widget = nullptr;
    if (_currentCfgWidgets.contains(object_info)) {
        widget = _currentCfgWidgets.take(object_info);
        delete widget;
    }

    // TODO: verify whether the smart pointer manages memory after all local references to
    // the actor have been removed.
}

void View3DWidget::onViewAll()
{
    // adjusts view so everything fits in the screen
    _rendererMainScene->ResetCamera();
    // redraw the scene
    _vtkwidget->GetRenderWindow()->Render();
}

void View3DWidget::onLookAtXY()
{
    //_renderer->ResetCamera();
    double *fp = _rendererMainScene->GetActiveCamera()->GetFocalPoint();
    double *p = _rendererMainScene->GetActiveCamera()->GetPosition();
    double dist
        = std::sqrt((p[0] - fp[0]) * (p[0] - fp[0]) + (p[1] - fp[1]) * (p[1] - fp[1])
                    + (p[2] - fp[2]) * (p[2] - fp[2]));
    _rendererMainScene->GetActiveCamera()->SetPosition(fp[0], fp[1], fp[2] + dist);
    _rendererMainScene->GetActiveCamera()->SetViewUp(0.0, 1.0, 0.0);
    // redraw the scene
    _vtkwidget->GetRenderWindow()->Render();
}

void View3DWidget::onLookAtXZ()
{
    double *fp = _rendererMainScene->GetActiveCamera()->GetFocalPoint();
    double *p = _rendererMainScene->GetActiveCamera()->GetPosition();
    double dist
        = std::sqrt((p[0] - fp[0]) * (p[0] - fp[0]) + (p[1] - fp[1]) * (p[1] - fp[1])
                    + (p[2] - fp[2]) * (p[2] - fp[2]));
    _rendererMainScene->GetActiveCamera()->SetPosition(fp[0], fp[1] - dist, fp[2]);
    _rendererMainScene->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
    // redraw the scene
    _vtkwidget->GetRenderWindow()->Render();
}

void View3DWidget::onLookAtYZ()
{
    double *fp = _rendererMainScene->GetActiveCamera()->GetFocalPoint();
    double *p = _rendererMainScene->GetActiveCamera()->GetPosition();
    double dist
        = std::sqrt((p[0] - fp[0]) * (p[0] - fp[0]) + (p[1] - fp[1]) * (p[1] - fp[1])
                    + (p[2] - fp[2]) * (p[2] - fp[2]));
    _rendererMainScene->GetActiveCamera()->SetPosition(fp[0] + dist, fp[1], fp[2]);
    _rendererMainScene->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
    // redraw the scene
    _vtkwidget->GetRenderWindow()->Render();
}

void View3DWidget::onObjectsListItemActivated(QListWidgetItem *item)
{
    // retrieve the selected item's object info
    View3DListRecord object_info = item->data(Qt::UserRole).value<View3DListRecord>();

    // fetch the object's pointer
    ProjectComponent *object
        = Application::instance()->getProject()->findObject(object_info.objectLocator);

    // removes the current view config widget
    removeCurrentConfigWidget();

    // if an object was found
    if (object) {
        Application::instance()->logInfo(
            "View3DWidget::onObjectsListItemActivated(): object found: "
            + object->getName() + ".");
        // retrieve or create the config widget for the object
        View3DConfigWidget *widget = nullptr;
        if (_currentCfgWidgets.contains(object_info)) {
            widget = _currentCfgWidgets[object_info];
        } else {
            View3DViewData viewObjects = _currentObjects[object_info];
            widget = object->build3DViewerConfigWidget(viewObjects);
            if (widget) {
                _currentCfgWidgets.insert(object_info, widget);
                // connects the signal/slot upon user changes
                connect(widget, SIGNAL(changed()), this, SLOT(onConfigWidgetChanged()));
            }
        }
        // if there is a config widget
        if (widget) {
            // places the config widget in the interface
            ui->frmDataViewOptions->layout()->addWidget(widget);
            // sets as the current config widget
            _currentCfgWidget = widget;
        } else {
            Application::instance()->logError(
                "View3DWidget::onObjectsListItemActivated(): null widget returned.");
        }
    } else {
        Application::instance()->logError(
            "View3DWidget::onObjectsListItemActivated(): object with locator/instance "
            + object_info.getDescription() + " not found.");
    }
}

void View3DWidget::onConfigWidgetChanged()
{
    Application::instance()->logInfo("View3DWidget::onConfigWidgetChanged()");
    _rendererMainScene->Render();
    _vtkwidget->GetRenderWindow()->Render();
}

void View3DWidget::onVerticalExaggeration()
{
    _verticalExaggWiget->show();
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    mousePos.setX(mousePos.x() - _verticalExaggWiget->width());
    _verticalExaggWiget->move(mousePos);
    _verticalExaggWiget->setFocus();
}

void View3DWidget::onVerticalExaggerationChanged(double value)
{
    // Get the current model (objects) transform matrix.
    vtkSmartPointer<vtkMatrix4x4> xform
        = _rendererMainScene->GetActiveCamera()->GetModelTransformMatrix();

    // Get the camera's focal point (where it is looking at).
    double *fp = _rendererMainScene->GetActiveCamera()->GetFocalPoint();

    // Get where the focal point would have to go so the scene stays focused.
    double offset = fp[2] * value;

    // Scale the whole scene along the Z axis.
    xform->SetElement(2, 2, value);

    // Translate back the whole scene so the scene stays in the same place when viewed.
    xform->SetElement(2, 3, fp[2] - offset);

    // Apply transform to the whole scene.
    _rendererMainScene->GetActiveCamera()->SetModelTransformMatrix(xform);

    // redraw the scene (none of these works :( )
    {
        _vtkwidget->GetRenderWindow()->GetInteractor()->Render();
        _vtkwidget->GetRenderWindow()->Render();
        _vtkwidget->repaint();
        QApplication::sendPostedEvents();
        //this->parentWidget()->update();
        _rendererMainScene->Modified();
        _rendererMainScene->Render();
        vtkSmartPointer< vtkRenderWindow > renderWindow = _vtkwidget->GetRenderWindow();
        renderWindow->Render();
        renderWindow->Modified();
        QApplication::processEvents();
    }

    //Call this slot to update the scaling of the picking marker.
    m_myInteractor->OnMouseWheelForward();
}

void View3DWidget::onTextStyle()
{
    _textConfigWiget->show();
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    mousePos.setX(mousePos.x() - _textConfigWiget->width());
    _textConfigWiget->move(mousePos);
    _textConfigWiget->setFocus();
}

void View3DWidget::onTextConfigChanged()
{
    QMap<View3DListRecord, View3DViewData>::iterator iterCO = _currentObjects.begin();
    for( ; iterCO != _currentObjects.end(); ++iterCO ){

        // Get the visual objects.
        View3DViewData viewData = iterCO.value();

        // Reconfigures the VTK actor of the label (if any).
        vtkSmartPointer<vtkBillboardTextActor3D> labelActor = viewData.labelActor;
        if( labelActor )
            applyCurrentTextStyle( labelActor );
    }

    // redraw the scene
    _vtkwidget->GetRenderWindow()->Render();
}
