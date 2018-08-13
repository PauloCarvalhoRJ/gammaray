#ifndef FKESTIMATION_H
#define FKESTIMATION_H

#include "geostatsutils.h"

class SearchStrategy;
class VariogramModel;
class Attribute;
class CartesianGrid;
class DataCell;
class SpatialIndexPoints;
class DataFile;

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

	/** Returns a container with the samples around the estimation cell to be used in the estimation.
	 * The resulting collection depends on the SearchStrategy object set.  Returns an empty object if any
	 * required parameter for the search to work (e.g. input data) is missing.
	 */
	std::multiset<DataCell> getSamples(const GridCell & estimationCell );

    /** Preforms the factorial kriging. Make sure all parameters have been set properly .*/
    std::vector<double> run();

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
};

#endif // FKESTIMATION_H
