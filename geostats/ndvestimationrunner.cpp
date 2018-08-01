#include "ndvestimationrunner.h"

#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "gridcell.h"
#include "geostatsutils.h"
#include "ndvestimation.h"
#include "util.h"
#include "imagejockey/imagejockeyutils.h"

enum class FlagState : char {
    NOT_SET = 0,
    TO_SET,
    SET
};


NDVEstimationRunner::NDVEstimationRunner(NDVEstimation *ndvEstimation, Attribute *at, QObject *parent) :
    QObject(parent),
    _finished( false ),
    _at(at),
    _ndvEstimation(ndvEstimation)
{
}

void NDVEstimationRunner::doRun()
{
    //gets the Attribute's column in its Cartesian grid's data array (GEO-EAS index - 1)
    uint atIndex = _at->getAttributeGEOEASgivenIndex() - 1;

    //Assumes the partent file of the selected attribut is a Cartesian grid
    CartesianGrid *cg = (CartesianGrid*)_at->getContainingFile();

    //get the grid dimensions
    uint nI = cg->getNX();
    uint nJ = cg->getNY();
    uint nK = cg->getNZ();

    //create a neighborhood flag volume.
    //the flag signals that there is at least one valued cell in the search
    //neighborhood.  This flag saves unnecessary calls to krige() for vast voids
    //in the grid.
    std::vector<FlagState> mask;
    mask.reserve(nI * nJ * nK);

    //sets the flags for valued cells
    for( uint k = 0; k <nK; ++k){
        for( uint j = 0; j <nJ; ++j){
            for( uint i = 0; i <nI; ++i){
                double value = cg->dataIJK( atIndex, i, j, k );
                if( cg->isNDV( value ) )
                    mask.push_back( FlagState::NOT_SET );
                else
                    mask.push_back( FlagState::SET );
            }
        }
    }

    //apply dilation algorithm on current flags, so we flag cells
    //which will require a call to krige().  The dilation must be enough
    //to cover the search neighborhood.
    int maskExpansion = std::max({ _ndvEstimation->searchNumCols()/2,
                                   _ndvEstimation->searchNumRows()/2,
                                   _ndvEstimation->searchNumSlices()/2});
    //for each dilation step
    emit setLabel("Creating neighborhood values mask...");
    for( int step = 0; step < maskExpansion; ++step){
        emit progress(  (step/(float)maskExpansion) * nI * nJ * nK );
        //for each cell
        for( uint k = 0; k <nK; ++k){
            for( uint j = 0; j <nJ; ++j){
                for( uint i = 0; i <nI; ++i){
                    //if the flag is not set
                    if( mask[ i + j*nI + k*nJ*nI ] == FlagState::NOT_SET ){
                        FlagState flagToSet = FlagState::NOT_SET; //assumes no value will be found
                        //for each immediate neighbor
                        for( uint kk = std::max(0, (int)k-1); kk < std::min(nK, k+2); ++kk ){
                            for( uint jj = std::max(0, (int)j-1); jj < std::min(nJ, j+2); ++jj ){
                                for( uint ii = std::max(0, (int)i-1); ii < std::min(nI, i+2); ++ii ){
                                    //if the immediate neighbor has a flag set
                                    if( mask[ ii + jj*nI + kk*nJ*nI ] == FlagState::SET ){
                                        //set the flag of the target cell to be set before the next dilation step
                                        flagToSet = FlagState::TO_SET;
                                        //interrupt the immediate neighbor search because the flag is already known
                                        goto set_flag;
                                    }
                                }
                            }
                        }
set_flag:               mask[ i + j*nI + k*nJ*nI ] = flagToSet;
                    }
                }
            }
        }
        //change the TO_SET flags to SET flags for the next dilation step
        for( uint k = 0; k <nK; ++k)
            for( uint j = 0; j <nJ; ++j)
                for( uint i = 0; i <nI; ++i)
                    if( mask[ i + j*nI + k*nJ*nI ] == FlagState::TO_SET )
                        mask[ i + j*nI + k*nJ*nI ] = FlagState::SET;
    }

    //prepare the vector with the results (to not overwrite the original data)
    _results.clear();
    _results.reserve( nI * nJ * nK );

    //get the no-data-value configuration
    bool hasNDV = cg->hasNoDataValue();
    double NDV = -999.0;
    if( hasNDV )
        NDV = cg->getNoDataValueAsDouble();

    //define what value to assign to an estimated cell in absence of values in the search neighborhood
    double valueForNoValuesInNeighborhood;
    if( _ndvEstimation->useDefaultValue() )
        //...assign a default value (e.g. a global mean or expected value).
        valueForNoValuesInNeighborhood = _ndvEstimation->defaultValue();
    else
        //...or assign a no-data-value.
        valueForNoValuesInNeighborhood = _ndvEstimation->ndv();

    //get the sill in a separate variable because VariogramModel::getSill() is slow.
    double variogramSill = _ndvEstimation->vmodel()->getSill();

    //reads variogram parameters from file
    _ndvEstimation->vmodel()->readParameters();

    //disable reread in model's getters to improve performance
    _ndvEstimation->vmodel()->setForceReread( false );

    //for all grid cells
    int nCopies = 0;
    int nTrivial = 0;
    int nKriging = 0;
	int nIllConditioned = 0;
    for( uint k = 0; k <nK; ++k)
        for( uint j = 0; j <nJ; ++j){
			emit setLabel("Running estimation:\n" + QString::number(nCopies) + " copies of values.\n" +
						  QString::number(nTrivial) + " trivial cases.\n" +
						  QString::number(nKriging) + " actual kriging operations (" + QString::number(nIllConditioned) + " ill-conditioned). " );
            emit progress( j * nI + k * nI * nJ );
			for( uint i = 0; i <nI; ++i){
                double value = cg->dataIJK( atIndex, i, j, k );
                if( cg->isNDV( value ) ){
                    //found an unvalued cell, call krige() only if we're sure we have at least one valued
                    //cell in the neighborhood.
                    if( mask[ i + j*nI + k*nJ*nI ] == FlagState::SET ){
						GridCell cell(cg, atIndex, i,j,k);
                        //estimate if at least one value exists in the neighborhood
                        ++nKriging;
						_results.push_back( krige( cell , _ndvEstimation->meanForSK(), hasNDV, NDV, variogramSill, nIllConditioned ) );
                    } else {
                        ++nTrivial;
                        _results.push_back( valueForNoValuesInNeighborhood );
                    }
                }
                else{
                    ++nCopies;
                    _results.push_back( value ); //simple copy from valued cells
                }
            }
        }

    //restore automatic reread in model's getters
    _ndvEstimation->vmodel()->setForceReread( true );

    //inform the calling thread computation has finished
    _finished = true;
}

