#ifndef MATRIX3X3_H
#define MATRIX3X3_H

/** A 3x3 matrix template. This class contains matrix operations hardcoded for 3 x 3 matrices, which are
 * supposedly better optimized by the compiler than generic matrix algorithms.*/

template <class T>
class Matrix3X3
{
public:

    /** Default constructor creates an identity matrix. */
    Matrix3X3(){
        _a11 = 1.0; _a12 = 0.0; _a13 = 0.0;
        _a21 = 0.0; _a22 = 1.0; _a23 = 0.0;
        _a31 = 0.0; _a32 = 0.0; _a33 = 1.0;
    }

    /** Constructor assiging all elements. */
    Matrix3X3( T a11, T a12, T a13,
               T a21, T a22, T a23,
               T a31, T a32, T a33 ){
        _a11 = a11; _a12 = a12; _a13 = a13;
        _a21 = a21; _a22 = a22; _a23 = a23;
        _a31 = a31; _a32 = a32; _a33 = a33;
    }

    /** 3X3 matrix multiplication. */
    Matrix3X3 operator*(const Matrix3X3& b) {
       Matrix3X3 m3;
       m3._a11 = _a11*b._a11 + _a12*b._a21 + _a13*b._a31;
       m3._a12 = _a11*b._a12 + _a12*b._a22 + _a13*b._a32;
       m3._a13 = _a11*b._a13 + _a12*b._a23 + _a13*b._a33;
       m3._a21 = _a21*b._a11 + _a22*b._a21 + _a23*b._a31;
       m3._a22 = _a21*b._a12 + _a22*b._a22 + _a23*b._a32;
       m3._a23 = _a21*b._a13 + _a22*b._a23 + _a23*b._a33;
       m3._a31 = _a31*b._a11 + _a32*b._a21 + _a33*b._a31;
       m3._a32 = _a31*b._a12 + _a32*b._a22 + _a33*b._a32;
       m3._a33 = _a31*b._a13 + _a32*b._a23 + _a33*b._a33;
       return m3;
    }

    /** Returns the determinant of the matrix. */
    double det(){
        return _a11 * ( _a22 * _a33 - _a32 * _a23 ) -
               _a12 * ( _a21 * _a33 - _a23 * _a31 ) +
               _a13 * ( _a21 * _a32 - _a22 * _a31 );
    }

    /** Inverts this matrix. */
    void invert(){
        double determinant = det();

        double invdet = 1 / determinant;

        //store the results in temporary variables to avoid aliasing.
        double a11 = ( _a22 * _a33 - _a32 * _a23 ) * invdet;
        double a12 = ( _a13 * _a32 - _a12 * _a33 ) * invdet;
        double a13 = ( _a12 * _a23 - _a13 * _a22 ) * invdet;
        double a21 = ( _a23 * _a31 - _a21 * _a33 ) * invdet;
        double a22 = ( _a11 * _a33 - _a13 * _a31 ) * invdet;
        double a23 = ( _a21 * _a13 - _a11 * _a23 ) * invdet;
        double a31 = ( _a21 * _a32 - _a31 * _a22 ) * invdet;
        double a32 = ( _a31 * _a12 - _a11 * _a32 ) * invdet;
        double a33 = ( _a11 * _a22 - _a21 * _a12 ) * invdet;

        _a11 = a11; _a12 = a12; _a13 = a13;
        _a21 = a21; _a22 = a22; _a23 = a23;
        _a31 = a31; _a32 = a32; _a33 = a33;
    }

    T _a11; T _a12; T _a13;
    T _a21; T _a22; T _a23;
    T _a31; T _a32; T _a33;
};

#endif // MATRIX3X3_H
