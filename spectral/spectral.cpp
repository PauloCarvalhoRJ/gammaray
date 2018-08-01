/*
Spectral primitives

(c) 2017, Péricles Lopes Machado

With contributions by Paulo R. M. Carvalho (paulo.r.m.carvalho@gmail.com)
*/

#include "spectral.h"
#include <cmath>
#include <Eigen/Dense>
#include <complex>

namespace spectral
{

complex_array::complex_array() : size_(0), ndim_(1), M_(1), d_(nullptr)
{
    d_ = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * 1);
}

complex_array::complex_array(index N) : size_(N), ndim_(1), M_(N), N_(1), K_(1)
{
    d_ = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
}

complex_array::complex_array(index M, index N)
    : size_(N * M), ndim_(2), M_(M), N_(N), K_(1)
{
    d_ = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N * M);
}

complex_array::complex_array(index M, index N, index K)
    : size_(N * M * K), ndim_(3), M_(M), N_(N), K_(K)
{
    d_ = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N * M * K);
}

complex_array::complex_array(fftw_array_raw d) : d_(d), size_(0) {}

complex_array::complex_array(const complex_array &other)
{
    ndim_ = other.ndim_;
    M_ = other.M_;
    N_ = other.N_;
    K_ = other.K_;
    size_ = M_ * N_ * K_;
    d_ = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * size_);
    for (index i = 0; i < size_; ++i) {
        d_[i][0] = other.d_[i][0];
        d_[i][1] = other.d_[i][1];
    }
}

complex_array::complex_array(complex_array &&other)
    : M_(other.M_), N_(other.N_), K_(other.K_), d_(other.d_), ndim_(other.ndim_)
{
    other.d_ = nullptr;
    other = nullptr;
}

complex_array::complex_array(const Eigen::MatrixXd &m) : M_(m.rows()), N_(m.cols()), K_(1)
{
    if (M_ < 1)
        M_ = 1;
    if (N_ < 1)
        N_ = 1;
    if (K_ < 1)
        K_ = 1;

    ndim_ = N_ > 1 ? 2 : 1;
    size_ = M_ * N_ * K_;
    d_ = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * size_);

    index idx = 0;
    for (index i = 0; i < M_; ++i) {
        for (index j = 0; j < N_; ++j) {
            d_[idx][0] = m(i, j);
            d_[idx][1] = 0;
            ++idx;
        }
    }
}

complex_array &complex_array::operator=(complex_array &&other)
{
    M_ = other.M_;
    N_ = other.N_;
    K_ = other.K_;
    ndim_ = other.ndim_;
    if (d_)
        fftw_free(d_);
    d_ = other.d_;
    other.d_ = nullptr;
    size_ = M_ * N_ * K_;
    return *this;
}

complex_array &complex_array::operator=(const complex_array &other)
{
    M_ = other.M_;
    N_ = other.N_;
    K_ = other.K_;
    ndim_ = other.ndim_;
    if (d_)
        fftw_free(d_);
    d_ = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N_ * M_ * K_);
    size_ = M_ * N_ * K_;
    for (index i = 0; i < size_; ++i) {
        d_[i][0] = other.d_[i][0];
        d_[i][1] = other.d_[i][1];
    }
    return *this;
}

complex_array::~complex_array()
{
    if (d_)
        fftw_free(d_);
}

index complex_array::idx(index i) const { return i; }

index complex_array::idx(index i, index j) const { return i * N_ + j; }

index complex_array::idx(index i, index j, index k) const
{
    return (i * N_ + j) * K_ + k;
}

fftw_complex &complex_array::operator()(index i) { return d_[i]; }

fftw_complex &complex_array::operator()(index i, index j) { return d_[i * N_ + j]; }

fftw_complex &complex_array::operator()(index i, index j, index k)
{
    return d_[(i * N_ + j) * K_ + k];
}

fftw_complex &complex_array::operator()(index i) const
{
    if (i < 0)
        i = (i + M_) % M_;

    return d_[i];
}

fftw_complex &complex_array::operator()(index i, index j) const
{

    if (i < 0)
        i = (i + M_) % M_;
    if (j < 0)
        j = (j + N_) % N_;

    return d_[i * N_ + j];
}

fftw_complex &complex_array::operator()(index i, index j, index k) const
{
    if (i < 0)
        i = (i + M_) % M_;
    if (j < 0)
        j = (j + N_) % N_;
    if (k < 0)
        k = (k + K_) % K_;

    return d_[(i * N_ + j) * K_ + k];
}

index complex_array::size() const { return size_; }

const double &complex_array::real(index i) const { return d_[i][0]; }

const double &complex_array::imag(index i) const { return d_[i][1]; }

double complex_array::norm(index i) const
{
    std::complex<double> v(real(i), imag(i));
    return std::norm(v);
}

double complex_array::arg(index i) const
{
    std::complex<double> v(real(i), imag(i));
    return std::arg(v);
}

std::complex<double> complex_array::complex(index i) const
{
    return std::complex<double>(real(i), imag(i));
}

fftw_complex &complex_array::operator[](index i) { return d_[i]; }

void complex_array::dot(const complex_array &a, const complex_array &other)
{
    for (index i = 0; i < size_; ++i) {
        d_[i][0] = a.real(i) * other.real(i) - a.imag(i) * other.imag(i);
        d_[i][1] = a.real(i) * other.imag(i) + a.imag(i) * other.real(i);
    }
}

void complex_array::dot_conj(const complex_array &a, const complex_array &b)
{
    for (index i = 0; i < size_; ++i) {
        d_[i][0] = a.real(i) * b.real(i) + a.imag(i) * b.imag(i);
        d_[i][1] = -a.real(i) * b.imag(i) + a.imag(i) * b.real(i);
    }
}

fftw_array_raw complex_array::data() { return d_; }

void complex_array::set_data(fftw_array_raw d, index M)
{
    ndim_ = 1;
    if (d_)
        fftw_free(d_);
    d_ = d;
    size_ = M;
    M_ = M;
    N_ = 1;
    K_ = 1;
}

void complex_array::set_data(fftw_array_raw d, index M, index N)
{
    ndim_ = 2;
    if (d_)
        fftw_free(d_);
    d_ = d;
    size_ = M * N;
    M_ = M;
    N_ = N;
    K_ = 1;
}

