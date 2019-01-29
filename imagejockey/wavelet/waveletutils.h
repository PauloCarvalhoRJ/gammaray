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
     * Performs Discrete Wavelet Transform on gridded data.
     * IMPORTANT: the returned grid is a square grid with a power of two size,
     *            for example: if the input grid is 120x100, the returned grid
     *            is 128x128.  This is a requirement for GSL DWT operation.
     *            The input grid is aligned with the top-left corner of the power-
     *            -of-two grid and the excess cells are filled by mirror-padding
     *            to avoid the introduction of broad band artifacts in the transform.
     * @param cg The grid object containing the data.
     * @param variableIndex The variable to be transformed.
     * @param waveletFamily The wavelet family.
     * @param waveletType The wavelet type of the selected family.
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
     * Fills the passed array of doubles with the values in the passed
     * image object.  The client code is responsible for correct allocation
     * of the memory needed by the array. That is, memory should be enough
     * for the number of values in the passed input image.
     */
    static void fillRawArray( const GaborUtils::ImageTypePtr input, double* output );

private:
    static void debugGridITK( const GaborUtils::ImageType &in);
    static void debugGrid( const spectral::array &grid );
    static void debugGridRawArray(const double *in, int nI, int nJ, int nK);
    //the returned structure must be deleted with gsl_wavelet_free().
    static gsl_wavelet* makeWavelet( WaveletFamily waveletFamily , int waveletType );
};

#endif // WAVELETUTILS_H
