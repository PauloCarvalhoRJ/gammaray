#ifndef WAVELETUTILS_H
#define WAVELETUTILS_H

#include "imagejockey/gabor/gaborutils.h"
#include <gsl/gsl_wavelet2d.h>

class IJAbstractCartesianGrid;

namespace spectral{
    class array;
}

enum class WaveletFamily : int {
    DAUBECHIES,
    HAAR,
    B_SPLINE,
    UNKNOWN
};

class WaveletUtils
{
public:
    WaveletUtils();

    /**
     * Performs 2D Discrete Wavelet Transform on gridded data.
     * IMPORTANT: the returned grid is a square grid with a power of two size,
     *            for example: if the input grid is 120x100, the returned grid
     *            is 128x128.  This is a requirement for GSL DWT operation.
     *            The input grid is aligned with the top-left corner of the power-
     *            -of-two grid and the excess cells are filled by mirror-padding
     *            to avoid the introduction of broad band artifacts in the transform.
     * @param cg The grid object containing the data.
     * @param variableIndex The variable to be transformed.
     * @param waveletFamily The wavelet family (see WaveletFamily enum for valid wavelt families).
     * @param waveletType The wavelet type of the selected family  (see WaveletTransformDialog::onWaveletFamilySelected()
     *                      for valid type values ).
     * @param interleaved If true, a two-step 1D DWT is performed on rows and columns alternately,
     *                    otherwise, 1D DWT is performed first on rows, then on columns.
     */
    static spectral::array transform( IJAbstractCartesianGrid* cg,
                                      int variableIndex,
                                      WaveletFamily waveletFamily,
                                      int waveletType,
                                      bool interleaved );

    static spectral::array backtrans( IJAbstractCartesianGrid *gridWithOriginalGeometry,
                                      const spectral::array& input,
                                      WaveletFamily waveletFamily,
                                      int waveletType,
                                      bool interleaved );

    /**
     * Does an inplace inverse 1D DWT in a raw double array. The result is put back in the data array.
     * @param input/output data 1D data series as a raw vector of doubles.
     * @param nLog2nData The number of elements in data expressed as log2(n).  Ex.: 4 means 16 elements in data.
     * @param waveletFamily See backtrans() for grids.
     * @param waveletType See backtrans() for grids.
     */
    static void backtrans( double* data,
                           int nLog2nData,
                           WaveletFamily waveletFamily,
                           int waveletType);


    /**
     * Fills the passed array of doubles with the values in the passed
     * image object.  The client code is responsible for correct allocation
     * of the memory needed by the array. That is, memory should be enough
     * for the number of values in the passed input image.
     */
    static void fillRawArray( const GaborUtils::ImageTypePtr input, double* output );

    /**
     * Creates a new image object that is the smallest square image with power-of-2
     * dimensions.  The contents of the input image are placed in the top left corner and
     * the excess pixels are mirror padded.
     * The power-of-2 dimension of the output image is stored in the output parameter nPowerOf2.
     */
    static GaborUtils::ImageTypePtr squareAndMirrorPad(const GaborUtils::ImageTypePtr input,
                                                        int& nPowerOf2 );

private:
    static void debugGridITK( const GaborUtils::ImageType &in);
    static void debugGrid( const spectral::array &grid );
    static void debugGridRawArray(const double *in, int nI, int nJ, int nK);
    //the returned structure must be deleted with gsl_wavelet_free().
    static gsl_wavelet* makeWavelet( WaveletFamily waveletFamily , int waveletType );
};

#endif // WAVELETUTILS_H