void complex_array::set_data(fftw_array_raw d, index M, index N, index K)
{
    ndim_ = 3;
    if (d_)
        fftw_free(d_);
    d_ = d;
    size_ = M * N * K;
    M_ = M;
    N_ = N;
    K_ = K;
}

void complex_array::set_size(index M, index N, index K)
{
    ndim_ = 3;
    if (d_)
        fftw_free(d_);
    size_ = M * N * K;
    M_ = M;
    N_ = N;
    K_ = K;
    d_ = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * size_);
}

void complex_array::set_size(index M, index N)
{
    ndim_ = 2;
    if (d_)
        fftw_free(d_);
    size_ = M * N;
    M_ = M;
    N_ = N;
    K_ = 1;
    d_ = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * size_);
}

void complex_array::set_size(index M)
{
    ndim_ = 1;
    if (d_)
        fftw_free(d_);
    size_ = M;
    M_ = M;
    N_ = 1;
    K_ = 1;
    d_ = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * size_);
}

array::array() : d_(), ndim_(1), M_(1), N_(1), K_(1) {}

array::array(index M, double default_value)
    : d_(M, default_value), ndim_(1), M_(M), N_(1), K_(1)
{
}

array::array(index M, index N, double default_value)
    : d_(M * N, default_value), ndim_(2), M_(M), N_(N), K_(1)
{
}

array::array(index M, index N, index K, double default_value)
    : d_(M * N * K, default_value), ndim_(3), M_(M), N_(N), K_(K)
{
}

array::array(const array &other) : d_(other.M_ * other.N_ * other.K_, 0)
{
    ndim_ = other.ndim_;
    M_ = other.M_;
    N_ = other.N_;
    K_ = other.K_;
    for (index i = 0; i < (index)other.d_.size(); ++i) {
        d_[i] = other[i];
    }
}

array::array(array &&other)
    : M_(other.M_), N_(other.N_), K_(other.K_), ndim_(other.ndim_),
      d_(std::move(other.d_))
{
}

array::array(const Eigen::MatrixXd &m)
    : M_(m.rows()), N_(m.cols()), K_(1), d_(m.rows() * m.cols())
{
    if (M_ < 1)
        M_ = 1;
    if (N_ < 1)
        N_ = 1;
    if (K_ < 1)
        K_ = 1;

    ndim_ = N_ > 1 ? 2 : 1;

    index idx = 0;
    for (index i = 0; i < M_; ++i) {
        for (index j = 0; j < N_; ++j) {
            d_[idx] = m(i, j);
            ++idx;
        }
    }
}

array &array::operator=(array &&other)
{
    ndim_ = other.ndim_;
    M_ = other.M_;
    N_ = other.N_;
    K_ = other.K_;
    d_ = std::move(other.d_);
    return *this;
}

array &array::operator=(const array &other)
{
    ndim_ = other.ndim_;
    M_ = other.M_;
    N_ = other.N_;
    K_ = other.K_;
    d_.resize(other.d_.size());
    for (index i = 0; i < other.d_.size(); ++i)
        d_[i] = other.d_[i];
    return *this;
}

array &array::operator+=(const array &other)
{
    for (index i = 0; i < other.d_.size(); ++i) {
        d_[i] += other.d_[i];
    }

    return *this;
}

array array::operator*(double scalar) const
{
    array result( M_, N_, K_ );
    for (index i = 0; i < d_.size(); ++i)
        result.d_[i] = d_[i] * scalar;
    return result;
}

array array::operator*(const array &other) const
{
    Eigen::MatrixXd tmpMe = to_2d( *this );
    Eigen::MatrixXd tmpOther = to_2d( other );
    return to_array( tmpMe * tmpOther );
}

array array::operator/(double scalar) const
{
	array result( M_, N_, K_ );
	for (index i = 0; i < d_.size(); ++i)
		result.d_[i] = d_[i] / scalar;
	return result;
}

array array::operator-(double scalar) const
{
	array result( M_, N_, K_ );
	for (index i = 0; i < d_.size(); ++i)
		result.d_[i] = d_[i] - scalar;
	return result;
}

array array::operator-(const array &other) const
{
    array result( M_, N_, K_ );
    for (index i = 0; i < d_.size(); ++i)
        result.d_[i] = d_[i] - other.d_[i];
	return result;
}

array array::getVectorColumn(index j) const
{
	array result( M_ );
	for (index i = 0; i < M_; ++i)
		result(i) = (*this)(i, j);
	return result;
}

array::~array() {}

double &array::operator()(index i, index j, index k)
{

    if (i < 0)
        i = (i + M_) % M_;
    if (j < 0)
        j = (j + N_) % N_;
    if (k < 0)
        k = (k + K_) % K_;

    return d_[(i * N_ + j) * K_ + k];
}

double &array::operator()(index i, index j)
{

    if (i < 0)
        i = (i + M_) % M_;
    if (j < 0)
        j = (j + N_) % N_;

    return d_[i * N_ + j];
}

double &array::operator()(index i)
{

    if (i < 0)
        i = (i + M_) % M_;

    return d_[i];
}

const double &array::operator()(index i, index j, index k) const
{

    if (i < 0)
        i = (i + M_) % M_;
    if (j < 0)
        j = (j + N_) % N_;
    if (k < 0)
        k = (k + K_) % K_;

    return d_.at((i * N_ + j) * K_ + k);
}

void array::set_size(index M, index N, index K)
{
    ndim_ = 3;
    M_ = M;
    N_ = N;
    K_ = K;
    d_.resize(M * N * K);
}

void array::set_size(index M, index N)
{
    ndim_ = 2;
    M_ = M;
    N_ = N;
    K_ = 1;
    d_.resize(M * N);
}

void array::set_size(index M)
{
    ndim_ = 1;
    M_ = M;
    N_ = 1;
    K_ = 1;
	d_.resize(M);
}

double array::max() const
{
	return *std::max_element( d_.begin(), d_.end() );
}

double array::min() const
{
	return *std::min_element( d_.begin(), d_.end() );
}

double array::euclideanLength() const
{
	return std::sqrt( spectral::dot( *this, *this ) );
}

