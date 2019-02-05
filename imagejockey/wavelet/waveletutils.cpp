#include "waveletutils.h"

#include "imagejockey/ijabstractcartesiangrid.h"
#include "imagejockey/svd/svdfactor.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "spectral/spectral.h"

#include <gsl/gsl_sort.h>
#include <itkMirrorPadImageFilter.h>

WaveletUtils::WaveletUtils()
{
}

spectral::array WaveletUtils::transform( IJAbstractCartesianGrid *cg,
                              int variableIndex ,
                              WaveletFamily waveletFamily,
                              int waveletType,
                              bool interleaved )
{
    //load the input data as an array
    cg->dataWillBeRequested();
    spectral::arrayPtr inputAsArray( cg->createSpectralArray( variableIndex ) );

    //convert the array into an image object
    GaborUtils::ImageTypePtr inputAsITK = GaborUtils::convertSpectralArrayToITKImage( *inputAsArray );

    //mirror pad the image to the needed dimension (power of 2)
    int nPowerOf2;
    GaborUtils::ImageTypePtr inputMirrorPadded = squareAndMirrorPad( inputAsITK, nPowerOf2 );

    //the following low-level code is necessary to interface to GSL library.

    //the input data in raw format used by GSL
    double *data = new double[ nPowerOf2 * nPowerOf2 ];
    fillRawArray( inputMirrorPadded, data );

//    //array to hold the absolute values of the DWT coefficients
//    double *abscoeff = new double[ nPowerOf2 * nPowerOf2 ];

    //array to hold the indexes of the coefficients ordered by value.
//    size_t *p = new size_t[ nPowerOf2 * nPowerOf2 ];

    //the wavelet
    gsl_wavelet *w = makeWavelet( waveletFamily, waveletType );

    //the transform in 2D operates on individual rows and columns.
    gsl_wavelet_workspace *work = gsl_wavelet_workspace_alloc ( nPowerOf2 );

    //DWT
    if( interleaved )
        gsl_wavelet2d_nstransform_forward( w, data, nPowerOf2, nPowerOf2, nPowerOf2, work );
    else
        gsl_wavelet2d_transform_forward( w, data, nPowerOf2, nPowerOf2, nPowerOf2, work );

    //prepare the result
    spectral::array result( nPowerOf2, nPowerOf2, 1, 0.0 );
    for( int j = 0; j < nPowerOf2; ++j )
        for( int i = 0; i < nPowerOf2; ++i )
            result(i, j) = data[ j * nPowerOf2 + i];

//    //compute the absolute values of the DWT coefficients
//    for (int i = 0; i < nPowerOf2*nPowerOf2; i++)
//        abscoeff[i] = fabs (data[i]);

//    //order them by their values
//    gsl_sort_index (p, abscoeff, 1, nPowerOf2*nPowerOf2);

//    //filter, which means to zero-out the coefficients corresponding to certain
//    //energy levels
//    for (int i = 0; i < nPowerOf2*nPowerOf2; i++)
//        if( i < nPowerOf2*nPowerOf2-20 || i > nPowerOf2*nPowerOf2 )
//            data[p[i]] = 0;

//    //DWT back transform
//    if( interleaved )
//        gsl_wavelet2d_nstransform_inverse(w, data, nPowerOf2, nPowerOf2, nPowerOf2, work);
//    else
//        gsl_wavelet2d_transform_inverse(w, data, nPowerOf2, nPowerOf2, nPowerOf2, work);

//    //free allocated resources
    gsl_wavelet_free (w);
    gsl_wavelet_workspace_free (work);
    delete[] data;
//    delete[] abscoeff;
//    delete[] p;

    return result;
}

spectral::array WaveletUtils::backtrans(IJAbstractCartesianGrid *gridWithOriginalGeometry,
                                        const spectral::array& input,
                                        WaveletFamily waveletFamily,
                                        int waveletType,
                                        bool interleaved)
{
    //assuming the input grid is square and with dimension that is a power of 2.
    int nPowerOf2 = input.M();

    //load input data into a raw array of doubles (readable by GSL)
    double *data = new double[ nPowerOf2 * nPowerOf2 ];
    for( int j = 0; j < nPowerOf2; ++j )
        for( int i = 0; i < nPowerOf2; ++i )
            data[ j * nPowerOf2 + i ] = input( i, j );

    //the wavelet
    gsl_wavelet *w = makeWavelet( waveletFamily, waveletType );

    //the transform in 2D operates on individual rows and columns.
    gsl_wavelet_workspace *work = gsl_wavelet_workspace_alloc ( nPowerOf2 );

    //DWT back transform
    if( interleaved )
        gsl_wavelet2d_nstransform_inverse(w, data, nPowerOf2, nPowerOf2, nPowerOf2, work);
    else
        gsl_wavelet2d_transform_inverse(w, data, nPowerOf2, nPowerOf2, nPowerOf2, work);

    //prepare the result
    int nI = gridWithOriginalGeometry->getNI();
    int nJ = gridWithOriginalGeometry->getNJ();
    spectral::array result( nI, nJ, 1, 0.0 );
    for( int j = 0; j < nPowerOf2; ++j ){
        int jr = j - ( nPowerOf2 - nJ );
        for( int i = 0; i < nPowerOf2; ++i ){
            //the retrotransformed result is to the top, left corner of the square grid
            //with the power-of-2 dimension
            if( i < nI && jr >= 0 )
                result(i, jr) = data[ j * nPowerOf2 + i ];
        }
    }

    //free allocated resources
    gsl_wavelet_free(w);
    gsl_wavelet_workspace_free (work);
    delete[] data;

    return result;
}

