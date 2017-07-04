#include "ijkdelta.h"
#include "ijkindex.h"

IJKDelta::IJKDelta(int di, int dj, int dk) :
    _di(di), _dj(dj), _dk(dk)
{
}

std::set<IJKIndex> IJKDelta::getIndexes(IJKIndex &fromIndex)
{
    std::set<IJKIndex> result;

    //tries to insert all eight combinations of deltas (plus and minus), but
    //std::set ensures only a distinct set of indexes (may end up with as little as 2 indexes)
    result.insert( IJKIndex( fromIndex._i + _di, fromIndex._j + _dj, fromIndex._k + _dk) );
    result.insert( IJKIndex( fromIndex._i + _di, fromIndex._j + _dj, fromIndex._k - _dk) );
    result.insert( IJKIndex( fromIndex._i + _di, fromIndex._j - _dj, fromIndex._k + _dk) );
    result.insert( IJKIndex( fromIndex._i + _di, fromIndex._j - _dj, fromIndex._k - _dk) );
    result.insert( IJKIndex( fromIndex._i - _di, fromIndex._j + _dj, fromIndex._k + _dk) );
    result.insert( IJKIndex( fromIndex._i - _di, fromIndex._j + _dj, fromIndex._k - _dk) );
    result.insert( IJKIndex( fromIndex._i - _di, fromIndex._j - _dj, fromIndex._k + _dk) );
    result.insert( IJKIndex( fromIndex._i - _di, fromIndex._j - _dj, fromIndex._k - _dk) );

    return result;
}