const double &array::operator()(index i, index j) const { return d_.at(i * N_ + j); }

const double &array::operator()(index i) const { return d_.at(i); }

void foward(complex_array &out, double *in, index M)
{
    index out_fft_size = M / 2 + 1;
    fftw_array_raw out_fft
        = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * out_fft_size);
    fftw_plan p_fft;
    p_fft = fftw_plan_dft_r2c_1d(M, in, out_fft, FFTW_ESTIMATE_PATIENT);
    fftw_execute(p_fft);
    fftw_destroy_plan(p_fft);
    out.set_data(out_fft, M / 2 + 1);
}

void foward(complex_array &out, double *in, index M, index N)
{
    index out_fft_size = (N / 2 + 1) * M;
    fftw_array_raw out_fft
        = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * out_fft_size);
    fftw_plan p_fft;
    p_fft = fftw_plan_dft_r2c_2d(M, N, in, out_fft, FFTW_ESTIMATE_PATIENT);
    fftw_execute(p_fft);
    fftw_destroy_plan(p_fft);
    out.set_data(out_fft, M, N / 2 + 1);
}

void foward(complex_array &out, std::vector<double> &in, index M)
{
    foward(out, in.data(), M);
}

void foward(complex_array &out, std::vector<double> &in, index M, index N)
{
    foward(out, in.data(), M, N);
}

void foward(complex_array &out, double *in, index M, index N, index K)
{
    index out_fft_size = (K / 2 + 1) * N * M;
    fftw_array_raw out_fft
        = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * out_fft_size);
    fftw_plan p_fft;
    p_fft = fftw_plan_dft_r2c_3d(M, N, K, in, out_fft, FFTW_ESTIMATE_PATIENT);
    fftw_execute(p_fft);
    fftw_destroy_plan(p_fft);
    out.set_data(out_fft, M, N, K / 2 + 1);
}

void foward(complex_array &out, std::vector<double> &in, index M, index N, index K)
{
    foward(out, in.data(), M, N, K);
}

void foward(complex_array &out, array &in)
{
    if (in.ndim_ == 1)
        foward(out, in.d_, in.M_);
    else if (in.ndim_ == 2)
        foward(out, in.d_, in.M_, in.N_);
    else if (in.ndim_ == 3)
        foward(out, in.d_, in.M_, in.N_, in.K_);
}

void backward(std::vector<double> &out, complex_array &in, index M)
{
    fftw_plan p_fft;
    p_fft = fftw_plan_dft_c2r_1d(M, in.data(), out.data(), FFTW_ESTIMATE_PATIENT);
    fftw_execute(p_fft);
    fftw_destroy_plan(p_fft);
}

void backward(std::vector<double> &out, complex_array &in, index M, index N)
{
    fftw_plan p_fft;
    p_fft = fftw_plan_dft_c2r_2d(M, N, in.data(), out.data(), FFTW_ESTIMATE_PATIENT);
    fftw_execute(p_fft);
    fftw_destroy_plan(p_fft);
}

void backward(std::vector<double> &out, complex_array &in, index M, index N, index K)
{
    fftw_plan p_fft;
    p_fft = fftw_plan_dft_c2r_3d(M, N, K, in.data(), out.data(), FFTW_ESTIMATE_PATIENT);
    fftw_execute(p_fft);
    fftw_destroy_plan(p_fft);
}

void backward(array &out, complex_array &in)
{
    if (out.ndim_ == 1)
        backward(out.data(), in, out.M_);
    else if (out.ndim_ == 2)
        backward(out.data(), in, out.M_, out.N_);
    else if (out.ndim_ == 3)
        backward(out.data(), in, out.M_, out.N_, out.K_);
}

void conv1d(std::vector<double> &out, const std::vector<double> &a,
            const std::vector<double> &b)
{
    index K = a.size() + b.size() - 1;
    std::vector<double> ina(K, 0.0), inb(K, 0.0);
    complex_array A, B;

    for (index i = 0; i < a.size(); ++i) {
        ina[i] = (std::isinf(a[i]) || std::isnan(a[i])) ? 0 : a[i];
    }

    for (index i = 0; i < b.size(); ++i) {
        inb[i] = (std::isinf(b[i]) || std::isnan(b[i])) ? 0 : b[i];
    }

    foward(A, ina, ina.size());
    foward(B, inb, inb.size());

    complex_array C(A.size());
    C.dot(A, B);

    if (out.size() != K)
        out.resize(K);

    backward(out, C, K);

    for (index i = 0; i < out.size(); ++i) {
        out[i] /= K;
    }
}

void conv2d(array &out, const array &a, const array &b)
{
    index K1 = a.M_ + b.M_ - 1;
    index K2 = a.N_ + b.N_ - 1;
    index K = K1 * K2;

    array ina(K1, K2, 0.0), inb(K1, K2, 0.0);
    complex_array A, B;

    for (index i = 0; i < a.M_; ++i) {
        for (index j = 0; j < a.N_; ++j) {
            ina(i, j) = (std::isinf(a(i, j)) || std::isnan(a(i, j))) ? 0 : a(i, j);
        }
    }

    for (index i = 0; i < b.M_; ++i) {
        for (index j = 0; j < b.N_; ++j) {
            inb(i, j) = (std::isinf(b(i, j)) || std::isnan(b(i, j))) ? 0 : b(i, j);
        }
    }

    foward(A, ina);
    foward(B, inb);

    complex_array C(A.size());
    C.dot(A, B);

    if (out.size() != K)
        out.resize(K);

    out.ndim_ = 2;
    out.M_ = K1;
    out.N_ = K2;
    out.K_ = 1;

    backward(out, C);

    for (index i = 0; i < out.size(); ++i) {
        out[i] /= K;
    }
}

