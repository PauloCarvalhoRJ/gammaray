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

	//prepare the vectors with the results (to not overwrite the original data)
    m_factor.clear();
    m_factor.reserve( nI * nJ * nK );
    m_means.clear();
    m_means.reserve( nI * nJ * nK );
	m_nSamples.clear();
	m_nSamples.reserve( nI * nJ * nK );

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

	//if the fator desired is nugget, then the approach is to estimate all structures less the nugget
	if( m_singleStructVModel->isPureNugget() ){
		delete m_singleStructVModel;
		m_singleStructVModel = new VariogramModel( m_fkEstimation->getVariogramModel()->makeVModelWithoutNugget() );
		m_singleStructVModel->setForceReread( false ); //disable reread from file improves performance.
	}

	//Disable automatic re-read from file for the selected variogram model.  This improves performance.
	m_fkEstimation->getVariogramModel()->readParameters(); //first, make sure the parameters are updated.
	m_fkEstimation->getVariogramModel()->setForceReread( false );

    //for all grid cells
    int nKriging = 0;
    int nFailed = 0;
    for( uint k = 0; k <nK; ++k){
        for( uint j = 0; j <nJ; ++j){
            emit setLabel("Running FK:\n" +
                          QString::number(nKriging) + " kriging operations (" +
                          QString::number(nFailed) + " failed). " );
            emit progress( j * nI + k * nI * nJ );
            for( uint i = 0; i <nI; ++i){
				GridCell estimationCell( estimationGrid, -1, i, j, k );
				double estimatedMean;
				uint nSamples;
				m_factor.push_back( fk(  estimationCell,
										 estimatedMean,
										 nSamples,
										 nFailed ) );
                m_means.push_back( estimatedMean );
				m_nSamples.push_back( nSamples );
                ++nKriging;
            }
        }
    }

	//Re-enable automatic re-read from file for the selected variogram model.
	m_fkEstimation->getVariogramModel()->setForceReread( true );

	//inform the calling thread the computation has finished.
    m_finished = true;
}

