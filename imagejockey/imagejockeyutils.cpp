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
#include <vtkStripper.h>
#include <vtkCleanPolyData.h>
#include <vtkCenterOfMass.h>
#include <vtkEllipseArcSource.h>
#include <vtkAppendPolyData.h>

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
	vtkSmartPointer<vtkPolyData> poly = contourFilter->GetOutput();
	//Copy it before the parent contour filter is destroyed.
	vtkSmartPointer<vtkPolyData> polydataCopy = vtkSmartPointer<vtkPolyData>::New();
	polydataCopy->DeepCopy(poly);
	return polydataCopy;
}

void ImageJockeyUtils::removeOpenPolyLines(vtkSmartPointer<vtkPolyData> &polyDataToModify)
{
	// If there is no geometry, there is nothing to do.
	if ( polyDataToModify->GetNumberOfPoints() == 0 )
		return;

    // Join the connected lines into polygonal lines.
    vtkSmartPointer<vtkStripper> geometryJoiner = vtkSmartPointer<vtkStripper>::New();
    geometryJoiner->SetInputData( polyDataToModify );
    geometryJoiner->JoinContiguousSegmentsOn();
    geometryJoiner->Update();

    // Get the joined poly data object.
    vtkSmartPointer<vtkPolyData> joinedPolyData = geometryJoiner->GetOutput();

    // Get joined poly geometry data.
    // TODO: to get the joined isosurfaces, it is necessary to call vtkPolyData::GetPolys().
    vtkSmartPointer<vtkCellArray> in_Lines = joinedPolyData->GetLines();

    // Make the final poly data sans open poly lines.
    vtkSmartPointer<vtkPolyData> polyDataSansOpenLines = vtkSmartPointer<vtkPolyData>::New();

    // Initially populate the final poly data with the same vertexes.
    polyDataSansOpenLines->SetPoints( joinedPolyData->GetPoints() );

    // Prepare a container of closed lines.
    vtkSmartPointer<vtkCellArray> closedLines = vtkSmartPointer<vtkCellArray>::New();

    // Traverse the joined the poly lines.
    in_Lines->InitTraversal();
    vtkSmartPointer<vtkIdList> vertexIdList = vtkSmartPointer<vtkIdList>::New();
    while( in_Lines->GetNextCell( vertexIdList ) ){
        int initialVertexID = vertexIdList->GetId( 0 );
        int finalVertexID = vertexIdList->GetId( vertexIdList->GetNumberOfIds()-1 );
        // If the poly line is closed.
        if( initialVertexID == finalVertexID )
            // Adds the list of vertexes to the closed lines container.
            closedLines->InsertNextCell( vertexIdList );
    }

    // Assign the closed lines to the final poly data.
    polyDataSansOpenLines->SetLines( closedLines );

    // Remove unused vertexes.
    vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
    cleaner->SetInputData( polyDataSansOpenLines );
    cleaner->Update();

    // Replace the input poly data with the processed one without open poly lines.
    polyDataToModify = cleaner->GetOutput();
}

void ImageJockeyUtils::removeNonConcentricPolyLines(vtkSmartPointer<vtkPolyData> &polyDataToModify,
                                                    double centerX,
                                                    double centerY,
                                                    double centerZ,
													double toleranceRadius,
													int numberOfVertexesThreshold
                                                    )
{
    // If there is no geometry, there is nothing to do.
    if ( polyDataToModify->GetNumberOfPoints() == 0 )
        return;

    // Get poly lines.
    // TODO: for surfaces (3D), it is necessary to call vtkPolyData::GetPolys().
	vtkSmartPointer<vtkCellArray> in_Lines = vtkSmartPointer<vtkCellArray>::New();
	in_Lines->Allocate( polyDataToModify->GetNumberOfLines() );
	in_Lines->SetCells( polyDataToModify->GetNumberOfLines(), polyDataToModify->GetLines()->GetData() );

    // Prepare the resulting poly data.
    vtkSmartPointer<vtkPolyData> result = vtkSmartPointer<vtkPolyData>::New();

    // Initially set all the points in the result.
    result->SetPoints( polyDataToModify->GetPoints() );

	// Prepare a container of line definitions for the result.
    vtkSmartPointer<vtkCellArray> linesForResult = vtkSmartPointer<vtkCellArray>::New();

    // Traverse the input poly lines.
    in_Lines->InitTraversal();
    vtkSmartPointer<vtkIdList> vertexIdList = vtkSmartPointer<vtkIdList>::New();
	double vertexCoords[3];
	while( in_Lines->GetNextCell( vertexIdList ) ){

		// Compute the center of the poly line
		double center[3] = {0.0, 0.0, 0.0};
		for( int idVertex = 0; idVertex < vertexIdList->GetNumberOfIds(); ++idVertex ){
			polyDataToModify->GetPoint( vertexIdList->GetId( idVertex ), vertexCoords );
			center[0] += vertexCoords[0];
			center[1] += vertexCoords[1];
			center[2] += vertexCoords[2];
		}
		center[0] /= vertexIdList->GetNumberOfIds();
		center[1] /= vertexIdList->GetNumberOfIds();
		center[2] /= vertexIdList->GetNumberOfIds();

        // Compute the distance to the point considered as "the" center.
        double dx = centerX - center[0];
        double dy = centerY - center[1];
        double dz = centerZ - center[2];
        double distance = std::sqrt( dx*dx + dy*dy + dz*dz );

        // Assign the line definitions if they correspond to a concentric poly line.
		if( distance <= toleranceRadius && vertexIdList->GetNumberOfIds() > numberOfVertexesThreshold )
            linesForResult->InsertNextCell( vertexIdList );
    }

    // Set the poly lines considered concentric.
    result->SetLines( linesForResult );

    // Discard the unused vertexes in the result.
    vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
    cleaner->SetInputData( result );
    cleaner->Update();

    // Replace the input poly data with the poly data containing only concentric poly lines.
    polyDataToModify = cleaner->GetOutput();
}

