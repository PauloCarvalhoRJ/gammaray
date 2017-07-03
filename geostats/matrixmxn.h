#ifndef MATRIXMXN_H
#define MATRIXMXN_H

#include <vector>
#include <cmath>
#include "domain/application.h"

/** A generic N lines x M columns matrix template. */

template <class T>
class MatrixNXM
{
public:
    /** Constructor that initializes the matrix elements with a value. */
    MatrixNXM(unsigned int n, unsigned int m, T initValue = 0.0 );

    /** Returns the number of rows. */
    T getN(){ return _n; }

    /** Returns the number of columns. */
    T getM(){ return _m; }

    /** Operator () for l-value element access: e.g.: a(1,2) = 20.0; .
     * Due to performance concern, no range check is performed.
     */
    T& operator() (unsigned int i, unsigned int j){
      return _values[_m*i + j];
    }

    /** Operator () for r-value element access: e.g.: double x = a(2,3); .
      * Due to performance concern, no range check is performed.
      */
    T operator() (unsigned i, unsigned j) const {
        return _values[_m*i + j];
    }

    /** Matrix multiplication operator. It is assumed the operands are compatible (this._m == b._n).*/
    MatrixNXM<T> operator*(const MatrixNXM<T>& b);

    /** Inverts this matrix. It is assumed that the matrix is square, no check in this regard is performed, though
     * there is a check for singularity (prints a message and aborts calculation.)
     */
    void invert();

private:
    /** Number of rows. */
    unsigned int _n;
    /** Number of columns. */
    unsigned int _m;
    /** Values storage. */
    std::vector<T> _values;
};

/////////////////////////////////////////IMPLEMENTATIONS////////////////////////////////////

template <typename T>
MatrixNXM<T>::MatrixNXM(unsigned int n, unsigned int m, T initValue ) :
    _n(n),
    _m(m),
    _values( n*m, initValue )
{}

template <typename T>
void MatrixNXM<T>::invert(){
    /* This is a Gauss-Jordan method implemented from the code in Numerical Recipes, 3rd edition.
       It is intended to solve a linear system, but it was modified just perform invertion.
    */
    MatrixNXM<T> &a = *this;
    // the independent terms (right-hand side) vector is created with dummy values,
    // since we're not interested in solving a system.
    int i, icol, irow, j, k, l , ll, n = a.getN();
    double big, dum, pivinv;
    std::vector<int> indxc(n), indxr(n), ipiv(n); //index bookkeeping vectors
    ipiv.assign(n, 0);
    for ( i=0; i<n; ++i) {
        big = 0.0;
        for (j=0; j<n; ++j)
            if (ipiv[j] != 1)
                for (k=0; k<n; ++k) {
                    if (ipiv[k] == 0) {
                        if ( std::abs(a(j,k)) >= big) {
                            big = std::abs( a(j,k) );
                            irow = j;
                            icol = k;
                        }
                    }
                }
        ++(ipiv[icol]);
        if (irow != icol) {
            for (l=0; l<n; ++l)
                std::swap( a(irow,l), a(icol,l) );
        }
        indxr[i] = irow;
        indxc[i] = icol;
        if ( a(icol, icol) == 0.0 ){
            Application::instance()->logError("MatrixNXM<T>::invert(): Singular matrix.  Operation aborted.  Matrix values inconsistent.");
            return;
        }
        pivinv = 1.0 / a(icol,icol);
        a(icol, icol) = 1.0;
        for (l=0; l<n; ++l) a(icol,l) *= pivinv;
        for (ll=0; ll<n; ++ll)
            if (ll != icol) {
                dum=a(ll, icol);
                a(ll, icol) = 0.0;
                for (l=0;l<n;++l)
                    a(ll, l) -= a(icol, l) * dum;
            }
    }
    for (l=n-1; l>=0; --l) {
        if (indxr[l] != indxc[l])
            for ( k=0; k<n; ++k)
                std::swap( a(k,indxr[l]), a(k,indxc[l]) );
    }
}

//TODO: naive matrix multiplication, improve performance (e.g. parallel)
template <typename T>
MatrixNXM<T> MatrixNXM<T>::operator*(const MatrixNXM<T>& b) {
   MatrixNXM<T>& a = *this;
   MatrixNXM<T> result( a._n, b._m );
   for(int i = 0; i < a._n; ++i)
       for(int j = 0; j < b._m; ++j)
           for(int k = 0; k < a._m; ++k) //a._m (number of cols) is supposed to be == b._n (number of rows)
               result(i,j) += a(i,k) * b(k,j);
   return result;
}


#endif // MATRIXMXN_H
