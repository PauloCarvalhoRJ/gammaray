#include "imagejockeysvdutils.h"

ImageJockeySVDUtils::ImageJockeySVDUtils()
{
}


/*
Author: P?ricles Lopes Machado <pericles@newavesoft.com>
Year: 2017
*/

//#include "spectral.h"
//#include "svd.h"


//using SVDFactors = std::vector<spectral::array>;

//void computeSVDFactors(int n_factors, const spectral::array& a) {
//    spectral::SVD svd = spectral::svd(a);

//    for (long i = 0; i < n_factors; ++i) {
//        auto f = svd.factor(i);
//        renderFactor(f, i);
//    }

//    auto weights = svd.factor_weights();
//    renderDecayingCurve(weights);
//}


//spectral::array rebuildFromSVDFactors(const SVDFactors& factors) {
//    spectral::array backward_array(factors[0].M_, factors[0].N_, factors[0].K_, 0);

//    for (long i = 0; i < factors.size(); ++i) {
//        const auto& f = factors[i];
//        for (long k = 0; k < f.size(); ++k) {
//            backward_array[k] += f[k];
//        }
//    }

//    return backward_array;
//}

