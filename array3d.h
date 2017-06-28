#ifndef ARRAY3D_H
#define ARRAY3D_H

#include <vector>
#include <cstdlib>
#include <stdexcept>

/**
 * The Array3D class is a generic array of any object type with rank 3 (3 dimensions, IJK).
 * It wraps an unidimensional array, hiding index arithmetic, allowing a more readable code (e.g. a(i,j,k))
 * while preserving memory locality, leading to better performance (e.g. more cache hits), since multidimensional
 * arrays tend to scatter in memory.
 * The Array3D can be used as a two-dimensional array in code (e.g. a(i,j) assumes k = 0)
 */
template <class TYPE>
class Array3D
{
public:
    Array3D(size_t nI, size_t nJ, size_t nK = 1):
        _nI(nI), _nJ(nJ), _nK(nK), _vec( nI * nJ * nK )
    {}

    TYPE & operator()( size_t i, size_t j, size_t k = 0 ) {
        if ( i < _nI && j < _nJ && k < _nK ) {
            return _vec[ i + j * _nI + k * _nJ * _nI ];
        }
        throw std::out_of_range("Indexes out of range");
    }

    TYPE operator()( size_t i, size_t j, size_t k = 0 ) const {
        if ( i < _nI && j < _nJ && k < _nK ) {
            return _vec[ i + j * _nI + k * _nJ * _nI ];
        }
        throw std::out_of_range("Indexes out of range");
    }

private:
    size_t _nI;
    size_t _nJ;
    size_t _nK;
    std::vector<TYPE> _vec;
};
#endif // ARRAY3D_H
