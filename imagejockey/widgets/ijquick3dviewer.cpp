//----------Since we're not building with CMake, we need to init the VTK modules------------------
//--------------linking with the VTK libraries is often not enough--------------------------------
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2) // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle)
VTK_MODULE_INIT(vtkRenderingFreeType)
//------------------------------------------------------------------------------------------------

#include "ijquick3dviewer.h"
#include "ui_ijquick3dviewer.h"
#include "../imagejockeyutils.h"
#include "../../spectral/spectral.h"

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
#include <vtkProperty.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkPointData.h>

IJQuick3DViewer::IJQuick3DViewer( QWidget *parent ) :
    QWidget(parent),
    ui(new Ui::IJQuick3DViewer)
{
	_ownerThreadId = std::this_thread::get_id();

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

    // adjusts view so everything fits in the screen
    _renderer->ResetCamera();

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


void IJQuick3DViewer::display(vtkPolyData* polyData , int r, int g, int b)
{
	if (! threadCheck() )
		return;

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData( polyData );

	vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
	actor->SetMapper( mapper );
	actor->GetProperty()->SetColor(r, g, b);

	_renderer->AddActor(actor);
	_currentActors.push_back( actor );

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

void IJQuick3DViewer::display(vtkPolyData *polyData, float pointSize)
{
    if (! threadCheck() )
        return;

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData( polyData );

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper( mapper );
    actor->GetProperty()->SetColor(255, 255, 255);
    actor->GetProperty()->SetPointSize( pointSize );

    _renderer->AddActor(actor);
    _currentActors.push_back( actor );

    _renderer->ResetCamera();

    _vtkwidget->GetRenderWindow()->Render();
}

void IJQuick3DViewer::display(vtkImageData * imageData, double colorScaleMin, double colorScaleMax )
{
	if (! threadCheck() )
		return;

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
	_currentActors.push_back( actor );

	_renderer->ResetCamera();

	_vtkwidget->GetRenderWindow()->Render();
}

void IJQuick3DViewer::display( const spectral::array & grid, double colorScaleMin, double colorScaleMax )
{
    display( grid, colorScaleMin, colorScaleMax, 1.0, 1.0, 1.0 );
}

void IJQuick3DViewer::display(const spectral::array &grid, double colorScaleMin, double colorScaleMax,
                              double cell_size_I, double cell_size_J, double cell_size_K )
{
    vtkSmartPointer<vtkImageData> out = vtkSmartPointer<vtkImageData>::New();
    auto f = [] (double x) { return x; }; //does-nothing lambda (requirement of ImageJockeyUtils::makeVTKImageDataFromSpectralArray())
    ImageJockeyUtils::makeVTKImageDataFromSpectralArray( out, grid, f, cell_size_I, cell_size_J, cell_size_K );
    display( out, colorScaleMin, colorScaleMax );
}

void IJQuick3DViewer::display(const spectral::array &grid)
{
    double min = grid.min();
    double max = grid.max();
    display( grid, min, max );
}

void IJQuick3DViewer::clearScene()
{
	if (! threadCheck() )
		return;

	std::vector< vtkSmartPointer<vtkActor> >::iterator it = _currentActors.begin();
	for( ; it != _currentActors.end(); ){ // erase() already increments the iterator.
		_renderer->RemoveActor( *it );
		it = _currentActors.erase( it );
    }
}

void IJQuick3DViewer::hideDismissButton()
{
    ui->btnDismiss->hide();
}

bool IJQuick3DViewer::threadCheck()
{
	std::thread::id callingThreadId = std::this_thread::get_id();
	return callingThreadId == _ownerThreadId;
}

void IJQuick3DViewer::onDismiss()
{
    this->close();
}