void conv3d(array &out, const array &a, const array &b)
{
    index K1 = a.M_ + b.M_ - 1;
    index K2 = a.N_ + b.N_ - 1;
    index K3 = a.K_ + b.K_ - 1;
    index K = K1 * K2 * K3;

    array ina(K1, K2, K3, 0.0), inb(K1, K2, K3, 0.0);
    complex_array A, B;

    for (index i = 0; i < a.M_; ++i) {
        for (index j = 0; j < a.N_; ++j) {
            for (index k = 0; k < a.K_; ++k) {
                ina(i, j, k)
                    = (std::isinf(a(i, j, k)) || std::isnan(a(i, j, k))) ? 0 : a(i, j, k);
            }
        }
    }

    for (index i = 0; i < b.M_; ++i) {
        for (index j = 0; j < b.N_; ++j) {
            for (index k = 0; k < b.K_; ++k) {
                inb(i, j, k)
                    = (std::isinf(b(i, j, k)) || std::isnan(b(i, j, k))) ? 0 : b(i, j, k);
            }
        }
    }

    foward(A, ina.d_, K1, K2, K3);
    foward(B, inb.d_, K1, K2, K3);

    complex_array C(A.size());
    C.dot(A, B);

    if (out.size() != K)
        out.resize(K);

    out.ndim_ = 3;
    out.M_ = K1;
    out.N_ = K2;
    out.K_ = K3;

    backward(out.d_, C, K1, K2, K3);

    for (index i = 0; i < out.size(); ++i) {
        out[i] /= K;
    }
}

void conv(array &out, const array &a, const array &b)
{
    if (a.ndim_ == 1)
        conv1d(out.d_, a.d_, b.d_);
    else if (a.ndim_ == 2)
        conv2d(out, a, b);
    else if (a.ndim_ == 3)
        conv3d(out, a, b);
}

void covariance1d(std::vector<double> &out, std::vector<double> &np,
                  const std::vector<double> &a, const std::vector<double> &b,
                  bool centered)
{
    index K = a.size() + b.size() - 1;
    std::vector<double> ina(K, 0.0), inb(K, 0.0), npa(K, 0.0), npb(K, 0.0);
    complex_array A, B, NPA, NPB;

    for (index i = 0; i < a.size(); ++i) {
        npa[i] = (std::isinf(a[i]) || std::isnan(a[i])) ? 0.0 : 1;
        ina[i] = (std::isinf(a[i]) || std::isnan(a[i])) ? 0.0 : a[i];
    }

    for (index i = 0; i < b.size(); ++i) {
        npb[i] = (std::isinf(b[i]) || std::isnan(b[i])) ? 0.0 : 1;
        inb[i] = (std::isinf(b[i]) || std::isnan(b[i])) ? 0.0 : b[i];
    }

    foward(NPA, npa, K);
    foward(NPB, npb, K);
    foward(A, ina, K);
    foward(B, inb, K);

    complex_array C(A.size()), NP(NPA.size());

    NP.dot_conj(NPB, NPA);
    C.dot_conj(B, A);

    if (out.size() != K)
        out.resize(K);
    if (np.size() != K)
        np.resize(K);

    backward(np, NP, K);
    backward(out, C, K);

    for (index i = 0; i < out.size(); ++i) {
        out[i] /= K;
        np[i] /= K;
        out[i] /= np[i];
    }

    if (!centered) {
        complex_array MA(A.size()), MB(A.size());
        std::vector<double> ma(K, 0), mb(K, 0);

        MA.dot_conj(NPB, A); // A * I
        MB.dot_conj(B, NPA); // B * I

        backward(ma, MA, K);
        backward(mb, MB, K);

        for (index i = 0; i < out.size(); ++i) {
            ma[i] /= K;
            mb[i] /= K;
            out[i] -= (ma[i] / np[i]) * (mb[i] / np[i]);
        }
    }
}

void covariance2d(array &out, array &np, const array &a, const array &b, bool centered)
{
    index K1 = a.M() + b.M() - 1;
    index K2 = a.N() + b.N() - 1;
    index K = K1 * K2;
    array ina(K1, K2, 0.0), inb(K1, K2, 0.0), npa(K1, K2, 0.0), npb(K1, K2, 0.0);
    complex_array A, B, NPA, NPB;

    for (index i = 0; i < a.M(); ++i) {
        for (index j = 0; j < a.N(); ++j) {
            npa(i, j) = (std::isinf(a(i, j)) || std::isnan(a(i, j))) ? 0.0 : 1;
            ina(i, j) = (std::isinf(a(i, j)) || std::isnan(a(i, j))) ? 0.0 : a(i, j);
        }
    }

    for (index i = 0; i < b.M(); ++i) {
        for (index j = 0; j < b.N(); ++j) {
            npb(i, j) = (std::isinf(b(i, j)) || std::isnan(b(i, j))) ? 0.0 : 1;
            inb(i, j) = (std::isinf(b(i, j)) || std::isnan(b(i, j))) ? 0.0 : b(i, j);
        }
    }

    foward(NPA, npa.d_, K1, K2);
    foward(NPB, npb.d_, K1, K2);
    foward(A, ina.d_, K1, K2);
    foward(B, inb.d_, K1, K2);

    complex_array C(A.size()), NP(NPA.size());

    NP.dot_conj(NPB, NPA);
    C.dot_conj(B, A);

    if (out.size() != K)
        out.resize(K);
    if (np.size() != K)
        np.resize(K);

    out.ndim_ = 2;
    out.M_ = K1;
    out.N_ = K2;
    out.K_ = 1;

    np.ndim_ = 2;
    np.M_ = K1;
    np.N_ = K2;
    np.K_ = 1;

    backward(np.data(), NP, K1, K2);
    backward(out.data(), C, K1, K2);

    for (index i = 0; i < out.size(); ++i) {
        out[i] /= K;
        np[i] /= K;
        out[i] /= np[i];
    }

    if (!centered) {
        complex_array MA(A.size()), MB(A.size());
        array ma(K1, K2, 0.0), mb(K1, K2, 0.0);

        MA.dot_conj(NPB, A); // A * I
        MB.dot_conj(B, NPA); // B * I

        backward(ma.data(), MA, K1, K2);
        backward(mb.data(), MB, K1, K2);

        for (index i = 0; i < out.size(); ++i) {
            ma[i] /= K;
            mb[i] /= K;
            out[i] -= (ma[i] / np[i]) * (mb[i] / np[i]);
        }
    }
}

