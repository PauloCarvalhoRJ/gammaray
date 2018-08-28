#ifndef MATRIXMXN_H
#define MATRIXMXN_H

#include <vector>
#include <cmath>
#include <qglobal.h> //for uint
#include <cassert>

namespace spectral {
	class array;
}

/** A generic N lines x M columns matrix template. */
template <class T>
class MatrixNXM
{
public:
    /** Constructor that initializes the matrix elements with a value. */
    MatrixNXM(unsigned int n, unsigned int m, T initValue = 0.0 );

	/** Constructor that initializes the matrix from the values in a spectral::array object. */
	MatrixNXM( const spectral::array& array );

    /** Returns the number of rows. */
	T getN() const { return _n; }

    /** Returns the number of columns. */
	T getM() const { return _m; }

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
	void invertWithGaussJordan();

	/** Inverts this matrix with Singular Values Decomposition (works with non-square matrices).
	 */
	void invertWithSVD();

	/** Returns a new matrix that is a transpose of this matrix.*/
	MatrixNXM<T> getTranspose( ) const;

	/** Returns a spectral::array object equivalent to this matrix. */
	spectral::array toSpectralArray() const;

	/** Matrix subtraction operator. It is assumed the operands are compatible (this._n == b._n && this._m == b._m).*/
	MatrixNXM<T> operator-(const MatrixNXM<T>& b) const;

	/** Zeroes all elements of the matrix and sets the elements in the main diagonal to one. */
	void setIdentity();

	/** Prints the matrix contents to std::out. Useful for debugging. */
	void print() const;

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

//TODO: naive matrix multiplication, improve performance (e.g. parallel) or use spectral::'s classes/methods
template <typename T>
MatrixNXM<T> MatrixNXM<T>::operator*(const MatrixNXM<T>& b) {
   MatrixNXM<T>& a = *this;
   assert( a._m == b._n && "MatrixNXM<T> MatrixNXM<T>::operator*: operands are matrices incompatible for multiplication." );
   MatrixNXM<T> result( a._n, b._m );
   for(uint i = 0; i < a._n; ++i)
       for(uint j = 0; j < b._m; ++j)
           for(uint k = 0; k < a._m; ++k) //a._m (number of cols) is supposed to be == b._n (number of rows)
               result(i,j) += a(i,k) * b(k,j);
   return result;
}

template <typename T>
MatrixNXM<T> MatrixNXM<T>::operator-(const MatrixNXM<T>& b) const{
	const MatrixNXM<T>& a = *this;
	assert( a._m == b._m && a._n == b._n && "MatrixNXM<T> MatrixNXM<T>::operator-: operands are matrices incompatible for subtraction." );
	MatrixNXM<T> result( a._n, a._m );
	for(uint i = 0; i < a._n; ++i)
		for(uint j = 0; j < a._m; ++j)
			result(i,j) += a(i,j) - b(i,j);
	return result;
}

template <typename T>
void MatrixNXM<T>::setIdentity() {
	MatrixNXM<T>& a = *this;
	for(uint i = 0; i < a._n; ++i)
		for(uint j = 0; j < a._m; ++j)
			if( i == j )
				a(i,j) = 1.0;
			else
				a(i,j) = 0.0;
}


#endif // MATRIXMXN_H
