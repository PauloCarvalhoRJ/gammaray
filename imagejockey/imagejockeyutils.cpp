#include "imagejockeyutils.h"
#include "ijspatiallocation.h"
#include "ijabstractcartesiangrid.h"
#include "spectral/spectral.h"
#include <cstdlib>
#include <cmath>
#include <complex>
#include <QList>
#include <QProgressDialog>
#include <QCoreApplication>
#include <QDir>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkPointData.h>
#include <vtkImageStencil.h>
#include <vtkContourFilter.h>

/*static*/const long double ImageJockeyUtils::PI( 3.141592653589793238L );

/*static*/const long double ImageJockeyUtils::PI_OVER_180( ImageJockeyUtils::PI / 180.0L );

ImageJockeyUtils::ImageJockeyUtils()
{
}

double ImageJockeyUtils::dB( double value, double refLevel, double epsilon, double scaleFactor )
{
	double absValue = std::abs( value );
	double valueToUse = value;
	if( absValue < epsilon ){
		if( value < 0.0 )
			valueToUse = -epsilon;
		else
			valueToUse = epsilon;
	}
	return scaleFactor * std::log10( valueToUse / refLevel );
}

IJMatrix3X3<double> ImageJockeyUtils::getAnisoTransform(double aSemiMajor,
                                                      double aSemiMinor,
                                                      double aSemiVert,
                                                      double azimuth,
                                                      double dip,
                                                      double roll)
{

    //convert the angles to radians and to trigonometric convention
    double azimuthRad = (azimuth - 90.0) * PI_OVER_180;
    double dipRad = dip * PI_OVER_180;
    double rollRad = roll * PI_OVER_180;

    //----------rotate the world so the aniso axes are parallel to world axes--------------------------
    IJMatrix3X3<double> Tyaw( std::cos(azimuthRad), -std::sin(azimuthRad), 0.0,
                            std::sin(azimuthRad), std::cos(azimuthRad),  0.0,
                            0.0,               0.0,                1.0);
    IJMatrix3X3<double> Tpitch( std::cos(dipRad),  0.0, std::sin(dipRad),
                              0.0,            1.0,           0.0,
                              -std::sin(dipRad), 0.0, std::cos(dipRad));
    IJMatrix3X3<double> Troll( 1.0,            0.0,             0.0,
                             0.0, std::cos(rollRad), std::sin(rollRad),
                             0.0, -std::sin(rollRad), std::cos(rollRad));
    //----------stretches the world so the aniso ranges are now equal (spherical) --------------------
    IJMatrix3X3<double> S( 1.0,                   0.0,                  0.0,
                         0.0, aSemiMajor/aSemiMinor,                  0.0,
                         0.0,                   0.0, aSemiMajor/aSemiVert);
    //     <--------- order of transform application
    //  the final effect is that we transform the world so the anisotropy becomes isotropy
    return       S * Troll * Tpitch * Tyaw;
}

void ImageJockeyUtils::transform(IJMatrix3X3<double> &t, double &a1, double &a2, double &a3)
{
    double temp_a1 = t._a11 * a1 + t._a12 * a2 + t._a13 * a3;
    double temp_a2 = t._a21 * a1 + t._a22 * a2 + t._a23 * a3;
    double temp_a3 = t._a31 * a1 + t._a32 * a2 + t._a33 * a3;
    a1 = temp_a1;
    a2 = temp_a2;
    a3 = temp_a3;
}

QString ImageJockeyUtils::humanReadable(double value)
{
    //buffer string for formatting the output (QString's sptrintf doesn't honor field size)
    char buffer[50];
    //define base unit to change suffix (could be 1024 for ISO bytes (iB), for instance)
    double unit = 1000.0d;
    //return the plain value if it doesn't require a multiplier suffix (small values)
    if (value <= unit){
        std::sprintf(buffer, "%.1f", value);
        return QString( buffer );
    }
    //compute the order of magnitude (approx. power of 1000) of the value
    int exp = (int) (std::log10(value) / std::log10(unit));
    //string that is a list of available multiplier suffixes
    QString suffixes = "pnum kMGTPE";
    //select the suffix
    char suffix = suffixes.at( 5+exp-1 ).toLatin1(); //-5 because pico would result in a -5 index.
    //format output, dividing the value by the power of 1000 found
    std::sprintf(buffer, "%.1f%c", value / std::pow<double, int>(unit, exp), suffix);
    return QString( buffer );
}

void ImageJockeyUtils::mirror2D(QList<QPointF> &points, const IJSpatialLocation &point)
{
    QList<QPointF>::iterator it = points.begin();
    for( ; it != points.end(); ++it){
        double dx = (*it).x() - point._x;
        double dy = (*it).y() - point._y;
        (*it).setX( point._x - dx );
        (*it).setY( point._y - dy );
    }
}