void covariance3d(array &out, array &np, const array &a, const array &b, bool centered)
{
    index K1 = a.M() + b.M() - 1;
    index K2 = a.N() + b.N() - 1;
    index K3 = a.K() + b.K() - 1;
    index K = K1 * K2 * K3;
    array ina(K1, K2, K3, 0.0), inb(K1, K2, K3, 0.0), npa(K1, K2, K3, 0.0),
        npb(K1, K2, K3, 0.0);
    complex_array A, B, NPA, NPB;

    for (index i = 0; i < a.M(); ++i) {
        for (index j = 0; j < a.N(); ++j) {
            for (index k = 0; k < a.K(); ++k) {
                npa(i, j, k)
                    = (std::isinf(a(i, j, k)) || std::isnan(a(i, j, k))) ? 0.0 : 1;
                ina(i, j, k) = (std::isinf(a(i, j, k)) || std::isnan(a(i, j, k)))
                                   ? 0.0
                                   : a(i, j, k);
            }
        }
    }

    for (index i = 0; i < b.M(); ++i) {
        for (index j = 0; j < b.N(); ++j) {
            for (index k = 0; k < b.K(); ++k) {
                npb(i, j, k)
                    = (std::isinf(b(i, j, k)) || std::isnan(b(i, j, k))) ? 0.0 : 1;
                inb(i, j, k) = (std::isinf(b(i, j, k)) || std::isnan(b(i, j, k)))
                                   ? 0.0
                                   : b(i, j, k);
            }
        }
    }

    foward(NPA, npa.d_, K1, K2, K3);
    foward(NPB, npb.d_, K1, K2, K3);
    foward(A, ina.d_, K1, K2, K3);
    foward(B, inb.d_, K1, K2, K3);

    complex_array C(A.size()), NP(NPA.size());

    NP.dot_conj(NPB, NPA);
    C.dot_conj(B, A);

    if (out.size() != K)
        out.resize(K);
    if (np.size() != K)
        np.resize(K);

    out.ndim_ = 3;
    out.M_ = K1;
    out.N_ = K2;
    out.K_ = K3;

    np.ndim_ = 3;
    np.M_ = K1;
    np.N_ = K2;
    np.K_ = K3;

    backward(np.data(), NP, K1, K2, K3);
    backward(out.data(), C, K1, K2, K3);

    for (index i = 0; i < out.size(); ++i) {
        out[i] /= K;
        np[i] /= K;
        out[i] /= np[i];
    }

    if (!centered) {
        complex_array MA(A.size()), MB(A.size());
        array ma(K1, K2, K3, 0.0), mb(K1, K2, K3, 0.0);

        MA.dot_conj(NPB, A); // A * I
        MB.dot_conj(B, NPA); // B * I

        backward(ma.data(), MA, K1, K2, K3);
        backward(mb.data(), MB, K1, K2, K3);

        for (index i = 0; i < out.size(); ++i) {
            ma[i] /= K;
            mb[i] /= K;
            out[i] -= (ma[i] / np[i]) * (mb[i] / np[i]);
        }
    }
}

void covariance(array &out, array &np, const array &a, const array &b, bool centered)
{
    if (a.ndim_ == 1)
        covariance1d(out.d_, np.d_, a.d_, b.d_, centered);
    else if (a.ndim_ == 2)
        covariance2d(out, np, a, b, centered);
    else if (a.ndim_ == 3)
        covariance3d(out, np, a, b, centered);
}

void covariance1d_naive(std::vector<double> &out, std::vector<double> &np,
                        const std::vector<double> &a, const std::vector<double> &b,
                        bool centered)
{
    index K = a.size() + b.size() - 1;

    if (out.size() != K)
        out.resize(K);
    if (np.size() != K)
        np.resize(K);

    std::vector<double> ma(K), mb(K);

    for (index k = 0; k < b.size(); ++k) {
        out[k] = 0;
        np[k] = 0;
        ma[k] = 0;
        mb[k] = 0;

        for (index j = 0; j < a.size(); ++j) {
            if (k + j < b.size()) {
                out[k] += a[j] * b[k + j];
                ma[k] += a[j];
                mb[k] += b[k + j];
                ++np[k];
            }
        }
    }

    for (index l = K - 1; l >= b.size(); --l) {
        out[l] = 0;
        np[l] = 0;
        ma[l] = 0;
        mb[l] = 0;

        for (index j = 0; j < b.size(); ++j) {
            if (K - l + j < a.size()) {
                out[l] += a[K - l + j] * b[j];
                ma[l] += a[K - l + j];
                mb[l] += b[j];
                ++np[l];
            }
        }
    }

    for (index i = 0; i < out.size(); ++i) {
        out[i] /= np[i];
    }

    if (!centered) {
        for (index i = 0; i < out.size(); ++i) {
            out[i] -= (ma[i] / np[i]) * (mb[i] / np[i]);
        }
    }
}

void covariance2d_naive(array &out, array &np, const array &a, const array &b,
                        bool centered)
{
    index K1 = a.M() + b.M() - 1;
    index K2 = a.N() + b.N() - 1;

    out.ndim_ = 2;
    out.M_ = K1;
    out.N_ = K2;
    out.K_ = 1;

    np.ndim_ = 2;
    np.M_ = K1;
    np.N_ = K2;
    np.N_ = 1;

    out.resize(K1 * K2);
    np.resize(K1 * K2);

    array ma(K1, K2, 0.0), mb(K1, K2, 0.0);

    for (index k1 = 0; k1 < K1; ++k1) {
        for (index k2 = 0; k2 < K2; ++k2) {
            ma(k1, k2) = 0;
            mb(k1, k2) = 0;
            out(k1, k2) = 0;
            np(k1, k2) = 0;
        }
    }

    for (index i = 0; i < a.M(); ++i) {
        for (index j = 0; j < a.N(); ++j) {
            for (index m = 0; m < b.M(); ++m) {
                for (index n = 0; n < b.N(); ++n) {
                    index k1 = m - i;
                    index k2 = n - j;

                    if (k1 < 0) {
                        k1 = K1 + k1;
                    }

                    if (k2 < 0) {
                        k2 = K2 + k2;
                    }

                    out(k1, k2) += a(i, j) * b(m, n);
                    ++np(k1, k2);
                    ma(k1, k2) += a(i, j);
                    mb(k1, k2) += b(m, n);
                }
            }
        }
    }

    for (index i = 0; i < out.size(); ++i) {
        if (np[i] > 0)
            out[i] /= np[i];
    }

    if (!centered) {
        for (index i = 0; i < out.size(); ++i) {
            if (np[i] > 0)
                out[i] -= (ma[i] / np[i]) * (mb[i] / np[i]);
        }
    }
}

