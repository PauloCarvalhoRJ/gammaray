#ifndef WAVELETUTILS_H
#define WAVELETUTILS_H

#include "imagejockey/gabor/gaborutils.h"

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
     * @brief Performs Discrete Wavelet Transform on gridded data.
     * @param cg The grid object containing the data.
     * @param variableIndex The variable to be transformed.
     * @param waveletFamily The wavelet family.
     * @param waveletType The wavelet type of the selected family.
     * @param interleaved If true, a two-step 1D DWT is performed on rows and columns alternately,
     *                    otherwise, 1D DWT is performed first on rows, then on columns.
     */
    static void transform( IJAbstractCartesianGrid* cg,
                           int variableIndex,
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
};

#endif // WAVELETUTILS_H