void ImageJockeyUtils::fitEllipses(const vtkSmartPointer<vtkPolyData> &polyData,
								   vtkSmartPointer<vtkPolyData> &ellipses,
								   double &mean_error,
								   double &max_error,
								   double &sum_error)
{
    // If there is no geometry, there is nothing to do.
    if ( polyData->GetNumberOfPoints() == 0 )
        return;

    // Get poly lines definitions.
    vtkSmartPointer<vtkCellArray> in_Lines = polyData->GetLines();

    // Prepare the result poly data.
    vtkSmartPointer<vtkPolyData> result = vtkSmartPointer<vtkPolyData>::New();

    // Traverse the input poly lines.
    in_Lines->InitTraversal();
    vtkSmartPointer<vtkIdList> vertexIdList = vtkSmartPointer<vtkIdList>::New();
	sum_error = 0.0;
	max_error = 0.0;
    while( in_Lines->GetNextCell( vertexIdList ) ){

        // Collect the X and Y vertex coordinates of a poly line
        spectral::array aX( vertexIdList->GetNumberOfIds() );
        spectral::array aY( vertexIdList->GetNumberOfIds() );
        for( vtkIdType iVertex = 0; iVertex < vertexIdList->GetNumberOfIds(); ++iVertex ){
            double vertex[3];
            ///////NOTE: DO NOT CALL THIS METHOD FROM MULTIPLE THREADS.
            polyData->GetPoint( vertexIdList->GetId( iVertex ), vertex );
            ////////////////////////////////////////////////////////////
            aX( iVertex ) = vertex[0];
            aY( iVertex ) = vertex[1];
        }

        // Fit the ellipse (find the A...F factors of its implicit equation).
        double A, B, C, D, E, F;
		double error;
		ImageJockeyUtils::ellipseFit( aX, aY, A, B, C, D, E, F, error );
		sum_error += error;
		max_error = std::max( max_error, error );

        // Find the geometric parameters of the ellipse.
        double semiMajorAxis, semiMinorAxis, rotationAngle, centerX, centerY;
		ImageJockeyUtils::getEllipseParametersFromImplicit2( A, B, C, D, E, F,
                                                            semiMajorAxis, semiMinorAxis, rotationAngle, centerX, centerY );

        // Make the ellipse poly.
        vtkSmartPointer< vtkEllipseArcSource > ellipseSource = vtkSmartPointer< vtkEllipseArcSource >::New();
        ellipseSource->SetCenter( centerX, centerY, 0 );
        ellipseSource->SetNormal( 0, 0, 1 );
        ellipseSource->SetMajorRadiusVector( semiMajorAxis * std::cos( rotationAngle ),
                                             semiMajorAxis * std::sin( rotationAngle ),
                                             0 );
        ellipseSource->SetSegmentAngle( 360 );
        ellipseSource->SetRatio( semiMinorAxis / semiMajorAxis );
        ellipseSource->Update();
        vtkSmartPointer< vtkPolyData > ellipse = ellipseSource->GetOutput();

        // Append the ellipse poly data to the result poly data.
        vtkSmartPointer< vtkAppendPolyData > appendFilter = vtkSmartPointer< vtkAppendPolyData >::New();
        appendFilter->AddInputData( result );
        appendFilter->AddInputData( ellipse );
        appendFilter->Update();

        //result = result + ellipse.
        result = appendFilter->GetOutput();
    }

	if( in_Lines->GetNumberOfCells() > 0 )
		mean_error = sum_error / in_Lines->GetNumberOfCells();

    // Return the result poly data.
    ellipses = result;
}