bool ImageJockeyUtils::isWithinBBox(double x, double y, double minX, double minY, double maxX, double maxY)
{
    if( x < minX ) return false;
    if( x > maxX ) return false;
    if( y < minY ) return false;
    if( y > maxY ) return false;
    return true;
}

bool ImageJockeyUtils::prepareToFFTW3reverseFFT(IJAbstractCartesianGrid *gridWithAmplitudes,
                                                uint indexOfVariableWithAmplitudes,
                                                IJAbstractCartesianGrid *gridWithPhases,
                                                uint indexOfVariableWithPhases,
                                                spectral::complex_array& output)
{
    //Get the complex numbers:
    //            a) in polar form ( a cis b );
    //            b) with the lower frequencies shifted to the center for ease of interpretation;
    //            c) grid scan order following the GSLib convention.
    spectral::complex_array* dataOriginal;
    if( gridWithAmplitudes == gridWithPhases ){
        //both amplitudes and phases come from the same grid: simple.
        dataOriginal = gridWithAmplitudes->createSpectralComplexArray(
                                                            indexOfVariableWithAmplitudes,
                                                            indexOfVariableWithPhases
                                                                  );
        if( ! dataOriginal )
            return false;
    }else{
        //Amplitudes and phases come from different grids.
        dataOriginal = new spectral::complex_array( gridWithAmplitudes->getNI(),
                                                    gridWithAmplitudes->getNJ(),
                                                    gridWithAmplitudes->getNK());
        int i = 0; /*int nI = gridWithAmplitudes->getNI();*/
        int j = 0; int nJ = gridWithAmplitudes->getNJ();
        int k = 0; int nK = gridWithAmplitudes->getNK();
        //Recall that spectral::complex_array grid scan convention (inner = k, mid = j, outer = i) is the opposite
        // of GSLib's (inner = i, mid = j, outer = k)
        for( int iGlobal = 0; iGlobal < dataOriginal->size(); ++iGlobal ){
            dataOriginal->d_[iGlobal][0] = gridWithAmplitudes->getData( indexOfVariableWithAmplitudes,
                                                                        i, j, k);
            dataOriginal->d_[iGlobal][1] = gridWithPhases->getData( indexOfVariableWithPhases,
                                                                        i, j, k);
            ++k;
            if( k == nK ){
                k = 0;
                ++j;
            }
            if( j == nJ ){
                j = 0;
                ++i;
            }
        }
    }

    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.show();
    progressDialog.setLabelText("Converting FFT image...");
    unsigned int nI = gridWithAmplitudes->getNI();
    unsigned int nJ = gridWithAmplitudes->getNJ();
    unsigned int nK = gridWithAmplitudes->getNK();
    for(unsigned int k = 0; k < nK; ++k) {
        QCoreApplication::processEvents(); //let Qt repaint widgets
        //de-shift in topological direction K
        int k_shift = (k + nK/2) % nK;
        for(unsigned int j = 0; j < nJ; ++j){
            //de-shift in topological direction J
            int j_shift = (j + nJ/2) % nJ;
            for(unsigned int i = 0; i < nI; ++i){
                //de-shift in topological direction I
                int i_shift = (i + nI/2) % nI;
                //compute the element index in the complex arrays
                //the scan order of fftw follows is the opposite of the GSLib convention
                int idxOriginal = k_shift + nK * (j_shift + nJ * i_shift );
                int idxReady = k + nK * ( j + nJ * i );
                //convert it to rectangular form
                std::complex<double> value = std::polar( dataOriginal->d_[idxOriginal][0],
                                                         dataOriginal->d_[idxOriginal][1] );
                //fills the output array with the final rectangular form
                output.d_[idxReady][0] = value.real();
                output.d_[idxReady][1] = value.imag();
            }
        }
    }
    //discard the intermediary array.
    delete dataOriginal;
    return true;
}

QString ImageJockeyUtils::generateUniqueFilePathInDir(const QString directory, const QString file_extension)
{
    while(true){
        int r = ( (int)((double)rand() / RAND_MAX * 10000000)) + 10000000;
        QString filename = QString::number(r);
        filename.append(".");
        filename.append(file_extension);
        QDir dir( directory );
        QFile file(dir.absoluteFilePath(filename));
        if( ! file.exists() )
            return dir.absoluteFilePath(filename);
	}
}

