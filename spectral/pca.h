/*
Author: Pericles Lopes Machado <pericles@newavesoft.com>
Year: 2017
*/

#pragma once

#include "svd.h"


namespace spectral
{

class PCA
{
public:
    PCA(const array &A);
    array forward(const array &X);
    array backward(const array &X);

private:
    Eigen::MatrixXd V_;
    Eigen::MatrixXd V_inv_;
};

} // namespace spectral