double NDVEstimationRunner::krige(GridCell cell, double meanSK, bool hasNDV, double NDV, double variogramSill, int& nIllConditioned )
{
    double result = std::numeric_limits<double>::quiet_NaN();

    //collects valued n-neighbors ordered by their topological distance with respect
    //to the target cell
    std::multiset<GridCell> vCells;

    GeostatsUtils::getValuedNeighborsTopoOrdered( cell,
                                                           _ndvEstimation->searchMaxNumSamples(),
                                                           _ndvEstimation->searchNumCols(),
                                                           _ndvEstimation->searchNumRows(),
                                                           _ndvEstimation->searchNumSlices(),
                                                           hasNDV,
                                                           NDV,
                                                           vCells);

    //if no sample was found, either...
	if( vCells.empty() ){
        if( _ndvEstimation->useDefaultValue() )
            //...return a default value (e.g. a global mean or expected value).
            return _ndvEstimation->defaultValue();
        else
            //...or return a no-data-value.
            return _ndvEstimation->ndv();
    }

	//get the covariance matrix for the neighbors cell.
    MatrixNXM<double> covMat = GeostatsUtils::makeCovMatrix( vCells,
                                                             _ndvEstimation->vmodel(),
                                                             variogramSill );

	//get the gamma matrix (covariances between samples and estimation locations)
	MatrixNXM<double> gammaMat = GeostatsUtils::makeGammaMatrix( vCells, cell, _ndvEstimation->vmodel() );

	//The eta (after greek letter eta) number is the threshold below which the eigenvalues are rounded off to zero
	//The eta number and the value are both in Mohammadi et al (2016) paper (see complete reference further below).
	double eta = 0.001;

	//test whether the covariance matrix is ill-conditioned (near-singular).
	//an ill-conditioned matrix has a bad numerical invert.
	bool is_cov_matrix_ill_conditioned = false;
	int cov_matrix_rank = 0;
	spectral::array eigenvectors, eigenvalues;
	std::tie( eigenvectors, eigenvalues ) = spectral::eig( covMat.toSpectralArray() );
	{
		double smallestEigenValue = std::numeric_limits<double>::max();
		double largestEigenValue = 0; //since the cov table is positive definite, the smallest value possible is zero.
		for( int i = 0; i < eigenvalues.size(); ++i ){
			double eigenvalue = eigenvalues(i);
			//since the cov table is positive definite, there is no need to compute the absolute value.
			if( eigenvalue < smallestEigenValue )
				smallestEigenValue = eigenvalue;
			if( eigenvalue > largestEigenValue )
				largestEigenValue = eigenvalue;
			if( eigenvalue > eta )
				cov_matrix_rank = i;
		}
		++cov_matrix_rank;
		double condition_number = largestEigenValue / smallestEigenValue;
		if( condition_number > 10 ){
			is_cov_matrix_ill_conditioned = true;
			++nIllConditioned;
		}
	}

	//make the kriging weights matrix (solve the kriging system).
	MatrixNXM<double> weightsSK( vCells.size() );
	if( is_cov_matrix_ill_conditioned ){
		spectral::array weightsSKSpectral = weightsSK.toSpectralArray();
		//make a spectral::array matrix from the data values (response values).
		spectral::array y( vCells.size() );
		{ //make the response-value (sample values) vector y.
			std::multiset<GridCell>::iterator vit = vCells.begin();
			for( int i = 0; vit != vCells.end(); ++vit, ++i )
				y(i) = (*vit).readValueFromGrid() - meanSK;
		}
		//Compute the kriging weights with the Pseudoinverse Regularization proposed by Mohammadi et al (2016) - Equation 12.
		// "An analytic comparison of regularization methods for Gaussian Processes" - https://arxiv.org/pdf/1602.00853.pdf
		for( int i = 0; i < cov_matrix_rank; ++i) {
			spectral::array eigenvector = eigenvectors.getVectorColumn( i );
			spectral::array eigenvectorT = spectral::transpose( eigenvector );
			weightsSKSpectral += ( (eigenvectorT * y) / eigenvalues(i) )(0) * eigenvector; //the (0) is to take the single matrix element as a scalar an not as an product-incompatible matrix.
		}
		weightsSK = MatrixNXM<double>( weightsSKSpectral );
	} else { // if the cov matrix is well conditioned, the kriging weights are computed the traditional way.
		//invert the covariance matrix.
		covMat.invertWithGaussJordan();
		//get the kriging weights vector: [w] = [Cov]^-1 * [gamma]
		weightsSK = covMat * gammaMat;  //covMat is inverted
	}

    //finally, compute the kriging
    if( _ndvEstimation->ktype() == KrigingType::SK ){
        //for SK mode
        result = meanSK;
		if( is_cov_matrix_ill_conditioned ){
			result += (gammaMat.getTranspose() * weightsSK)(0,0); //(0,0) is to get the single element as a scalar and not as a matrix object.
		} else {
			std::multiset<GridCell>::iterator itSamples = vCells.begin();
			for( uint i = 0; i < vCells.size(); ++i, ++itSamples){
				result += weightsSK(i,0) * ( (*itSamples).readValueFromGrid() - meanSK );
			}
		}
    } else {
        //for OK mode
        //compute the OK weights (follows the same rationale of SK)
        //TODO: improve performance.  Just expand SK matrices with the 1.0s and 0.0s instead of computing new ones.
        MatrixNXM<double> covMatOK = GeostatsUtils::makeCovMatrix( vCells,
                                                                   _ndvEstimation->vmodel(),
                                                                   variogramSill,
                                                                   KrigingType::OK );
		covMatOK.invertWithGaussJordan();
        MatrixNXM<double> gammaMatOK = GeostatsUtils::makeGammaMatrix( vCells, cell, _ndvEstimation->vmodel(), KrigingType::OK );
        MatrixNXM<double> weightsOK = covMatOK * gammaMatOK;  //covMat is inverted
        //Estimate the OK local mean (use OK weights)
        double mOK = 0.0;
        std::multiset<GridCell>::iterator itSamples = vCells.begin();
        for( int i = 0; i < weightsOK.getN()-1; ++i, ++itSamples){ //the last element in weightsOK is the Lagrangian (mu)
            mOK += weightsOK(i,0) * (*itSamples).readValueFromGrid();
        }
        //compute the kriging weight for the local OK mean (use SK weights)
        double wmOK = 1.0;
        for( int i = 0; i < weightsSK.getN(); ++i){
            wmOK -= weightsSK(i,0);
        }
        //krige (with SK weights plus the OK mean (with OK mean weight))
        result = 0.0;
        itSamples = vCells.begin();
        for( uint i = 0; i < vCells.size(); ++i, ++itSamples){
            result += weightsSK(i,0) * ( (*itSamples).readValueFromGrid() );
        }
        result += wmOK * mOK;
    }

    return result;
}