void covariance3d_naive(array &out, array &np, const array &a, const array &b,
                        bool centered)
{
    index K1 = a.M() + b.M() - 1;
    index K2 = a.N() + b.N() - 1;
    index K3 = a.K() + b.K() - 1;

    out.ndim_ = 3;
    out.M_ = K1;
    out.N_ = K2;
    out.K_ = K3;

    np.ndim_ = 3;
    np.M_ = K1;
    np.N_ = K2;
    np.K_ = K3;

    out.resize(K1 * K2 * K3);
    np.resize(K1 * K2 * K3);

    array ma(K1, K2, K3, 0.0), mb(K1, K2, K3, 0.0);

    for (index k1 = 0; k1 < K1; ++k1) {
        for (index k2 = 0; k2 < K2; ++k2) {
            for (index k3 = 0; k3 < K3; ++k3) {
                ma(k1, k2, k3) = 0;
                mb(k1, k2, k3) = 0;
                out(k1, k2, k3) = 0;
                np(k1, k2, k3) = 0;
            }
        }
    }

    for (index i = 0; i < a.M(); ++i) {
        for (index j = 0; j < a.N(); ++j) {
            for (index k = 0; k < a.K(); ++k) {

                for (index m = 0; m < b.M(); ++m) {
                    for (index n = 0; n < b.N(); ++n) {
                        for (index p = 0; p < b.K(); ++p) {
                            index k1 = m - i;
                            index k2 = n - j;
                            index k3 = p - k;

                            if (k1 < 0) {
                                k1 = K1 + k1;
                            }

                            if (k2 < 0) {
                                k2 = K2 + k2;
                            }

                            if (k3 < 0) {
                                k3 = K3 + k3;
                            }

                            out(k1, k2, k3) += a(i, j, k) * b(m, n, p);
                            ++np(k1, k2, k3);
                            ma(k1, k2, k3) += a(i, j, k);
                            mb(k1, k2, k3) += b(m, n, p);
                        }
                    }
                }
            }
        }
    }

    for (index i = 0; i < out.size(); ++i) {
        if (np[i] > 0)
            out[i] /= np[i];
    }

    if (!centered) {
        for (index i = 0; i < out.size(); ++i) {
            if (np[i] > 0)
                out[i] -= (ma[i] / np[i]) * (mb[i] / np[i]);
        }
    }
}

void conv1d_naive(std::vector<double> &out, const std::vector<double> &a,
                  const std::vector<double> &b)
{
    index K = a.size() + b.size() - 1;

    if (out.size() != K)
        out.resize(K);

    for (index k = 0; k < K; ++k) {
        out[k] = 0;
        for (index j = 0; j <= k; ++j) {
            out[k] += ((j < a.size()) && (k - j < b.size())) ? a[j] * b[k - j] : 0.0;
        }
    }
}

void conv2d_naive(array &out, const array &a, const array &b)
{
    index K1 = a.M_ + b.M_ - 1;
    index K2 = a.N_ + b.N_ - 1;
    index K = K1 * K2;

    if (out.size() != K)
        out.resize(K);

    out.ndim_ = 2;
    out.M_ = K1;
    out.N_ = K2;
    out.K_ = 1;

    for (index k1 = 0; k1 < K1; ++k1) {
        for (index k2 = 0; k2 < K2; ++k2) {
            double sum = 0;
            for (index i = 0; i <= k1; ++i) {
                for (index j = 0; j <= k2; ++j) {
                    if (i >= a.M_ || j >= a.N_)
                        continue;

                    if (k1 - i >= b.M_ || k2 - j >= b.N_)
                        continue;
                    if (k1 - i < 0 || k2 - j < 0)
                        continue;

                    sum += a(i, j) * b(k1 - i, k2 - j);
                }
            }
            out(k1, k2) = sum;
        }
    }
}

void conv3d_naive(array &out, const array &a, const array &b)
{
    index K1 = a.M_ + b.M_ - 1;
    index K2 = a.N_ + b.N_ - 1;
    index K3 = a.K_ + b.K_ - 1;
    index K = K1 * K2 * K3;

    if (out.size() != K)
        out.resize(K);

    out.ndim_ = 3;
    out.M_ = K1;
    out.N_ = K2;
    out.K_ = K3;

    for (index k1 = 0; k1 < K1; ++k1) {
        for (index k2 = 0; k2 < K2; ++k2) {
            for (index k3 = 0; k3 < K3; ++k3) {
                double sum = 0;
                for (index i = 0; i <= k1; ++i) {
                    for (index j = 0; j <= k2; ++j) {
                        for (index k = 0; k <= k3; ++k) {
                            if (i >= a.M_ || j >= a.N_ || k >= a.K_)
                                continue;

                            if (k1 - i >= b.M_ || k2 - j >= b.N_ || k3 - k >= b.K_)
                                continue;
                            if (k1 - i < 0 || k2 - j < 0 || k3 - k < 0)
                                continue;

                            sum += a(i, j, k) * b(k1 - i, k2 - j, k3 - k);
                        }
                    }
                }
                out(k1, k2, k3) = sum;
            }
        }
    }
}

void autocovariance(array &out, array &np, const array &a, bool centered)
{
    covariance(out, np, a, a, centered);
}

void autoconv(array &out, array &a) { conv(out, a, a); }

void covariance_naive(array &out, array &np, const array &a, const array &b,
                      bool centered)
{
    if (a.ndim_ == 1)
        covariance1d_naive(out.d_, np.d_, a.d_, b.d_, centered);
    else if (a.ndim_ == 2)
        covariance2d_naive(out, np, a, b, centered);
    else if (a.ndim_ == 3)
        covariance3d_naive(out, np, a, b, centered);
}

void autocovariance_naive(array &out, array &np, const array &a, bool centered)
{
    covariance_naive(out, np, a, a, centered);
}

