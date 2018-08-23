#include "fkestimationrunner.h"
#include "fkestimation.h"
#include "domain/cartesiangrid.h"
#include "gridcell.h"

FKEstimationRunner::FKEstimationRunner(FKEstimation *fkEstimation, QObject *parent) :
    QObject(parent),
    m_finished( false ),
	m_fkEstimation( fkEstimation ),
	m_singleStructVModel( nullptr )
{
}

FKEstimationRunner::~FKEstimationRunner()
{
	if( m_singleStructVModel )
		delete m_singleStructVModel;
}

void FKEstimationRunner::doRun()
{
    CartesianGrid* estimationGrid = m_fkEstimation->getEstimationGrid();

    //get the grid dimensions
    uint nI = estimationGrid->getNX();
    uint nJ = estimationGrid->getNY();
    uint nK = estimationGrid->getNZ();

    //prepare the vector with the results (to not overwrite the original data)
    m_factor.clear();
    m_factor.reserve( nI * nJ * nK );
    m_means.clear();
    m_means.reserve( nI * nJ * nK );

	//factor number can be -1 (mean), which is not a valid variographic structure number.
	int ist = m_fkEstimation->getFactorNumber();
	if( ist == -1 )
		ist = 0; //set nugget as default

	//make the single-structure variogram model.
	//Switch to a single-structure variogram model if a structure number was specified.
	if( m_singleStructVModel )
		delete m_singleStructVModel;
	m_singleStructVModel = new VariogramModel( m_fkEstimation->getVariogramModel()->makeVModelFromSingleStructure( ist ) );

    //for all grid cells
    int nKriging = 0;
    int nIllConditioned = 0;
    int nFailed = 0;
    for( uint k = 0; k <nK; ++k)
        for( uint j = 0; j <nJ; ++j){
            emit setLabel("Running FK:\n" +
                          QString::number(nKriging) + " kriging operations (" +
                          QString::number(nIllConditioned) + " ill-conditioned, " +
                          QString::number(nFailed) + " failed). " );
            emit progress( j * nI + k * nI * nJ );
            for( uint i = 0; i <nI; ++i){
				GridCell estimationCell( estimationGrid, -1, i, j, k );
				double estimatedMean;
                m_factor.push_back( fk( estimationCell,
										 m_fkEstimation->getVariogramModel()->getNstWithNugget(),
										 estimatedMean,
										 nIllConditioned,
										 nFailed ) );
                m_means.push_back( estimatedMean );
                ++nKriging;
            }
        }

    //inform the calling thread the computation has finished.
    m_finished = true;
}

double FKEstimationRunner::fk( GridCell& estimationCell,
                               int nst,
                               double& estimatedMean,
                               int &nIllConditioned,
                               int &nFailed )
{
	//collects samples from the input data set ordered by their distance with respect
	//to the estimation cell.
	std::multiset<DataCellPtr> vSamples = m_fkEstimation->getSamples( estimationCell );

	//if no sample was found, either...
	if( vSamples.empty() ){
		//Return the no-data-value defined for the output dataset.
		return m_fkEstimation->ndvOfEstimationGrid();
	}

    //The matrix names follow the formulation presented by
    //Ma et al. (2014) - Factorial kriging for multiscale modelling.

	//get the inverse of the matrix of the theoretical covariances between the data sample locations and themselves.
	// TODO PERFORMANCE: the cov matrix needs only to be computed once.
	MatrixNXM<double> Czz_inv = GeostatsUtils::makeCovMatrix( vSamples,
														  m_fkEstimation->getVariogramModel(),
														  m_fkEstimation->getVariogramSill(),
														  m_fkEstimation->getKrigingType() );
	Czz_inv.invertWithGaussJordan();

	//get the matrix with theoretical covariances between sample locations and estimation location.
	MatrixNXM<double> Cyz = GeostatsUtils::makeGammaMatrix( vSamples,
															estimationCell,
															m_fkEstimation->getVariogramModel(),
															m_fkEstimation->getKrigingType() );

	//get the inverse of matrix of the theoretical covariances between the data sample locations and themselves
	// of the structure targeted for FK analysis.
	// TODO PERFORMANCE: the cov matrix needs only to be computed once.
	MatrixNXM<double> Cij_inv = GeostatsUtils::makeCovMatrix( vSamples,
														  m_singleStructVModel,
														  m_fkEstimation->getVariogramSill(),
														  m_fkEstimation->getKrigingType() );
	Cij_inv.invertWithGaussJordan();

	//get the n x k matrix of an "chosen analytical function p(x) for fitting the nonstationary component".
	//Ma et al. (2014) don't give details of p(x).
    //TODO PERFORMANCE: if P is invariant, then it needs to be computed only once.
	MatrixNXM<double> P = GeostatsUtils::makePmatrixForFK( vSamples.size(), nst );

	//Get P's transpose.
	//TODO PERFORMANCE: is P is invariant, then Pt needs to be computed only once.
	MatrixNXM<double> Pt = P.getTranspose();

	//get the k x 1 matrix of an "chosen analytical function p(x) for fitting the nonstationary component".
	//Ma et al. (2014) don't give details of p(x).
	//TODO PERFORMANCE: is p is invariant, then it needs to be computed only once.
	MatrixNXM<double> p = GeostatsUtils::makepMatrixForFK( nst );

    //Compute an intermediate product.
    MatrixNXM<double> Pt__x__Cij_inv__x__P_____inv = Pt * Cij_inv * P;
    Pt__x__Cij_inv__x__P_____inv.invertWithGaussJordan();

    //Compute the kriging weights for the estimated mean.
    MatrixNXM<double> Lambda_T = Czz_inv * P * ( Pt__x__Cij_inv__x__P_____inv ) * p;

    //Compute the kriging weights for the zero-mean FK estimate.
    MatrixNXM<double> Lambda_yi =
            Czz_inv * Cyz - Czz_inv * P * ( Pt__x__Cij_inv__x__P_____inv ) * Pt * Czz_inv * Cyz;

	//Estimate the mean.
	estimatedMean = 0.0;
	std::multiset<DataCellPtr>::iterator itSamples = vSamples.begin();
	for( uint i = 0; i < vSamples.size(); ++i, ++itSamples){
		estimatedMean += Lambda_T(i,0) * (*itSamples)->readValueFromDataSet();
	}

    //Estimate the factor.
    double factor = 0.0;
    itSamples = vSamples.begin();
    for( uint i = 0; i < vSamples.size(); ++i, ++itSamples){
        factor += Lambda_yi(i,0) * ( (*itSamples)->readValueFromDataSet() - estimatedMean );
    }

	//rarely, kriging may fail with a NaN or infinity value.
	//guard the output against such failures.
	if( std::isnan(factor) || !std::isfinite(factor) ){
		++nFailed;
		double failValue = m_fkEstimation->ndvOfEstimationGrid();
		Application::instance()->logWarn( "FKEstimationRunner::fk(): at least one kriging operation failed (resulted in NaN or infinity).  Returning " +
										  QString::number(failValue) + " to protect the output data file." );
		factor = failValue;
	}
	if( std::isnan(estimatedMean) || !std::isfinite(estimatedMean) ){
		estimatedMean = m_fkEstimation->ndvOfEstimationGrid();
	}

    //Return the factor (and the estimated mean as an output paramater).
    return factor;
}
