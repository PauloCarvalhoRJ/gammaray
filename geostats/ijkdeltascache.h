#ifndef IJKDELTASCACHE_H
#define IJKDELTASCACHE_H

#include <map>
#include <set>
#include "ijkdelta.h"


/** Key used in IJKDeltasCache. */
class IJKDeltasCacheKey
{
public:
    IJKDeltasCacheKey( int _nColsAround,
                       int _nRowsAround,
                       int _nSlicesAround );
    int _nColsAround;
    int _nRowsAround;
    int _nSlicesAround;
};

/** Define a type for brevity. */
typedef std::map< IJKDeltasCacheKey, std::vector<IJKDelta>* > IJKDeltasCacheMap;

/** Cache used to fetch previously created lists of IJKDelta objects. */
class IJKDeltasCache
{
public:
    IJKDeltasCache();

    static IJKDeltasCacheMap cache;
};

/**
 * This global non-member less-than operator enables the IJKDeltasCacheKey class as key-able
 * in STL or STL-like ordered containers.
 */
inline bool operator<(const IJKDeltasCacheKey &e1, const IJKDeltasCacheKey &e2){
   if( e1._nColsAround < e2._nColsAround)
        return true;
    else if( e1._nColsAround > e2._nColsAround)
        return false;
    else if( e1._nRowsAround < e2._nRowsAround )
        return true;
    else if( e1._nRowsAround > e2._nRowsAround )
        return false;
    else if( e1._nSlicesAround < e2._nSlicesAround )
        return true;
    return false;
}
#endif // IJKDELTASCACHE_H