void ImageJockeyUtils::getEllipseParametersFromImplicit(double A, double B, double C, double D, double E, double F,
                                                        double &semiMajorAxis,
                                                        double &semiMinorAxis,
                                                        double &rotationAngle,
                                                        double &centerX,
                                                        double &centerY)
{
    // Input parameters: a, b, c, d, e, f
    rotationAngle = std::atan(B / (A - C)) * 0.5; // rotation
    double cos_phi = std::cos(rotationAngle);
    double sin_phi = std::sin(rotationAngle);
    centerX = (2 * C * D - B * E) / (B * B - 4 * A * C);
    centerY = (2 * A * E - B * D) / (B * B - 4 * A * C);
    //center = cv::Vec2d(u, v);        // center

    // eliminate rotation and recalculate 6 parameters
    double aa = A * cos_phi * cos_phi - B * cos_phi * sin_phi + C * sin_phi * sin_phi;
	//double bb = 0;
    double cc = A * sin_phi * sin_phi + B * cos_phi * sin_phi + C * cos_phi * cos_phi;
	//double dd = D * cos_phi - E * sin_phi;
	//double ee = D * sin_phi + E * cos_phi;
    double ff = 1 + (D * D) / (4 * A) + (E * E) / (4 * C);

    semiMajorAxis = std::sqrt(ff / aa);              // semi-major axis
    semiMinorAxis = std::sqrt(ff / cc);              // semi-minor axis

    if(semiMajorAxis < semiMinorAxis) {
        double temp = semiMajorAxis;
        semiMajorAxis = semiMinorAxis;
        semiMinorAxis = temp;
    }
}

void ImageJockeyUtils::getEllipseParametersFromImplicit2(double A, double B, double C, double D, double E, double F,
														 double & semiMajorAxis, double & semiMinorAxis, double & rotationAngle, double & centerX, double & centerY)
{
	double determinant = B*B - 4*A*C;
	double part1 = 2 * ( A*E*E + C*D*D - B*D*E + (determinant)*F );
	double part2 = std::sqrt( (A-C)*(A-C) + B*B );
	semiMajorAxis = -std::sqrt( part1 * (A+C + part2) ) / determinant;
	semiMinorAxis = -std::sqrt( part1 * (A+C - part2) ) / determinant;
	centerX = (2*C*D - B*E) / determinant;
	centerY = (2*A*E - B*D) / determinant;
	//determine the angle (of the ellipse's semi-major axis).
	{
		if( std::abs(B) < 0.00001){
			if( A < C )
				rotationAngle = 0;
			else
				rotationAngle = ImageJockeyUtils::PI / 2.0;
		} else
			rotationAngle = std::atan( ( C-A-part2 ) / B );
	}
}

void ImageJockeyUtils::ellipseFit(const spectral::array &aX, const spectral::array &aY,
								  double &A, double &B, double &C, double &D, double &E, double &F,
								  double& fitnessError )
{
    spectral::array aXX = spectral::hadamard( aX, aX ); //Hadamard product == element-wise product.
    spectral::array aXY = spectral::hadamard( aX, aY );
    spectral::array aYY = spectral::hadamard( aY, aY );
    spectral::array aOnes( aX.M(), 1.0 );

    // Build design matrix (n x 6), where n is the number of X,Y samples.
    spectral::array aDesign = spectral::joinColumnVectors( { &aXX, &aXY, &aYY, &aX, &aY, &aOnes } );

    // Build scatter matrix.
    // NOTE: Fitzgibbon et al (1996)'s Matlab implementation makes the scatter matrix as
    //     S = D' * D
    //     The ' operator is the transpose conjugate operator, which is different from
    //     the transpose operator .'.  But for real numbers, ' and .' result in the same matrix.
    //     The code below assumes all the elements are real numbers.
    spectral::array aScatter = spectral::transpose( aDesign ) * aDesign;

    // Build the 6x6 constraint matrix.
	// All elements are initialized with zeros.
    spectral::array aConstraint( (spectral::index)6, (spectral::index)6, (double)0.0 );
	aConstraint(0, 2) = 2;
	aConstraint(1, 1) = -1;
	aConstraint(2, 0) = 2;

    // Solve eigensystem.
    spectral::array eigenvectors, eigenvalues;
    std::tie( eigenvectors, eigenvalues ) = spectral::eig( spectral::inv( aScatter ) * aConstraint );

    // Find the index of the positive eigenvalue.
    int PosC = 0;
    for( ; PosC < 6; ++PosC )
        if( eigenvalues( PosC ) > 0 && std::isfinite( eigenvalues( PosC ) ) )
            break;

    // The PosC-th eigenvector contains the A...F factors, which are returned.
    A = eigenvectors( 0, PosC );
    B = eigenvectors( 1, PosC );
    C = eigenvectors( 2, PosC );
    D = eigenvectors( 3, PosC );
    E = eigenvectors( 4, PosC );
    F = eigenvectors( 5, PosC );

	// ============ Paper's agorithm ends here. ===============

	// ============= Commencing fitness error computation. ============

	// Create the a vector-column with the A...F factors.
	spectral::array a( (spectral::index)6, (double)0.0 );
	a(0) = A; a(1) = B; a(2) = C; a(3) = D; a(4) = E; a(5) = F;

	// Acoording to the paper, the objective is to minimize ||Da||^2, that is, the fitness error.
	// D in the paper is the design matrix, or aDesign in this code.
	spectral::array Da = aDesign * a;
	Da = Da / Da.euclideanLength(); //normalize the Da vector to remove scale effect (error would be proportional to the size of the fitted ellipse).
	fitnessError = Da(0)*Da(0) + Da(1)*Da(1) + Da(2)*Da(2) + Da(3)*Da(3) + Da(4)*Da(4) + Da(5)*Da(5);
}
