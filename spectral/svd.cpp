/*
Author: Pericles Lopes Machado <pericles@newavesoft.com>
Year: 2017
*/
#include "svd.h"

#include <Eigen/Dense>
#include <Eigen/SVD>

namespace spectral
{

SVD::SVD(const array &A, array &&U, array &&S, array &&V)
    : U_(std::move(U)), S_(std::move(S)), V_(std::move(V)), M_(A.M()), N_(A.N()),
      K_(A.K()), A_(A)
{
    if (M_ < 1)
        M_ = 1;
    if (N_ < 1)
        N_ = 1;
    if (K_ < 1)
        K_ = 1;
}

array &SVD::U() { return U_; }

array &SVD::S() { return S_; }

array &SVD::V() { return V_; }

const array &SVD::A() const { return A_; }

size_t SVD::n_factors() { return S_.M(); }

void SVD::factor(array &f, size_t i)
{
    if (i < S_.M()) {
        Eigen::MatrixXd u = to_2d(U_);

        Eigen::MatrixXd s = Eigen::MatrixXd::Zero(M_, N_ * K_);

        s(i, i) = S_(0, i);

        Eigen::MatrixXd v = to_2d(V_).transpose();
        Eigen::MatrixXd c = u * s * v;

        for (size_t m = 0; m < M_; ++m) {
            for (size_t n = 0; n < N_; ++n) {
                for (size_t k = 0; k < K_; ++k) {
                    f(m, n, k) += c(m, n * K_ + k);
                }
            }
        }
    }
}

array SVD::factor(size_t i)
{
    array f(M_, N_, K_, 0);

    factor(f, i);

    return f;
}

array SVD::factor(size_t start, size_t end)
{
    array f(M_, N_, K_, 0);

    for (size_t i = start; i < end; ++i) {
        if (i < S_.M()) {
            factor(f, i);
        }
    }

    return f;
}

array SVD::pca()
{
    array p(M_, N_, K_, 0);
    array rhs(U_.M(), S_.M(), 1, 0);

    for (index i = 0; i < U_.M(); ++i) {
        for (index j = 0; j < S_.M(); ++j) {
            if (j < S_.M()) {
                rhs(i, j) = U_(i, j) * S_(j, 0);
            }
        }
    }

    for (index i = 0; i < M_; ++i) {
        index idx = 0;
        for (index j = 0; j < N_; ++j) {
            for (index k = 0; k < K_; ++k) {
                p(i, j, k) = rhs(i, idx);
                ++idx;
            }
        }
    }

    return p;
}

array SVD::pca_inv(const array &C)
{
    auto v = to_2d(V_);
    auto v_inv = v.inverse();
    auto c = to_2d(C);
    auto a = c * v_inv;
    array b(a);
    return b;
}

array SVD::factor_weights()
{
    double T = 0;

    for (size_t i = 0; i < S_.M(); ++i) {
        T += S_(i);
    }

    array w(S_.M(), 1, 1, 0);

    for (size_t i = 0; i < S_.M(); ++i) {
        w(i) = S_(i) / T;
    }

    return w;
}

SVD SVD::compute(const array &A) { return svd(A); }

array SVD::solve(const array &A, const array &b) { return svd_lsq_solve(A, b); }

SVD svd(const array &A)
{
    Eigen::MatrixXd a = to_2d(A);
    Eigen::BDCSVD<Eigen::MatrixXd> svd(a, Eigen::ComputeFullU | Eigen::ComputeFullV);

    auto S = to_array(svd.singularValues());
    auto U = to_array(svd.matrixU());
    auto V = to_array(svd.matrixV());

    return SVD(A, std::move(U), std::move(S), std::move(V));
}

array svd_lsq_solve(const array &A, const array &B)
{
    auto a = to_2d(A);
    auto b = to_2d(B);

    Eigen::BDCSVD<Eigen::MatrixXd> svd(a, Eigen::ComputeFullU | Eigen::ComputeFullV);
    auto c = svd.solve(b);

    return array(c);
}

} // namespace spectral

