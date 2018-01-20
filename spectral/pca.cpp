/*
Author: Pericles Lopes Machado <pericles@newavesoft.com>
Year: 2017
*/

#include "pca.h"
#include <Eigen/Dense>

namespace spectral
{

PCA::PCA(const array &A)
{
    SVD s = svd(A);
    V_ = to_2d(s.V());
    V_inv_ = V_.inverse();
}

array PCA::forward(const array &X)
{
    auto x = to_2d(X);
    auto pca = x * V_;
    array a(pca);
    return a;
}

array PCA::backward(const array &X)
{
    auto x = to_2d(X);
    auto pca = x * V_inv_;
    array a(pca);
    return a;
}

} // namespace spectral

