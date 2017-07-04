#ifndef IJKINDEX_H
#define IJKINDEX_H

/** A data structure used in geostats algorithms.
 * It represents a topological coordinate.
 */
class IJKIndex
{
public:
    IJKIndex(int i, int j, int k );

    int _i;
    int _j;
    int _k;
};

/**
 * This global non-member less-than operator enables the IJKIndex class as key-able
 * in STL or STL-like ordered containers.
 */
inline bool operator<(const IJKIndex &e1, const IJKIndex &e2){
    else if( e1._k < e2._k)
        return true;
    else if( e1._k > e2._k)
        return false;
    else if( e1._j < e2._j )
        return true;
    else if( e1._j > e2._j )
        return false;
    else if( e1._i < e2._i )
        return true;
    return false;
}
#endif // IJKINDEX_H
