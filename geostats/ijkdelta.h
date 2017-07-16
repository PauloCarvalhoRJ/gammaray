#ifndef IJKDELTA_H
#define IJKDELTA_H

#include <algorithm>
#include <set>
#include "ijkindex.h"

/** A data structure used in geostats algorithms.
 * It represents a difference in topological coordinates.
 */
class IJKDelta
{
public:
    IJKDelta( int di, int dj, int dk );

    int sum() const { return _di+_dj+_dk; }
    int max() const { return std::max({_di, _dj, _dk}); }

    /** Fills the given vector with the possible indexes from this delta, given a reference index.
     * It was made inline for performance and beacuse it is called in one place.
     * @param result An array with room for at least 8 elements.
     * @return The number of first elements in result with the computed indexes (2, 4 or 8).
     */
    inline int getIndexes(IJKIndex& fromIndex, IJKIndex *result ) const
    {
        int possibleIs[2];
        int possibleJs[2];
        int possibleKs[2];
        int li, lj, lk;

        if( _di > 0 ){
            li = 2;
            possibleIs[0] = fromIndex._i + _di;
            possibleIs[1] = fromIndex._i - _di;
        } else {
            li = 1;
            possibleIs[0] = fromIndex._i;
        }

        if( _dj > 0 ){
            lj = 2;
            possibleJs[0] = fromIndex._j + _dj;
            possibleJs[1] = fromIndex._j - _dj;
        } else {
            lj = 1;
            possibleJs[0] = fromIndex._j;
        }

        if( _dk > 0 ){
            lk = 2;
            possibleKs[0] = fromIndex._k + _dk;
            possibleKs[1] = fromIndex._k - _dk;
        } else {
            lk = 1;
            possibleKs[0] = fromIndex._k;
        }

        int count = 0;
        for( int k = 0; k < lk; ++k )
            for( int j = 0; j < lj; ++j )
                for( int i = 0; i < li; ++i ){
                    result[count]._i = possibleIs[i];
                    result[count]._j = possibleJs[j];
                    result[count]._k = possibleKs[k];
                    ++count;
                }
        return count;
    }

    int _di;
    int _dj;
    int _dk;
};

/**
 * This global non-member less-than operator enables the IJKDelta class as key-able
 * in STL or STL-like ordered containers.
 */
inline bool operator<(const IJKDelta &e1, const IJKDelta &e2){
    int e1sum = e1.sum();
    int e2sum = e2.sum();
    int e1max = e1.max();
    int e2max = e2.max();
    if( e1sum < e2sum ) //greatest delta is given by the sum of its components
        return true;
    if( e1sum > e2sum ) //greatest delta is given by the sum of its components
        return false;
    else if( e1max < e2max ) //otherwise, the greatest delta is given by its greatest component
        return true;
    else if( e1max > e2max ) //otherwise, the greatest delta is given by its greatest component
        return false;
    //tie-breaking comparissons
    else if( e1._dk < e2._dk)
        return true;
    else if( e1._dk > e2._dk)
        return false;
    else if( e1._dj < e2._dj )
        return true;
    else if( e1._dj > e2._dj )
        return false;
    else if( e1._di < e2._di )
        return true;
    return false;
}
#endif // IJKDELTA_H
