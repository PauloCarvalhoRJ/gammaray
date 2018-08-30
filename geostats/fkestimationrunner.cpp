#include "fkestimationrunner.h"
#include "fkestimation.h"
#include "domain/cartesiangrid.h"
#include "gridcell.h"
#include "domain/application.h"

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
	m_singleStructVModel->setForceReread( false ); //disable reread from file improves performance.

	//Disable automatic re-read from file for the selected variogram model.  This improves performance.
	m_fkEstimation->getVariogramModel()->readParameters(); //first, make sure the parameters are updated.
	m_fkEstimation->getVariogramModel()->setForceReread( false );

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
				m_factor.push_back( fkDeutsch( estimationCell,
										 m_fkEstimation->getVariogramModel()->getNstWithNugget(),
										 estimatedMean,
										 nIllConditioned,
										 nFailed ) );
                m_means.push_back( estimatedMean );
                ++nKriging;
            }
        }

	//Re-enable automatic re-read from file for the selected variogram model.
	m_fkEstimation->getVariogramModel()->setForceReread( true );

	//inform the calling thread the computation has finished.
    m_finished = true;
}

double FKEstimationRunner::fk( GridCell& estimationCell,
                               int nst,
                               double& estimatedMean,
                               int &nIllConditioned,
                               int &nFailed )
{
	Q_UNUSED(nIllConditioned);

	//collects samples from the input data set ordered by their distance with respect
	//to the estimation cell.
	DataCellPtrMultiset vSamples = m_fkEstimation->getSamples( estimationCell );

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
														  m_fkEstimation->getKrigingType(),
														  true );
    Czz_inv.invertWithGaussJordan();

	//get the matrix with theoretical covariances between sample locations and estimation location.
	MatrixNXM<double> Cyz = GeostatsUtils::makeGammaMatrix( vSamples,
															estimationCell,
															m_fkEstimation->getVariogramModel(),
															m_fkEstimation->getVariogramSill(),
															m_fkEstimation->getKrigingType(),
															true );

	//get the inverse of matrix of the theoretical covariances between the data sample locations and themselves
	// of the structure targeted for FK analysis.
	// TODO PERFORMANCE: the cov matrix needs only to be computed once.
	MatrixNXM<double> Cij_inv = GeostatsUtils::makeCovMatrix( vSamples,
														  m_singleStructVModel,
														  m_singleStructVModel->getSill(),
														  m_fkEstimation->getKrigingType(),
														  true );
	Cij_inv.invertWithGaussJordan();

    //get the n x k matrix of an "chosen analytical function p(x) for fitting the nonstationary component".
	//Ma et al. (2014) don't give details of p(x).
    //TODO PERFORMANCE: if P is invariant, then it needs to be computed only once.
	MatrixNXM<double> P = GeostatsUtils::makePmatrixForFK( vSamples.size(), nst, m_fkEstimation->getKrigingType() );

	//Get P's transpose.
	//TODO PERFORMANCE: is P is invariant, then Pt needs to be computed only once.
	MatrixNXM<double> Pt = P.getTranspose();

	//get the k x 1 matrix of an "chosen analytical function p(x) for fitting the nonstationary component".
	//Ma et al. (2014) don't give details of p(x).
	//TODO PERFORMANCE: is p is invariant, then it needs to be computed only once.
	MatrixNXM<double> p = GeostatsUtils::makepMatrixForFK( nst );

    //Compute an intermediate product.
	//TODO: there's something wrong with P and p.  The paper is not explicit with regards to how to build them...
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

