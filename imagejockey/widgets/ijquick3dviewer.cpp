//----------Since we're not building with CMake, we need to init the VTK modules------------------
//--------------linking with the VTK libraries is often not enough--------------------------------
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2) // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle)
VTK_MODULE_INIT(vtkRenderingFreeType)
//------------------------------------------------------------------------------------------------

#include "ijquick3dviewer.h"
#include "ui_ijquick3dviewer.h"

#include <QVTKOpenGLWidget.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPolyDataMapper.h>
#include <vtkSphereSource.h>
#include <vtkDataSetMapper.h>
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>

IJQuick3DViewer::IJQuick3DViewer( QWidget *parent ) :
    QWidget(parent),
    ui(new Ui::IJQuick3DViewer)
{
    ui->setupUi(this);

	this->setWindowTitle("Quick 3D Viewer");

	_vtkwidget = new QVTKOpenGLWidget();

	_renderer = vtkSmartPointer<vtkRenderer>::New();

	// enable antialiasing
	_renderer->SetUseFXAA( true );

	_vtkwidget->SetRenderWindow(vtkGenericOpenGLRenderWindow::New());
	_vtkwidget->GetRenderWindow()->AddRenderer(_renderer);
	_vtkwidget->setFocusPolicy(Qt::StrongFocus);

	//----------------------adding the orientation axes-------------------------
	vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
	_vtkAxesWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
	_vtkAxesWidget->SetOutlineColor(0.9300, 0.5700, 0.1300);
	_vtkAxesWidget->SetOrientationMarker(axes);
	_vtkAxesWidget->SetInteractor(_vtkwidget->GetRenderWindow()->GetInteractor());
	_vtkAxesWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
	_vtkAxesWidget->SetEnabled(1);
	_vtkAxesWidget->InteractiveOn();


	// add the VTK widget the layout
	ui->frmViewer->layout()->addWidget(_vtkwidget);
}

IJQuick3DViewer::~IJQuick3DViewer()
{
	delete ui;
}

vtkSmartPointer<vtkLookupTable> makeGrayScaleColorTable(double min, double max)
{
	size_t tableSize = 32;

	//create a color interpolator object
	vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToRGB();
	ctf->AddRGBPoint(0.00, 0.000, 0.000, 0.000);
	ctf->AddRGBPoint(1.00, 1.000, 1.000, 1.000);

	//create the color table object
	vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
	lut->SetTableRange(min, max);
	lut->SetNumberOfTableValues(tableSize);
	for(size_t i = 0; i < tableSize; ++i)
	{
		double *rgb;
		rgb = ctf->GetColor(static_cast<double>(i)/tableSize);
		lut->SetTableValue(i, rgb[0], rgb[1], rgb[2]);
	}
	lut->SetRampToLinear();
	lut->Build();

	return lut;
}


void IJQuick3DViewer::display( vtkPolyData* polyData )
{
	clearScene();

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData( polyData );

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper( mapper );

	_renderer->AddActor(actor);
	_currentActor = actor;

	//===========VTK TEST CODE==========================================
//		vtkSmartPointer<vtkSphereSource> sphereSource =
//			vtkSmartPointer<vtkSphereSource>::New();
//		vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
//			vtkSmartPointer<vtkPolyDataMapper>::New();
//		sphereMapper->SetInputConnection( sphereSource->GetOutputPort() );
//		vtkSmartPointer<vtkActor> sphereActor =
//			vtkSmartPointer<vtkActor>::New();
//		sphereActor->SetMapper( sphereMapper );
//		_renderer->AddActor(sphereActor);
	//==================================================================


	_renderer->ResetCamera();

	_vtkwidget->GetRenderWindow()->Render();
}

void IJQuick3DViewer::display(vtkImageData * imageData, double colorScaleMin, double colorScaleMax )
{
	clearScene();

	vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
	mapper->SetInputData( imageData );

	//assign a color table
	vtkSmartPointer<vtkLookupTable> lut = makeGrayScaleColorTable( colorScaleMin, colorScaleMax );

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	mapper->SetLookupTable(lut);
	mapper->SetScalarRange(colorScaleMin, colorScaleMax);

	this->setWindowTitle("Quick 3D Viewer   min=" + QString::number( colorScaleMin ) + " max=" + QString::number( colorScaleMax ));

	actor->SetMapper( mapper );

	_renderer->AddActor(actor);
	_currentActor = actor;

	_renderer->ResetCamera();

	_vtkwidget->GetRenderWindow()->Render();
}

void IJQuick3DViewer::clearScene()
{
	if( _currentActor.GetPointer() ){
		_renderer->RemoveActor( _currentActor );
		_currentActor = nullptr;
	}

}

void IJQuick3DViewer::onDismiss()
{
	this->close();
}