void conv_naive(array &out, const array &a, const array &b)
{
    if (a.ndim_ == 1)
        conv1d_naive(out.d_, a.d_, b.d_);
    else if (a.ndim_ == 2)
        conv2d_naive(out, a, b);
    else if (a.ndim_ == 3)
        conv3d_naive(out, a, b);
}

void covariance(array &out, const array &a, const array &b, bool centered)
{
    array np;
    covariance(out, np, a, b, centered);
}

void autocovariance(array &out, const array &a, bool centered)
{
    covariance(out, a, a, centered);
}

void covariance_naive(array &out, const array &a, const array &b, bool centered)
{
    array np;
    covariance_naive(out, np, a, b, centered);
}

void autocovariance_naive(array &out, const array &a, bool centered)
{
    array np;
    covariance_naive(out, np, a, centered);
}

void normalize(complex_array &in, const std::complex<double> &K)
{
    for (index i = 0; i < in.size(); ++i) {
        std::complex<double> v(in(i)[0], in(i)[1]);
        auto res = v * K;
        in(i)[0] = res.real();
        in(i)[1] = res.imag();
    }
}

void normalize(complex_array &in, double K)
{
    for (index i = 0; i < in.size(); ++i) {
        in(i)[0] *= K;
        in(i)[1] *= K;
    }
}

void normalize(array &in, double K)
{
    for (index i = 0; i < in.size(); ++i) {
        in(i) *= K;
    }
}

void foward(complex_array &out, complex_array &in, index M)
{
    fftw_plan p;
    fftw_complex *fout = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * M);
    p = fftw_plan_dft_1d(M, in.data(), fout, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);
    out.set_data(fout, M);
}

void foward(complex_array &out, complex_array &in, index M, index N)
{
    fftw_plan p;
    fftw_complex *fout = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * M * N);
    p = fftw_plan_dft_2d(M, N, in.data(), fout, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);
    out.set_data(fout, M, N);
}

void foward(complex_array &out, complex_array &in, index M, index N, index K)
{
    fftw_plan p;
    fftw_complex *fout = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * M * N * K);
    p = fftw_plan_dft_3d(M, N, K, in.data(), fout, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);
    out.set_data(fout, M, N, K);
}

void foward(complex_array &out, complex_array &in)
{
    if (in.ndim() == 1)
        foward(out, in, in.M());
    else if (in.ndim() == 2)
        foward(out, in, in.M(), in.N());
    else if (in.ndim() == 3)
        foward(out, in, in.M(), in.N(), in.K());
}

void backward(complex_array &out, complex_array &in)
{
    if (in.ndim() == 1)
        backward(out, in, in.M());
    else if (in.ndim() == 2)
        backward(out, in, in.M(), in.N());
    else if (in.ndim() == 3)
        backward(out, in, in.M(), in.N(), in.K());
}

void backward(complex_array &out, complex_array &in, index M, index N, index K)
{
    fftw_plan p;
    fftw_complex *fout = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * M * N * K);
    p = fftw_plan_dft_3d(M, N, K, in.data(), fout, FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);
    out.set_data(fout, M, N, K);
}

void backward(complex_array &out, complex_array &in, index M, index N)
{
    fftw_plan p;
    fftw_complex *fout = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * M * N);
    p = fftw_plan_dft_2d(M, N, in.data(), fout, FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);
    out.set_data(fout, M, N);
}

void backward(complex_array &out, complex_array &in, index M)
{
    fftw_plan p;
    fftw_complex *fout = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * M);
    p = fftw_plan_dft_1d(M, in.data(), fout, FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);
    out.set_data(fout, M);
}

double mse(const array &A, const array &B)
{
    size_t M = A.M(), N = A.N(), K = A.K();
    if (M < 1)
        M = 1;
    if (N < 1)
        N = 1;
    if (K < 1)
        K = 1;

    double e = 0.0;
    double C = 1. / (M * N * K);

    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            for (size_t k = 0; k < K; ++k) {
                double dA = A(i, j, k) - B(i, j, k);
                e += C * dA * dA;
            }
        }
    }

    return e;
}

array to_array(const Eigen::MatrixXd &m)
{
    auto r = m.rows();
    auto c = m.cols();

    array M(r, c, 1, 0);

    for (size_t i = 0; i < r; ++i) {
        for (size_t j = 0; j < c; ++j) {
            M(i, j) = m(i, j);
        }
    }

    return M;
}

Eigen::MatrixXd to_2d(const array &A)
{
    auto M = A.M();
    auto N = A.N();
    auto K = A.K();

    if (M < 1)
        M = 1;
    if (N < 1)
        N = 1;
    if (K < 1)
        K = 1;

    auto m = M;
    auto n = N * K;

    Eigen::MatrixXd a(m, n);

    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            for (size_t k = 0; k < K; ++k) {
                a(i, j * K + k) = A(i, j, k);
            }
        }
    }

    return a;
}

double absolute_error(const array &A, const array &B)
{
    size_t M = A.M(), N = A.N(), K = A.K();
    if (M < 1)
        M = 1;
    if (N < 1)
        N = 1;
    if (K < 1)
        K = 1;
    double e = 0.0;
    double C = 1. / (M * N * K);

    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            for (size_t k = 0; k < K; ++k) {
                double dA = A(i, j, k) - B(i, j, k);
                e += C * std::fabs(dA);
            }
        }
    }

    return e;
}

array to_array(const Eigen::MatrixXd &m, index M, index N, index K)
{
    auto r = m.rows();
    auto c = m.cols();

    array a(M, N, K, 0);

    for (size_t i = 0; i < r; ++i) {
        for (size_t j = 0; j < c; ++j) {
            a(i, j / K, j % K) = m(i, j);
        }
    }

    return a;
}

array real(const complex_array &in)
{
    index M = in.M();
    index N = in.N();
    index K = in.K();
    array a(M, N, K, 0);

    for (index i = 0; i < in.size(); ++i) {
        a(i) = in(i)[0];
    }

    return a;
}

array imag(const complex_array &in)
{
    index M = in.M();
    index N = in.N();
    index K = in.K();
    array a(M, N, K, 0);

    for (index i = 0; i < in.size(); ++i) {
        a(i) = in(i)[1];
    }

    return a;
}

