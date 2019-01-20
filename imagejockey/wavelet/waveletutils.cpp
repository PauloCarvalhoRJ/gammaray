#include "waveletutils.h"

#include "imagejockey/ijabstractcartesiangrid.h"
#include "imagejockey/svd/svdfactor.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "spectral/spectral.h"

#include <gsl/gsl_wavelet2d.h>
#include <gsl/gsl_sort.h>
#include <itkMirrorPadImageFilter.h>

WaveletUtils::WaveletUtils()
{
}

void WaveletUtils::transform( IJAbstractCartesianGrid *cg, int variableIndex )
{
    //get the grid dimensions
    int nI = cg->getNI();
    int nJ = cg->getNJ();

    //get which dimension has the greatest value
    int nMax = std::max( nI, nJ );

    //find the smallest power of 2 that is greater than or equal
    //the greatest dimension.
    //make it the grid dimension for DWT
    int nPowerOf2 = 1;
    while( nPowerOf2 < nMax )
        nPowerOf2 <<= 1;


    //load the input data as an array
    cg->dataWillBeRequested();
    spectral::arrayPtr inputAsArray( cg->createSpectralArray( variableIndex ) );

    //conver the array into an image object
    GaborUtils::ImageTypePtr inputAsITK = GaborUtils::convertSpectralArrayToITKImage( *inputAsArray );

    //mirror pad the image to the needed dimension (power of 2)
    typedef itk::MirrorPadImageFilter <GaborUtils::ImageType, GaborUtils::ImageType>
            MirrorPadImageFilterType;
    GaborUtils::ImageType::SizeType lowerBound;
    lowerBound[0] = 0;
    lowerBound[1] = 0;
    GaborUtils::ImageType::SizeType upperBound;
    upperBound[0] = nPowerOf2 - nI;
    upperBound[1] = nPowerOf2 - nJ;
    MirrorPadImageFilterType::Pointer padFilter
            = MirrorPadImageFilterType::New();
    padFilter->SetInput( inputAsITK );
    padFilter->SetPadLowerBound(lowerBound);
    padFilter->SetPadUpperBound(upperBound);
    padFilter->Update();
    GaborUtils::ImageTypePtr inputMirrorPadded = padFilter->GetOutput();

    int nc = nPowerOf2*nPowerOf2 - 20;

    //the following low-level code is necessary to interface to GSL library.

    //the input data in raw format used by GSL
    double *data = new double[ nPowerOf2 * nPowerOf2 ];
    fillRawArray( inputMirrorPadded, data );

    double *abscoeff = new double[ nPowerOf2 * nPowerOf2 ];

    size_t *p = new size_t[ nPowerOf2 * nPowerOf2 ];

    gsl_wavelet *w = gsl_wavelet_alloc (gsl_wavelet_daubechies, 4);

    gsl_wavelet_workspace *work = gsl_wavelet_workspace_alloc ( nPowerOf2 * nPowerOf2 );

    gsl_wavelet2d_transform_forward( w, data, 1, nPowerOf2, nPowerOf2, work);

    for (int i = 0; i < nPowerOf2*nPowerOf2; i++)
        abscoeff[i] = fabs (data[i]);

    gsl_sort_index (p, abscoeff, 1, nPowerOf2*nPowerOf2);

    for (int i = 0; (i + nc) < nPowerOf2*nPowerOf2; i++)
        data[p[i]] = 0;

    gsl_wavelet2d_transform_inverse(w, data, 1, nPowerOf2, nPowerOf2, work);

    debugGridRawArray( data, nPowerOf2, nPowerOf2, 1 );

    //free allocated resources
    gsl_wavelet_free (w);
    gsl_wavelet_workspace_free (work);
    delete[] data;
    delete[] abscoeff;
    delete[] p;
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

void WaveletUtils::debugGridRawArray( const double* in, int nI, int nJ, int nK ){
    spectral::array a( nI, nJ, nK, 0.0 );
    for(unsigned int k = 0; k < nK; ++k)
        for(unsigned int j = 0; j < nJ; ++j)
            for(unsigned int i = 0; i < nI; ++i){
                a(i, j, k) = in[ k * nJ * nI + j * nI + i ];
            }
    debugGrid( a );
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