void WaveletUtils::backtrans(double *data, int nLog2nData, WaveletFamily waveletFamily, int waveletType)
{
    //assuming the input array has a lenght that is a power of 2.
    int nPowerOf2 = 1 << nLog2nData;

    //the wavelet
    gsl_wavelet *w = makeWavelet( waveletFamily, waveletType );

    //the workspace for the algorithm.
    gsl_wavelet_workspace *work = gsl_wavelet_workspace_alloc ( nPowerOf2 );

    //DWT back transform
    gsl_wavelet_transform_inverse(w, data, 1, nPowerOf2, work);

    //free allocated resources
    gsl_wavelet_free(w);
    gsl_wavelet_workspace_free (work);
}

void WaveletUtils::fillRawArray(const GaborUtils::ImageTypePtr input, double *output)
{
    //get input grid dimensions
    GaborUtils::ImageType::RegionType region = input->GetLargestPossibleRegion();
    GaborUtils::ImageType::SizeType size = region.GetSize();
    int nI = size[0];
    int nJ = 1;
    if( GaborUtils::gridDim > 1 )
        nJ = size[1];
    int nK = 1;
    if( GaborUtils::gridDim > 2 )
        nK = size[2];

    //transfer values to the allocated raw array of doubles
    for(unsigned int k = 0; k < nK; ++k)
        for(unsigned int j = 0; j < nJ; ++j)
            for(unsigned int i = 0; i < nI; ++i){
                itk::Index<GaborUtils::gridDim> index;
                switch( GaborUtils::gridDim ){
                case 3: index[2] = nK - 1 - k; // itkImage grid convention is different from GSLib's
                case 2: index[1] = nJ - 1 - j; // itkImage grid convention is different from GSLib's
                }
                index[0] = i;
                GaborUtils::realType value = input->GetPixel( index );
                output[ k * nJ * nI + j * nI + i ] = value;
            }
}

GaborUtils::ImageTypePtr WaveletUtils::squareAndMirrorPad(const GaborUtils::ImageTypePtr input, int &nPowerOf2)
{
    typedef itk::MirrorPadImageFilter <GaborUtils::ImageType, GaborUtils::ImageType>
            MirrorPadImageFilterType;

    //get the grid dimensions
    //get input grid dimensions
    GaborUtils::ImageType::RegionType region = input->GetLargestPossibleRegion();
    GaborUtils::ImageType::SizeType size = region.GetSize();
    spectral::index nI = size[0];
    spectral::index nJ = 1;
    if( GaborUtils::gridDim > 1 )
        nJ = size[1];
    spectral::index nK = 1;
    if( GaborUtils::gridDim > 2 )
        nK = size[2];

    //get which dimension has the greatest value
    int nMax = std::max( nI, nJ );

    //find the smallest power of 2 that is greater than or equal
    //the greatest dimension.
    //make it the grid dimension for DWT
    nPowerOf2 = 1;
    while( nPowerOf2 < nMax )
        nPowerOf2 <<= 1;

    GaborUtils::ImageType::SizeType lowerBound;
    lowerBound[0] = 0;
    lowerBound[1] = 0;
    GaborUtils::ImageType::SizeType upperBound;
    upperBound[0] = nPowerOf2 - nI;
    upperBound[1] = nPowerOf2 - nJ;
    MirrorPadImageFilterType::Pointer padFilter
            = MirrorPadImageFilterType::New();
    padFilter->SetInput( input );
    padFilter->SetPadLowerBound(lowerBound);
    padFilter->SetPadUpperBound(upperBound);
    padFilter->Update();

    return padFilter->GetOutput();
}

void WaveletUtils::debugGridRawArray( const double* in, int nI, int nJ, int nK ){
    spectral::array a( nI, nJ, nK, 0.0 );
    for(unsigned int k = 0; k < nK; ++k)
        for(unsigned int j = 0; j < nJ; ++j)
            for(unsigned int i = 0; i < nI; ++i){
                a(i, j, k) = in[ k * nJ * nI + j * nI + i ];
            }
    debugGrid( a );
}

gsl_wavelet *WaveletUtils::makeWavelet(WaveletFamily waveletFamily,
                                       int waveletType )
{
    gsl_wavelet *w = nullptr;
    switch ( waveletFamily ) {
    case WaveletFamily::DAUBECHIES:
        w = gsl_wavelet_alloc ( gsl_wavelet_daubechies, waveletType );
        break;
    case WaveletFamily::HAAR:
        w = gsl_wavelet_alloc ( gsl_wavelet_haar, waveletType );
        break;
    case WaveletFamily::B_SPLINE:
        w = gsl_wavelet_alloc ( gsl_wavelet_bspline, waveletType );
        break;
    default:
        break;
    }
    return w;
}

void WaveletUtils::debugGridITK( const GaborUtils::ImageType& in ){
    spectral::array a = GaborUtils::convertITKImageToSpectralArray( in );
    debugGrid( a );
}

void WaveletUtils::debugGrid(const spectral::array &grid)
{
    spectral::array result ( grid );
    SVDFactor* gridSVD = new SVDFactor( std::move(result), 1, 0.42,
                                     0.0,
                                     0.0,
                                     0.0,
                                     1.0,
                                     1.0,
                                     1.0, 0.0 );
    IJGridViewerWidget* ijgv = new IJGridViewerWidget( true, false, true );
    ijgv->setFactor( gridSVD );
    ijgv->show();
}