complex_array to_complex_array(const array &in)
{
    index M = in.M();
    index N = in.N();
    index K = in.K();
    complex_array a(M, N, K);

    for (index i = 0; i < in.size(); ++i) {
        a(i)[0] = in(i);
        a(i)[1] = 0;
    }

    return a;
}

complex_array to_complex_array(const array &in, double scale)
{
    index M = in.M();
    index N = in.N();
    index K = in.K();
    complex_array a(M, N, K);

    for (index i = 0; i < in.size(); ++i) {
        a(i)[0] = in(i)*scale;
        a(i)[1] = 0;
    }

	return a;
}

void print(const array & A)
{
	Eigen::MatrixXd tmp = spectral::to_2d( A );
    std::cout << "Here is the matrix:\n" << tmp << std::endl;
}

array shiftByHalf(const array &in)
{
    int nI = in.M();
    int nJ = in.N();
    int nK = in.K();
    array result( (index)nI, (index)nJ, (index)nK );
    for (size_t i = 0; i < nI; ++i) {
        int i_shift = (i + nI/2) % nI;
        for (size_t j = 0; j < nJ; ++j) {
            int j_shift = (j + nJ/2) % nJ;
            for (size_t k = 0; k < nK; ++k) {
                int k_shift = (k + nK/2) % nK;
                result(i_shift, j_shift, k_shift) = in(i, j, k);
            }
        }
    }
    return result;
}

double sumOfAbsDifference(const array &one, const array &other)
{
    double result = 0.0;
    for( int i = 0; i < one.size(); ++i )
        result += std::abs( one.d_[i] - other.d_[i] );
    return result;
}

array operator-(double theValue, const array & theArray){
	array result( theArray.M(), theArray.N(), theArray.K() );
	for( int i = 0; i < theArray.size(); ++i )
		result.d_[i] = theValue - theArray.d_[i];
	return result;
}

void standardize(array &in)
{
	double min = in.min();
	in = in - min;
	double max = in.max();
	in = in / max;
}

double dot(const array & one, const array & other)
{
	double result = 0;
	for( int i = 0; i < one.size(); ++i )
		result += one.d_[i] * other.d_[i];
	return result;
}

double angle(const array & one, const array & other)
{
	double dot = spectral::dot( one, other );
	double mags_sqr = one.euclideanLength() * other.euclideanLength();
	if( std::abs(mags_sqr) < 0.000001 ) //the the mags squared is too small, consider it zero.
		return 0.0;
	double argument = dot / mags_sqr;
	argument = ( argument < -1.0 ? -1.0 : ( argument > 1.0 ? 1.0 : argument ) ); //avoids domain errors when calling acos()
	return std::acos( argument );
}

array hadamard(const array &one, const array &other)
{
    array result( one.M(), one.N(), one.K() );
    for( int i = 0; i < one.size(); ++i )
        result.d_[i] = one.d_[i] * other.d_[i];
    return result;
}

array joinColumnVectors(const std::vector<const array *> &columnVectors)
{
    // Convert the spectral::array's to Eigen matrices.
    std::vector<Eigen::MatrixXd> columnVectorsAsEigenMatrices;
    {
        std::vector<const array *>::const_iterator it = columnVectors.cbegin();
        for(; it != columnVectors.cend(); ++it){
            columnVectorsAsEigenMatrices.push_back( spectral::to_2d(**it) );
        }
    }

    // Instantiate the resulting matrix as Eigen matrix.
    //  Use the size of the first vector to define the number of rows in the
    //  resulting matrix.
    Eigen::MatrixXd result( columnVectors[0]->M(), columnVectors.size() );

    // Perform the join.
    std::vector<Eigen::MatrixXd>::iterator it = columnVectorsAsEigenMatrices.begin();
    for( int iCol = 0; it != columnVectorsAsEigenMatrices.cend(); ++it, ++iCol){
        result.col( iCol ) << *it;
    }

    return spectral::to_array( result );
}

array transpose( const array &input )
{
    Eigen::MatrixXd temp = to_2d( input );
    Eigen::MatrixXd tempT = temp.transpose();
    return to_array( tempT );
}

array inv(const array &input)
{
    Eigen::MatrixXd temp = to_2d( input );
    return to_array( temp.inverse() );
}

std::pair<array, array> eig(const array &input)
{
	Eigen::MatrixXd temp = to_2d( input );
	Eigen::EigenSolver< Eigen::MatrixXd > eigensolver(temp);
//	if (eigensolver.info() != Eigen::Success)
//		abort();
	Eigen::MatrixXd eigenvectors = eigensolver.eigenvectors().real(); //EigenSolver yields complex matrices, even if the result are real.
	Eigen::MatrixXd eigenvalues = eigensolver.eigenvalues().real();
	return { to_array(eigenvectors), to_array(eigenvalues) };
}

complex_array to_complex_array(const array & A, const array & B)
{
	index M = A.M();
	index N = A.N();
	index K = A.K();
	complex_array a(M, N, K);

	for (index i = 0; i < A.size(); ++i) {
		a(i)[0] = A(i);
		a(i)[1] = B(i);
	}

	return a;
}

complex_array to_polar_form(const complex_array & in)
{
	std::complex<double> value;
	index M = in.M();
	index N = in.N();
	index K = in.K();
	complex_array out(M, N, K);
	for (index i = 0; i < in.size(); ++i) {
		value.real( in(i)[0] ); //real part
		value.imag( in(i)[1] ); //imaginary part
		out(i)[0] = std::abs( value ); //get magnitude
		out(i)[1] = std::arg( value ); //get phase
	}
	return out;
}

complex_array to_rectangular_form(const complex_array & in)
{
	std::complex<double> value;
	index M = in.M();
	index N = in.N();
	index K = in.K();
	complex_array out(M, N, K);
	for (index i = 0; i < in.size(); ++i) {
		value = std::polar( in(i)[0], in(i)[1] );
		out(i)[0] = value.real();
		out(i)[1] = value.imag();
	}
	return out;
}

array operator*(double theValue, const array & theArray)
{
	array result( theArray.M(), theArray.N(), theArray.K() );
	for( int i = 0; i < theArray.size(); ++i )
		result.d_[i] = theValue * theArray.d_[i];
	return result;
}

} // namespace spectral

