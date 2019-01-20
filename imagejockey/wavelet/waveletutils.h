#ifndef WAVELETUTILS_H
#define WAVELETUTILS_H

#include "imagejockey/gabor/gaborutils.h"

class IJAbstractCartesianGrid;

namespace spectral{
    class array;
}

class WaveletUtils
{
public:
    WaveletUtils();

    static void transform( IJAbstractCartesianGrid* cg, int variableIndex );

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
