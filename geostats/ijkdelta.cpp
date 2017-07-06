#include "ijkdelta.h"
#include "ijkindex.h"

IJKDelta::IJKDelta(int di, int dj, int dk) :
    _di(di), _dj(dj), _dk(dk)
{
}

int IJKDelta::getIndexes(IJKIndex &fromIndex, IJKIndex *result)
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