double FKEstimationRunner::fk(GridCell & estimationCell, double& estimatedMean, uint& nSamples, int& nFailed)
{
	double factor;

	//Compute an adequate epsilon for the nugget factor estimation: about 10% of the grid cell size.
	//TODO: performance.  This doesn't need to be here because the cell dimensions are not supposed to vary.
	double epsilonNugget = 0.1;
	if( m_fkEstimation->getFactorNumber() == 0 ){
		epsilonNugget = std::min<double>( estimationCell._grid->getDX(), estimationCell._grid->getDY() );
		if( estimationCell._grid->isTridimensional() )
			epsilonNugget = std::min<double>( epsilonNugget, estimationCell._grid->getDZ() );
		epsilonNugget /= 10;
	}

	//collects samples from the input data set ordered by their distance with respect
	//to the estimation cell.
	DataCellPtrMultiset vSamples = m_fkEstimation->getSamples( estimationCell );

	//register the number of samples to be used in the estimation.
	nSamples = vSamples.size();

    //if no samples was returned found...
    if( vSamples.empty() ){
        //...Return the no-data-value defined for the output dataset.
		estimatedMean = m_fkEstimation->ndvOfEstimationGrid();
		return m_fkEstimation->ndvOfEstimationGrid();
	}

	//*************************SFK***************************************
	if( m_fkEstimation->getKrigingType() == KrigingType::SK ){

		//Get the user-supplied simple kriging mean.
		double mSK = m_fkEstimation->getMeanForSimpleKriging();

		//get the covariance matrix (theoretical full covariances between the data sample locations and themselves.)
		MatrixNXM<double> covMat_inv = GeostatsUtils::makeCovMatrix( vSamples,
																 m_fkEstimation->getVariogramModel(),
																 m_fkEstimation->getVariogramModel()->getSill(),
																 KrigingType::SK,
																 true ); //using semivariogram per Deutsch
		covMat_inv.invertWithGaussJordan();

		if( m_fkEstimation->getFactorNumber() != 0 ){
			//get the gamma matrix (theoretical partial covariances between sample locations and estimation location)
			MatrixNXM<double> gammaMatSFK = GeostatsUtils::makeGammaMatrix( vSamples,
																			estimationCell,
																			m_singleStructVModel,
																			m_singleStructVModel->getSill(),
																			KrigingType::SK,
																			true ); //using semivariogram per Deutsch

			//get the kriging weights vector: [w] = [Cov]^-1 * [gamma] (solve the kriging system)
			MatrixNXM<double> weightsSFK( covMat_inv * gammaMatSFK );

			//Apply the weights (estimate).
			factor = 0.0; //the mean is a separate factor.
			DataCellPtrMultiset::iterator itSamples = vSamples.begin();
			for( uint i = 0; i < vSamples.size(); ++i, ++itSamples){
				factor += weightsSFK(i,0) * ( (*itSamples)->readValueFromDataSet() - mSK );
			}
		}

		//if the user opted for the nugget effect, the procedure is different:
		//FK is used to compute estimates with a small shift of the estimation location.
		//This procedure effectively eliminates nugget effect.  Thus, the nugget factor
		//is the difference between the normal SK estimate and the FK estimate with estimation
		//location shift.
		if( m_fkEstimation->getFactorNumber() == 0 ){
			//The gamma matrix for exact SK.
			MatrixNXM<double> gammaMatSK = GeostatsUtils::makeGammaMatrix( vSamples,
																		   estimationCell,
																		   m_fkEstimation->getVariogramModel(),
																		   m_fkEstimation->getVariogramModel()->getSill(),
																		   KrigingType::SK,
																		   true ); //using semivariogram per Deutsch
			//The gamma matrix for exact SK with the esimation location slightly shifted.
			MatrixNXM<double> gammaMatSansNugget = GeostatsUtils::makeGammaMatrix( vSamples,
																		   estimationCell,
																		   m_fkEstimation->getVariogramModel(),
																		   m_fkEstimation->getVariogramModel()->getSill(),
																		   KrigingType::SK,
																		   true, //using semivariogram per Deutsch
																		   epsilonNugget );
			//The kriging weights for exact SK.
			MatrixNXM<double> weightsSK( covMat_inv * gammaMatSK );
			//The kriging weights for FK with shifted location.
			MatrixNXM<double> weightsSansNugget( covMat_inv * gammaMatSansNugget );
			//Apply the SK weights (estimate).
			factor = 0.0;
			DataCellPtrMultiset::iterator itSamples = vSamples.begin();
			for( uint i = 0; i < vSamples.size(); ++i, ++itSamples){
				factor += ( weightsSK(i,0) - weightsSansNugget(i,0) ) * ( (*itSamples)->readValueFromDataSet() - mSK );
			}
		}

		//The "estimated" mean is simply the user-given global constant mean.
		estimatedMean = mSK;

	//*************************OFK***************************************
	} else {

		//get the covariance matrix (theoretical full covariances between the data sample locations and themselves.)
		MatrixNXM<double> CZZ_inv = GeostatsUtils::makeCovMatrix( vSamples,
																 m_fkEstimation->getVariogramModel(),
																 m_fkEstimation->getVariogramModel()->getSill(),
																 KrigingType::SK,
																 true ); //using semivariogram per Deutsch
		CZZ_inv.invertWithGaussJordan();

		//get the gamma matrix (theoretical partial covariances between sample locations and estimation location)
		MatrixNXM<double> CY = GeostatsUtils::makeGammaMatrix( vSamples,
																	 estimationCell,
																	 m_singleStructVModel,
																	 m_singleStructVModel->getSill(),
																	 KrigingType::SK,
																	 true ); //using semivariogram per Deutsch

		//Make a vector-column of ones and its transpose.
		MatrixNXM<double> e( vSamples.size(), 1, 1.0 );
		MatrixNXM<double> e_t = e.getTranspose();

		//Get a compatible identity matrix.
		MatrixNXM<double> I( vSamples.size(), vSamples.size() );
		I.setIdentity();

		//get the kriging weights vector: [w] = [Cov]^-1 * [gamma] (solve the kriging system)
		//the sum of these weights is zero.
		MatrixNXM<double> et_x_CZZ_inv_x_e____inv( e_t * CZZ_inv * e );
		et_x_CZZ_inv_x_e____inv(0, 0) = 1 / et_x_CZZ_inv_x_e____inv(0, 0);


		//To estimate non-nugget factors.
		if( m_fkEstimation->getFactorNumber() != 0 ){
			MatrixNXM<double> weightsFactor( ( CY.getTranspose() * CZZ_inv * ( I - et_x_CZZ_inv_x_e____inv(0,0) * e * e_t * CZZ_inv ) ).getTranspose() );

			//Apply the weights (estimate).
			factor = 0.0;
			DataCellPtrMultiset::iterator itSamples = vSamples.begin();
			for( uint i = 0; i < vSamples.size(); ++i, ++itSamples){
				factor += weightsFactor(i,0) * ( (*itSamples)->readValueFromDataSet() );
			}
		//To estimate the nugget factor (apply location shift trick)
		} else {
			//Make a full gamma matrix with estimation location slightly shifted.
			MatrixNXM<double> CAA_t = GeostatsUtils::makeGammaMatrix( vSamples,
																	estimationCell,
																	m_fkEstimation->getVariogramModel(),
																	m_fkEstimation->getVariogramModel()->getSill(),
																	KrigingType::SK,
																	true,  //using semivariogram per Deutsch
																	epsilonNugget ).getTranspose();
			//Get the weights for shifted location.
			MatrixNXM<double> weightsNugget(  ( CAA_t * CZZ_inv * ( I - et_x_CZZ_inv_x_e____inv(0,0) * e * e_t * CZZ_inv ) +
												(et_x_CZZ_inv_x_e____inv(0,0) * e_t * CZZ_inv) ).getTranspose()  );
			//get a full gamma matrix.
			MatrixNXM<double> gammaNugget = GeostatsUtils::makeGammaMatrix( vSamples,
																		 estimationCell,
																		 m_fkEstimation->getVariogramModel(),
																		 m_fkEstimation->getVariogramModel()->getSill(),
																		 KrigingType::SK,
																		 true ); //using semivariogram per Deutsch
			//Get weights (without location shift).
			//MatrixNXM<double> weightsFactor( ( gammaNugget.getTranspose() * CZZ_inv * ( I - et_x_CZZ_inv_x_e____inv(0,0) * e * e_t * CZZ_inv ) ).getTranspose() );
			MatrixNXM<double> weightsFactor(  ( gammaNugget.getTranspose() * CZZ_inv * ( I - et_x_CZZ_inv_x_e____inv(0,0) * e * e_t * CZZ_inv ) +
												(et_x_CZZ_inv_x_e____inv(0,0) * e_t * CZZ_inv) ).getTranspose()  );
			//Apply the weights (estimate).
			factor = 0.0;
			DataCellPtrMultiset::iterator itSamples = vSamples.begin();
			for( uint i = 0; i < vSamples.size(); ++i, ++itSamples){
				factor += ( weightsFactor(i,0) - weightsNugget(i,0) ) * ( (*itSamples)->readValueFromDataSet() );
			}
		}

		//Estimating the mean
		{
			//Getting OK weights to estimate the neighborhood mean.
			MatrixNXM<double> weightsMean( ( et_x_CZZ_inv_x_e____inv(0,0) * e_t * CZZ_inv ).getTranspose() );

			//Apply the OK weights (estimate the mean).
			estimatedMean = 0.0;
			DataCellPtrMultiset::iterator itSamples = vSamples.begin();
			for( uint i = 0; i < vSamples.size(); ++i, ++itSamples){
				estimatedMean += weightsMean(i,0) * ( (*itSamples)->readValueFromDataSet() );
			}
		}
	}

	//************************* Check result and return ***************************************

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
