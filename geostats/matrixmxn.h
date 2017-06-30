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
    /** Simplest constructor. */
    MatrixNXM(unsigned int n, unsigned int m);

    /** Constructor that initializes the matrix elements with a value. */
    MatrixNXM(unsigned int n, unsigned int m, T initValue );

    /** Returns the number of rows. */
    T getN(){ return _n; }

    /** Returns the number of columns. */
    T getM(){ return _m; }

    /** Operator () for l-value element access: e.g.: a(1,2) = 20.0; . */
    T& operator() (unsigned int i, unsigned int j){
      return _values[_n*i + j];
    }

    /** Operator () for r-value element access: e.g.: double x = a(2,3); . */
    T operator() (unsigned i, unsigned j) const {
        return _values[_n*i + j];
    }

    /** Inverts this matrix. */
    void invert();

private:
    /** Number of lines. */
    unsigned int _n;
    /** Number of columns. */
    unsigned int _m;
    /** Values storage. */
    std::vector<T> _values;
};

/////////////////////////////////////////IMPLEMENTATIONS////////////////////////////////////

template <typename T>
MatrixNXM<T>::MatrixNXM(unsigned int n, unsigned int m) :
    _n(n),
    _m(m)
{
    _values.reserve( n * m );
}

template <typename T>
MatrixNXM<T>::MatrixNXM(unsigned int n, unsigned int m, T initValue )
{
    MatrixNXM( n, m );
    _values.assign( n * m, initValue );
}

template <typename T>
void MatrixNXM<T>::invert(){
    /* This is a Gauss-Jordan method implemented from the code in Numerical Recipes, 3rd edition.
       It is intended to solve a linear system, but we'll use it to invert this matrix.
       the unknowns matrix will be this array. */
    MatrixNXM<T> &a = *this;
    // the independent terms (right-hand side) vector is created with dummy values,
    // since we're not interested in solving a system.
    //MatrixNXM<T> b( a._n, 1, 0.0 );
    int i, icol, irow, j, k, l , ll, n = a.getN()/*, m = b.getM()*/;
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
            for (l=0; l<n; ++l) std::swap( a(irow,l), a(icol,l) );
            //for (l=0; l<m; ++l) std::swap( b(irow,l), b(icol,l) );
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
        //for (l=0; l<m; ++l) b(icol,l) *= pivinv;
        for (ll=0; ll<n; ++ll)
            if (ll != icol) {
                dum=a(ll, icol);
                a(ll, icol) = 0.0;
                for (l=0;l<n;++l) a(ll, l) -= a(icol, l) * dum;
                //for (l=0;l<m;++l) b(ll, l) -= b(icol, l) * dum;
            }
    }
    for (l=n-1; l>=0; --l) {
        if (indxr[l] != indxc[l])
            for ( k=0; k<n; ++k)
                std::swap( a(k,indxr[l]), a(k,indxc[l]) );
    }
}


#endif // MATRIXMXN_H
