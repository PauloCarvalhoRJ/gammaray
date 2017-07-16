#include "ijkdeltascache.h"

/*static*/ IJKDeltasCacheMap IJKDeltasCache::cache;

IJKDeltasCacheKey::IJKDeltasCacheKey(int nColsAround,
                                     int nRowsAround,
                                     int nSlicesAround ):
    _nColsAround(nColsAround), _nRowsAround(nRowsAround), _nSlicesAround(nSlicesAround)
{
}


IJKDeltasCache::IJKDeltasCache()
{

}
