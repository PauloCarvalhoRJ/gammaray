#ifndef FKESTIMATION_H
#define FKESTIMATION_H

#include "geostatsutils.h"
#include "datacell.h"
#include "searchstrategy.h"

class VariogramModel;
class Attribute;
class CartesianGrid;
class DataCell;
class SpatialIndexPoints;


/** This class encpsulates the factorial kriging estimation.
 */
class FKEstimation
{
public:
    FKEstimation();
	~FKEstimation();

    //@{
	/** Set the Factorial Kriging parameters. */
	void setSearchStrategy( SearchStrategyPtr searchStrategy );
    void setVariogramModel( VariogramModel* variogramModel );
    void setMeanForSimpleKriging( double meanSK );
    void setKrigingType( KrigingType ktype );
    void setInputVariable( Attribute* at_input );
    void setEstimationGrid( CartesianGrid* cg_estimation );
    void setFactorNumber( int factorNumber );
    //@}

    //@{
    /** Getters. */
    SearchStrategyPtr getSearchStrategy(){ return m_searchStrategy; }
    CartesianGrid* getEstimationGrid(){ return m_cg_estimation; }
	Attribute* getInputVariable(){ return m_at_input; }
	VariogramModel* getVariogramModel(){ return m_variogramModel; }
	KrigingType getKrigingType(){ return m_ktype; }
    int getFactorNumber(){ return m_factorNumber; }
	double getMeanForSimpleKriging(){ return m_meanSK; }
    //@}

	/** Returns a container with the samples around the estimation cell to be used in the estimation.
	 * The resulting collection depends on the SearchStrategy object set.  Returns an empty object if any
	 * required parameter for the search to work (e.g. input data) is missing.  The data cells are ordered
	 * by their distance to the passed estimation cell.
	 */
	DataCellPtrMultiset getSamples(const GridCell & estimationCell );

    /** Performs the factorial kriging. Make sure all parameters have been set properly.
     * @param factorNumber The number of factor to get: -1 (mean); 0 (nugget); 1 and onwards (each variographic structure).
     */
	std::vector<double> run( );

	/** Returns the no-data-value for the estimation grid. */
	double ndvOfEstimationGrid(){ return m_NDV_of_output; }

	/** Shortcut method to get the variogam model's sill. */
	double getVariogramSill(){ return m_variogramSill; }

private:
    SearchStrategyPtr m_searchStrategy;
    VariogramModel *m_variogramModel;
    double m_meanSK;
	KrigingType m_ktype;
    Attribute *m_at_input;
    CartesianGrid* m_cg_estimation;
    double m_NDV_of_input;
    double m_NDV_of_output;
	SpatialIndexPoints* m_spatialIndexPoints;
	DataFile* m_inputDataFile;
	double m_variogramSill;
    int m_factorNumber;
};

#endif // FKESTIMATION_H
