#include "waveletutils.h"

#include "imagejockey/ijabstractcartesiangrid.h"

#include <gsl/gsl_wavelet2d.h>

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
    int nPowerOf2 = 1;
    while( nPowerOf2 < nMax )
        nPowerOf2 << 1;

    int n = nPowerOf2;

    int nc = 20;

//    //the input data in raw format used by GSL
//    double *data = malloc (n * sizeof (double));

//    double *abscoeff = malloc (n * sizeof (double));

//    size_t *p = malloc (n * sizeof (size_t));

//    FILE * f;
//    gsl_wavelet *w;
//    gsl_wavelet_workspace *work;

//    w = gsl_wavelet_alloc (gsl_wavelet_daubechies, 4);
//    work = gsl_wavelet_workspace_alloc (n);

//    f = fopen (argv[1], "r");
//    for (i = 0; i < n; i++)
//    {
//        fscanf (f, "%lg", &data[i]);
//    }
//    fclose (f);

//    gsl_wavelet_transform_forward (w, data, 1, n, work);

//    for (i = 0; i < n; i++)
//    {
//        abscoeff[i] = fabs (data[i]);
//    }

//    gsl_sort_index (p, abscoeff, 1, n);

//    for (i = 0; (i + nc) < n; i++)
//        data[p[i]] = 0;

//    gsl_wavelet_transform_inverse (w, data, 1, n, work);

//    for (i = 0; i < n; i++)
//    {
//        printf ("%g\n", data[i]);
//    }

//    gsl_wavelet_free (w);
//    gsl_wavelet_workspace_free (work);

//    free (data);
//    free (abscoeff);
//    free (p);
//    return 0;
}
