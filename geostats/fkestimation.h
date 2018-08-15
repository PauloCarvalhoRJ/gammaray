#ifndef FKESTIMATION_H
#define FKESTIMATION_H

#include "geostatsutils.h"
#include "datacell.h"

class SearchStrategy;
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
    void setSearchStrategy( const SearchStrategy* searchStrategy );
    void setVariogramModel( VariogramModel* variogramModel );
    void setMeanForSimpleKriging( double meanSK );
    void setKrigingType( KrigingType ktype );
    void setInputVariable( Attribute* at_input );
    void setEstimationGrid( CartesianGrid* cg_estimation );
    //@}

    CartesianGrid* getEstimationGrid(){ return m_cg_estimation; }
	Attribute* getInputVariable(){ return m_at_input; }
	VariogramModel* getVariogramModel(){ return m_variogramModel; }
	KrigingType getKrigingType(){ return m_ktype; }

	/** Returns a container with the samples around the estimation cell to be used in the estimation.
	 * The resulting collection depends on the SearchStrategy object set.  Returns an empty object if any
	 * required parameter for the search to work (e.g. input data) is missing.
	 */
	std::multiset< DataCellPtr > getSamples(const GridCell & estimationCell );

	/** Performs the factorial kriging. Make sure all parameters have been set properly .*/
    std::vector<double> run();

	/** Returns the no-data-value for the estimation grid. */
	double ndvOfEstimationGrid(){ return m_NDV_of_output; }

	/** Shortcut method to get the variogam model's sill. */
	double getVariogramSill(){ return m_variogramSill; }

private:
    const SearchStrategy *m_searchStrategy;
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
};

#endif // FKESTIMATION_H
