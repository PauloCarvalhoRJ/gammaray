/*
Author: Pericles Lopes Machado <pericles@newavesoft.com>
Year: 2017
*/

#pragma once

#include "spectral.h"

namespace spectral
{

class SVD
{
public:
    SVD(const array &A, array &&U, array &&S, array &&V);

    array &U();
    array &S();
    array &V();
    const array &A() const;

    size_t n_factors();

    void factor(array &f, size_t i);
    array factor(size_t i);
    // get factor sum in interval [start, end)
    array factor(size_t start, size_t end);

    array pca();
    array pca_inv(const array &C);

    array factor_weights();

    static SVD compute(const array &A);
    static array solve(const array &A, const array &b);

private:
    array U_;
    array S_;
    array V_;

    size_t M_;
    size_t N_;
    size_t K_;

    const array &A_;
};

SVD svd(const array &A);
array svd_lsq_solve(const array &A, const array &b);

} // namespace spectral

