#ifndef NDVESTIMATION_H
#define NDVESTIMATION_H

class Attribute;
class GridCell;
class VariogramModel;

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

    void setVariogramModel( VariogramModel *vm ){ _vmodel = vm; }

    void setUseDefaultValue( bool flag ){ _useDefaultValue = flag; }
    bool useDefaultValue(){ return _useDefaultValue; }

    void setDefaultValue( double value ){ _defaultValue = value; }
    double defaultValue(){ return _defaultValue; }

    int searchMaxNumSamples() const;
    void setSearchMaxNumSamples(int searchMaxNumSamples);
    int searchNumCols() const;
    void setSearchNumCols(int searchNumCols);
    int searchNumRows() const;
    void setSearchNumRows(int searchNumRows);
    int searchNumSlices() const;
    void setSearchNumSlices(int searchNumSlices);

    double ndv() const;
    void setNdv(double ndv);

    VariogramModel *vmodel() const;
    void setVmodel(VariogramModel *vmodel);

private:
    Attribute *_at;
    int _searchMaxNumSamples;
    int _searchNumCols;
    int _searchNumRows;
    int _searchNumSlices;

    VariogramModel* _vmodel;

    /** The default value is used when no valued neighbors are found. */
    bool _useDefaultValue;
    double _defaultValue;

    double _ndv;
};

#endif // NDVESTIMATION_H