void ImageJockeyUtils::makeVTKImageDataFromSpectralArray(vtkImageData * out, const spectral::array & in)
{
	out->SetExtent(0, in.M()-1, 0, in.N()-1, 0, in.K()-1); //extent (indexes) of GammaRay grids start at i=0,j=0,k=0
	out->AllocateScalars(VTK_DOUBLE, 1); //each cell will contain one double value.
	int* extent = out->GetExtent();

	for (int k = extent[4]; k <= extent[5]; ++k){
		for (int j = extent[2]; j <= extent[3]; ++j){
			for (int i = extent[0]; i <= extent[1]; ++i){
				double* pixel = static_cast<double*>(out->GetScalarPointer(i,j,k));
				pixel[0] = in( i, j, k );
			}
		}
    }
}

void ImageJockeyUtils::rasterize( spectral::array &out, vtkPolyData *in, double rX, double rY, double rZ )
{
    // Set grid cell sizes.
    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
    double bounds[6];
    in->GetBounds(bounds);
    double spacing[3]; // desired volume spacing
    spacing[0] = rX;
    spacing[1] = rY;
    spacing[2] = rZ;
    image->SetSpacing(spacing);

    // Compute grid dimensions.
    int dim[3];
    for (int i = 0; i < 3; i++)
		dim[i] = static_cast<int>( std::abs( std::ceil((bounds[i * 2 + 1] - bounds[i * 2]) / spacing[i]) ) );
    image->SetDimensions(dim);
    image->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);

    // Set grid origin.
    double origin[3];
    origin[0] = bounds[0] + spacing[0] / 2;
    origin[1] = bounds[2] + spacing[1] / 2;
    origin[2] = bounds[4] + spacing[2] / 2;
    image->SetOrigin(origin);

    // Paints all grid cells.
    image->AllocateScalars( VTK_UNSIGNED_CHAR, 1 ); //doubles are not necessary to store in/out values
    unsigned char inval = 255;
    unsigned char outval = 0;
    vtkIdType count = image->GetNumberOfPoints();
    for (vtkIdType i = 0; i < count; ++i)
        image->GetPointData()->GetScalars()->SetTuple1(i, inval);

    // Make a vector geometry to raster image stencil.
    vtkSmartPointer<vtkPolyDataToImageStencil> pol2stenc = vtkSmartPointer<vtkPolyDataToImageStencil>::New();
    pol2stenc->SetInputData(in);
    pol2stenc->SetOutputOrigin(origin);
    pol2stenc->SetOutputSpacing(spacing);
    pol2stenc->SetOutputWholeExtent(image->GetExtent());
    pol2stenc->Update();

    // Perform stencil (paints voxels outside the shape with another value).
    vtkSmartPointer<vtkImageStencil> imgstenc = vtkSmartPointer<vtkImageStencil>::New();
    imgstenc->SetInputData(image);
    imgstenc->SetStencilConnection(pol2stenc->GetOutputPort());
    imgstenc->ReverseStencilOff();
    imgstenc->SetBackgroundValue(outval);
    imgstenc->Update();

    // Transfer results to spectral::array object.
    out.set_size( dim[0], dim[1], dim[2] );
    vtkImageData* outputImage = imgstenc->GetOutput();
    for( int k = 0; k < dim[2]; ++k )
        for( int j = 0; j < dim[1]; ++j )
            for( int i = 0; i < dim[0]; ++i ){
                double* cell = static_cast<double*>(outputImage->GetScalarPointer(i,j,k));
                out(i, j, k) = *cell;
			}
}

vtkSmartPointer<vtkPolyData> ImageJockeyUtils::computeIsosurfaces(const spectral::array & in,
																  int nContours,
																  double minValue,
																  double maxValue)
{
	//Convert the grid into a VTK grid object.
	vtkSmartPointer<vtkImageData> vtkVarmap = vtkSmartPointer<vtkImageData>::New();
	ImageJockeyUtils::makeVTKImageDataFromSpectralArray( vtkVarmap, in );
	//Create the varmap's isosurface(s).
	vtkSmartPointer<vtkContourFilter> contourFilter = vtkSmartPointer<vtkContourFilter>::New();
	contourFilter->SetInputData( vtkVarmap );
	contourFilter->GenerateValues( nContours, minValue, maxValue); // (numContours, rangeStart, rangeEnd)
	contourFilter->Update();
	//Get the isocontour/isosurface as polygonal data
	vtkPolyData* poly = contourFilter->GetOutput();
	//Copy it before the parent contour filter is destroyed.
	vtkSmartPointer<vtkPolyData> polydataCopy = vtkSmartPointer<vtkPolyData>::New();
	polydataCopy->DeepCopy(poly);
	return polydataCopy;
}