double FKEstimationRunner::fkDeutsch(GridCell & estimationCell, int nst, double & estimatedMean, int & nIllConditioned, int & nFailed)
{
	Q_UNUSED(nst);
	Q_UNUSED(nIllConditioned);

	double factor;

	//collects samples from the input data set ordered by their distance with respect
	//to the estimation cell.
	DataCellPtrMultiset vSamples = m_fkEstimation->getSamples( estimationCell );

	//if no sample was found, either...
	if( vSamples.empty() ){
		//Return the no-data-value defined for the output dataset.
		estimatedMean = m_fkEstimation->ndvOfEstimationGrid();
		return m_fkEstimation->ndvOfEstimationGrid();
	}

	double sill = m_fkEstimation->getVariogramModel()->getSill();

	//get the matrix of the theoretical covariances between the data sample locations and themselves.
	// TODO PERFORMANCE: the cov matrix needs only to be computed once.
	MatrixNXM<double> covMat_inv = GeostatsUtils::makeCovMatrix( vSamples,
															 m_singleStructVModel,
															 sill,
															 m_fkEstimation->getKrigingType() );
	covMat_inv.invertWithGaussJordan();

	//get the gamma matrix (theoretical covariances between sample locations and estimation location)
	MatrixNXM<double> gammaMat = GeostatsUtils::makeGammaMatrix( vSamples,
																 estimationCell,
																 m_singleStructVModel,
																 sill,
																 m_fkEstimation->getKrigingType() );

	//get the kriging weights vector: [w] = [Cov]^-1 * [gamma]
	MatrixNXM<double> weightsSK( covMat_inv * gammaMat );

	//finally, compute the kriging
	if( m_fkEstimation->getKrigingType() == KrigingType::SK ){
		//for SK mode
		double meanSK = m_fkEstimation->getMeanForSimpleKriging();
		factor = meanSK;
		//computing SK the normal way.
		DataCellPtrMultiset::iterator itSamples = vSamples.begin();
		for( uint i = 0; i < vSamples.size(); ++i, ++itSamples){
			factor += weightsSK(i,0) * ( (*itSamples)->readValueFromDataSet() - meanSK );
		}
		estimatedMean = meanSK;
	} else {
		//for OK mode

		//get the OK gamma matrix (theoretical covariances between sample locations and estimation location)
		//TODO: improve performance: Just append 1 to gammaMat.
		MatrixNXM<double> gammaMatOK = GeostatsUtils::makeGammaMatrix( vSamples,
																	   estimationCell,
																	   m_fkEstimation->getVariogramModel(),
																	   m_fkEstimation->getVariogramModel()->getSill(),
																	   KrigingType::OK );

		//make the OK cov matrix (theoretical covariances between sample locations and themselves)
		//TODO: improve performance: Just expand SK matrices with the 1.0s and 0.0s instead of computing new ones.
		// TODO PERFORMANCE: the cov matrix needs only to be computed once.
		MatrixNXM<double> covMatOK_inv = GeostatsUtils::makeCovMatrix( vSamples,
																	   m_fkEstimation->getVariogramModel(),
																	   m_fkEstimation->getVariogramModel()->getSill(),
																	   KrigingType::OK );
		covMatOK_inv.invertWithGaussJordan();

		//make the OK kriging weights matrix (solve the ordinary kriging system).
		//compute the OK weights the normal way (follows the same rationale of SK)
		MatrixNXM<double> weightsOK( covMatOK_inv * gammaMatOK );

		//Correct OK weights according to Deutsch (1995) - "Correcting for negative weights in ordinary kriging"
		{
			// Compute means of: a) covariances between the estimation location and locations with neg weights.
			//                   b) absolute values of negative weights.
			double mean_of_abs_value_of_neg_weights = 0.0;
			double mean_of_cov_between_neg_weights_and_est_location = 0.0;
			int n_neg_weights = 0;
			for( int i = 0; i < weightsOK.getN()-1; ++i ){ //N-1 is to not "correct" the lagrangean (the last element in the weights matrix).
				if( weightsOK(i,0) < 0.0 ){
					mean_of_abs_value_of_neg_weights += std::abs( weightsOK(i,0) );
					mean_of_cov_between_neg_weights_and_est_location += gammaMatOK(i,0);
					++n_neg_weights;
				}
			}
			if( n_neg_weights ){
				MatrixNXM<double> weightsOKcorrected = weightsOK;
				mean_of_abs_value_of_neg_weights /= n_neg_weights;
				mean_of_cov_between_neg_weights_and_est_location /= n_neg_weights;

				// Zero off negative and small positive weights.
				for( int i = 0; i < weightsOKcorrected.getN()-1; ++i ){
					if( weightsOKcorrected(i,0) < 0.0 )
						weightsOKcorrected(i,0) = 0.0;
					else if( weightsOKcorrected(i,0) > 0.0 ){
						double cov = gammaMatOK(i,0);
						double weight = weightsOKcorrected(i,0);
						if( cov < mean_of_cov_between_neg_weights_and_est_location &&
							weight < mean_of_abs_value_of_neg_weights )
							weightsOKcorrected(i,0) = 0.0;
					}
				}
				// Re-standardize weights so they sum up to 1.0 again.
				double sum_from_i_to_end = 0.0;
				for( int a = 0; a < weightsOKcorrected.getN()-1; ++a )
					sum_from_i_to_end += weightsOKcorrected(a, 0);
				for( int i = 0; i < weightsOKcorrected.getN()-1; ++i ){
					if( sum_from_i_to_end > 0.0 )
						weightsOKcorrected(i, 0) /= sum_from_i_to_end;
					else
						weightsOKcorrected(i, 0) = 0.0;
				}
				// Replace the original weights with the corrected ones.
				weightsOK = weightsOKcorrected;
				//Check
				double sum = 0.0;
				for( int i = 0; i < weightsOK.getN()-1; ++i )
					sum += weightsOK(i, 0);
				if( sum < 0.0001 )
					return m_fkEstimation->ndvOfEstimationGrid();
			}
		}

		//Estimate the OK local mean (use OK weights)
		double mOK = 0.0;
		DataCellPtrMultiset::iterator itSamples = vSamples.begin();
		for( int i = 0; i < weightsOK.getN()-1; ++i, ++itSamples){ //the last element in weightsOK is the Lagrangian (mu)
			mOK += weightsOK(i,0) * (*itSamples)->readValueFromDataSet();
		}

		//compute the kriging weight for the local OK mean (use SK weights)
		double wmOK = 1.0;
//		for( int i = 0; i < weightsSK.getN(); ++i){
//			wmOK -= weightsSK(i,0);   //TODO: somehow only when the OK mean weight is 1.0, results are good.
//		}

		//krige (with SK weights plus the OK mean (with OK mean weight))
		factor = 0.0;
		//computing kriging the normal way.
		itSamples = vSamples.begin();
		for( uint i = 0; i < vSamples.size(); ++i, ++itSamples){
			factor += weightsSK(i,0) * ( (*itSamples)->readValueFromDataSet() - mOK );
		}
		//factor += wmOK * mOK;
		estimatedMean = wmOK * mOK;
	}

	//rarely, kriging may fail with a NaN or infinity value.
	//guard the output against such failures.
	if( std::isnan(factor) || !std::isfinite(factor) ){
		++nFailed;
		double failValue = m_fkEstimation->ndvOfEstimationGrid();
		Application::instance()->logWarn( "FKEstimationRunner::fkDeutsch(): at least one kriging operation failed (resulted in NaN or infinity).  Returning " +
										  QString::number(failValue) + " to protect the output data file." );
		factor = failValue;
	}
	if( std::isnan(estimatedMean) || !std::isfinite(estimatedMean) ){
		estimatedMean = m_fkEstimation->ndvOfEstimationGrid();
	}

	return factor;
}
