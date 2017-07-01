#ifndef NDVESTIMATION_H
#define NDVESTIMATION_H

class Attribute;
class GridCell;

/** This class encpsulates the estimation of unvalued cells of an Attribute in a Cartesian grid,
 * a tailored kriging operation.
 */
class NDVEstimation
{
public:
    NDVEstimation( Attribute* at );

    /** Preforms the kriging. Make sure all parameters have been set properly .*/
    void run();

    void setSearchParameters(int searchMaxNumSamples,
                             int searchNumCols,
                             int searchNumRows,
                             int searchNumSlices);

private:
    Attribute *_at;
    int _searchMaxNumSamples;
    int _searchNumCols;
    int _searchNumRows;
    int _searchNumSlices;

    void krige( GridCell cell );
};

#endif // NDVESTIMATION_H
