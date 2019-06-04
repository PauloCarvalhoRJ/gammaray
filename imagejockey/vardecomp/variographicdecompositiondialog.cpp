#include "variographicdecompositiondialog.h"
#include "ui_variographicdecompositiondialog.h"
#include "../widgets/ijcartesiangridselector.h"
#include "../widgets/ijvariableselector.h"
#include "../ijabstractcartesiangrid.h"
#include "../ijabstractvariable.h"
#include "../imagejockeyutils.h"
#include "spectral/svd.h"
#include "../svd/svdfactortree.h"
#include "../svd/svdfactor.h"
#include "../svd/svdanalysisdialog.h"
#include "../widgets/ijquick3dviewer.h"
#include "imagejockey/gabor/gaborutils.h"
#include "imagejockey/ijvariographicmodel2d.h"

#include <QMessageBox>
#include <QProgressDialog>
#include <cstdlib>
#include <thread>
#include <algorithm>
#include <limits>
#include <mutex>
#include <vtkPolyData.h>
#include <vtkDelaunay2D.h>
#include <vtkCleanPolyData.h>
#include <cassert>

std::mutex mutexObjectiveFunction;

struct objectiveFunctionFactors{
    double f1, f2, f3, f4, f5, f6, f7;
};


//////////////////////////////CLASS FOR THE GENETIC ALGORITHM//////////////////////////////

class Individual{
public:
    //constructors
    Individual() = delete;
    Individual( int nNumberOfParameters ) :
        parameters( nNumberOfParameters ),
        fValue( std::numeric_limits<double>::max() )
    {}
    Individual( const spectral::array& pparameters ) :
        parameters( pparameters ),
        fValue( std::numeric_limits<double>::max() )
    {}
    Individual( const Individual& otherIndividual ) :
        parameters( otherIndividual.parameters ),
        fValue( otherIndividual.fValue )
    {}

    //methods
    std::pair<Individual, Individual> crossOver( const Individual& otherIndividual,
                                                 int pointOfCrossOver ) const {
        assert( parameters.size() && otherIndividual.parameters.size() &&
            "Individual::crossOver(): Either operands have zero parameters.") ;
        Individual child1( parameters.size() ), child2( parameters.size() );
        for( int iParameter = 0; iParameter < parameters.size(); ++iParameter ){
            if( iParameter < pointOfCrossOver ){
                child1.parameters[iParameter] = parameters[iParameter];
                child2.parameters[iParameter] = otherIndividual.parameters[iParameter];
            } else {
                child1.parameters[iParameter] = otherIndividual.parameters[iParameter];
                child2.parameters[iParameter] = parameters[iParameter];
            }
        }
        return { child1, child2 };
    }
    void mutate( double mutationRate,
                 const spectral::array& lowBoundaries,
                 const spectral::array& highBoundaries ){
        //sanity checks
        if( lowBoundaries.size() != parameters.size() ||
            highBoundaries.size() != parameters.size() ){
            QString message ("Individual::mutate(): Either low boundary (n=");
            message.append( QString::number( lowBoundaries.size() ) );
            message.append( ") or the high boundary (n=" );
            message.append( QString::number( highBoundaries.size() ) );
            message.append( ") have a different number of elements than the parameters member (n=" );
            message.append( QString::number( parameters.size() ) );
            message.append( "). Operation canceled.") ;
            QMessageBox::critical( nullptr, "Error", message);
            return;
        }
        //compute the mutation probability for a single gene (parameter)
        double probOfMutation = 1.0 / parameters.size() * mutationRate;
        //traverse all genes (parameters)
        for( int iPar = 0.0; iPar < parameters.size(); ++iPar ){
            //draw a value between 0.0 and 1.0 from an uniform distribution
            double p = std::rand() / (double)RAND_MAX;
            //if a mutation is due...
            if( p < probOfMutation ) {
                //perform mutation by randomly sorting a value within the domain.
                double LO = lowBoundaries[iPar];
                double HI = highBoundaries[iPar];
                parameters[iPar] = LO + std::rand() / (RAND_MAX/(HI-LO));
            }
        }
    }

    //member variables
    spectral::array parameters;
    double fValue;

    //operators
    Individual& operator=( const Individual& otherIndividual ){
        parameters = otherIndividual.parameters;
        fValue = otherIndividual.fValue;
        return *this;
    }
    bool operator<( const Individual& otherIndividual ) const {
        return fValue < otherIndividual.fValue;
    }

};
typedef Individual Solution; //make a synonym just for code readbility

///////////////////////////////////////////////////////////////////////////////////////////

/** The objective function for the optimization process (SVD on varmap).
 * See complete theory in the program manual for in-depth explanation of the method's parameters below.
 * @param originalGrid  The grid with original data for comparison.
 * @param vectorOfParameters The column-vector with the free paramateres.
 * @param A The LHS of the linear system originated from the information conservation constraints.
 * @param Adagger The pseudo-inverse of A.
 * @param B The RHS of the linear system originated from the information conservation constraints.
 * @param I The identity matrix compatible with the formula: [a] = Adagger.B + (I-Adagger.A)[w]
 * @param m The desired number of geological factors.
 * @param fundamentalFactors  The list with the original data's fundamental factors computed with SVD.
 * @param fftOriginalGridMagAndPhase The Fourier image of the original data in polar form.
 * @return A distance/difference measure.
 */
//TODO: PERFORMANCE: F is costly and is called often.  Invest optimization effort here first.
double F(const spectral::array &originalGrid,
		 const spectral::array &vectorOfParameters,
		 const spectral::array &A,
		 const spectral::array &Adagger,
		 const spectral::array &B,
		 const spectral::array &I,
		 const int m,
		 const std::vector<spectral::array> &fundamentalFactors,
		 const spectral::complex_array& fftOriginalGridMagAndPhase,
		 const bool addSparsityPenalty,
         const bool addOrthogonalityPenalty,
         const double sparsityThreshold)
{
	std::unique_lock<std::mutex> lck (mutexObjectiveFunction, std::defer_lock);

	int nI = originalGrid.M();
	int nJ = originalGrid.N();
	int nK = originalGrid.K();
	int n = fundamentalFactors.size();

	//Compute the vector of weights [a] = Adagger.B + (I-Adagger.A)[w]
	spectral::array va;
	{
		Eigen::MatrixXd eigenAdagger = spectral::to_2d( Adagger );
		Eigen::MatrixXd eigenB = spectral::to_2d( B );
		Eigen::MatrixXd eigenI = spectral::to_2d( I );
		Eigen::MatrixXd eigenA = spectral::to_2d( A );
		Eigen::MatrixXd eigenvw = spectral::to_2d( vectorOfParameters );
		Eigen::MatrixXd eigenva = eigenAdagger * eigenB + ( eigenI - eigenAdagger * eigenA ) * eigenvw;
		va = spectral::to_array( eigenva );
	}

	//Compute the sparsity of the solution matrix
	double sparsityPenalty = 1.0;
	if( addSparsityPenalty ){
		int nNonZeros = va.size();
		for (int i = 0; i < va.size(); ++i )
            if( std::abs(va.d_[i]) <= sparsityThreshold )
				--nNonZeros;
		sparsityPenalty = nNonZeros/(double)va.size();
	}

	//Make the m geological factors (expected variographic structures)
	std::vector< spectral::array > geologicalFactors;
	{
		for( int iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor){
			spectral::array geologicalFactor( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
			for( int iSVDFactor = 0; iSVDFactor < n; ++iSVDFactor){
				geologicalFactor += fundamentalFactors[iSVDFactor] * va.d_[ iGeoFactor * m + iSVDFactor ];
			}
			geologicalFactors.push_back( std::move( geologicalFactor ) );
		}
	}

	//Compute the grid derived form the geological factors (ideally it must match the input grid)
	spectral::array derivedGrid( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
	{
		//Sum up all geological factors.
		spectral::array sum( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
		std::vector< spectral::array >::iterator it = geologicalFactors.begin();
		for( ; it != geologicalFactors.end(); ++it ){
			sum += *it;
		}
		//Compute FFT of the sum
		spectral::complex_array tmp;
		lck.lock();                   //
		spectral::foward( tmp, sum ); //fftw crashes when called simultaneously
		lck.unlock();                 //
		//inbue the sum's FFT with the phase field of the original data.
		for( int idx = 0; idx < tmp.size(); ++idx)
		{
			std::complex<double> value;
			//get the complex number value in rectangular form
			value.real( tmp.d_[idx][0] ); //real part
			value.imag( tmp.d_[idx][1] ); //imaginary part
			//convert to polar form
			//but phase is replaced with that of the original data
			tmp.d_[idx][0] = std::abs( value ); //magnitude part
			tmp.d_[idx][1] = fftOriginalGridMagAndPhase.d_[idx][1]; //std::arg( value ); //phase part (this should be zero all over the grid)
			//convert back to rectangular form (recall that the varmap holds covariance values, then it is necessary to take its square root)
			value = std::polar( std::sqrt(tmp.d_[idx][0]), tmp.d_[idx][1] );
			tmp.d_[idx][0] = value.real();
			tmp.d_[idx][1] = value.imag();
		}
		//Compute RFFT (with the phase of the original data imbued)
		spectral::array rfftResult( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
		lck.lock();                            //
		spectral::backward( rfftResult, tmp ); //fftw crashes when called simultaneously
		lck.unlock();                          //
		//Divide the RFFT result (due to fftw3's RFFT implementation) by the number of grid cells
		rfftResult = rfftResult * (1.0/(nI*nJ*nK));
		derivedGrid += rfftResult;
	}

	//Compute the penalty caused by the angles between the vectors formed by the fundamental factors in each geological factor
	//The more orthogonal (angle == PI/2) the better.  Low angles result in more penalty.
	//The penalty value equals the smallest angle in radians between any pair of weights vectors.
	double orthogonalityPenalty = 1.0;
	if( addOrthogonalityPenalty ){
		//make the vectors of weights for each geological factor
		std::vector<spectral::array*> vectors;
		spectral::array* currentVector = new spectral::array();
		for( size_t i = 0; i < va.d_.size(); ++i){
			if( currentVector->size() < n )
				currentVector->d_.push_back( va.d_[i] );
			else{
				vectors.push_back( currentVector );
				currentVector = new spectral::array();
			}
		}
		//compute the penalty
		for( size_t i = 0; i < vectors.size()-1; ++i ){
			for( size_t j = i+1; j < vectors.size(); ++j ){
				spectral::array* vectorA = vectors[i];
				spectral::array* vectorB = vectors[j];
				double angle = spectral::angle( *vectorA, *vectorB );
				if( angle < orthogonalityPenalty )
					orthogonalityPenalty = angle;
			}
		}
		orthogonalityPenalty = 1.0 - orthogonalityPenalty/1.571; //1.571 radians ~ 90 degrees
		//cleanup the vectors
		for( size_t i = 0; i < vectors.size(); ++i )
			delete vectors[i];
	}

	//Return the measure of difference between the original data and the derived grid
	// The measure is multiplied by a factor that is a function of weights vector angle penalty (the more close to orthogonal the less penalty )
	return spectral::sumOfAbsDifference( originalGrid, derivedGrid )
		   * sparsityPenalty
		   * orthogonalityPenalty;
}

/** The objective function for the optimization process (FFT on original data).
 * See complete theory in the program manual for in-depth explanation of the method's parameters below.
 * @param originalGrid  The grid with original data for comparison.
 * @param vectorOfParameters The column-vector with the free paramateres.
 * @param A The LHS of the linear system originated from the information conservation constraints.
 * @param Adagger The pseudo-inverse of A.
 * @param B The RHS of the linear system originated from the information conservation constraints.
 * @param I The identity matrix compatible with the formula: [a] = Adagger.B + (I-Adagger.A)[w]
 * @param m The desired number of geological factors.
 * @param fundamentalFactors  The list with the original data's fundamental factors computed with SVD.
 * @return A distance/difference measure.
 */
double F2(const spectral::array &originalGrid,
         const spectral::array &vectorOfParameters,
         const spectral::array &A,
         const spectral::array &Adagger,
         const spectral::array &B,
         const spectral::array &I,
         const int m,
         const std::vector<spectral::array> &fundamentalFactors,
         const bool addSparsityPenalty,
         const bool addOrthogonalityPenalty,
		 const double sparsityThreshold,
		 const int nSkipOutermost,
         const int nIsosurfs,
         const int nMinIsoVertexes,
         const objectiveFunctionFactors& off )
{

	// A mutex to create critical sections to avoid crashes in multithreaded calls.
	std::unique_lock<std::mutex> lck (mutexObjectiveFunction, std::defer_lock);

	///Visualizing the results on the fly is optional/////////////
	lck.lock();
	static IJQuick3DViewer* q3Dv[50]{}; //initializes all elements to zero (null pointer)
	for( int i = 0; i < m; ++i){
		if( ! q3Dv[i] )
			q3Dv[i] = new IJQuick3DViewer();
		q3Dv[i]->show();
	}
	lck.unlock();
	///////////////////////////////////////////////////////////////

	int nI = originalGrid.M();
    int nJ = originalGrid.N();
    int nK = originalGrid.K();
    int n = fundamentalFactors.size();

    //Compute the vector of weights [a] = Adagger.B + (I-Adagger.A)[w]
    spectral::array va;
    {
        Eigen::MatrixXd eigenAdagger = spectral::to_2d( Adagger );
        Eigen::MatrixXd eigenB = spectral::to_2d( B );
        Eigen::MatrixXd eigenI = spectral::to_2d( I );
        Eigen::MatrixXd eigenA = spectral::to_2d( A );
        Eigen::MatrixXd eigenvw = spectral::to_2d( vectorOfParameters );
        Eigen::MatrixXd eigenva = eigenAdagger * eigenB + ( eigenI - eigenAdagger * eigenA ) * eigenvw;
        va = spectral::to_array( eigenva );
    }

	//Compute the sparsity of the solution matrix
    double sparsityPenalty = 1.0;
    if( addSparsityPenalty ){
        int nNonZeros = va.size();
        for (int i = 0; i < va.size(); ++i )
            if( std::abs(va.d_[i]) <= sparsityThreshold  )
                --nNonZeros;
        sparsityPenalty = nNonZeros/(double)va.size() * 1.0;
    }

	//Make the m geological factors (data decomposed into grids with features with different spatial correlation)
	//The geological factors are linear combinations of fundamental factors whose weights are given by the array va.
    std::vector< spectral::array > geologicalFactors;
    {
        for( int iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor){
            spectral::array geologicalFactor( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
            for( int iSVDFactor = 0; iSVDFactor < n; ++iSVDFactor){
                geologicalFactor += fundamentalFactors[iSVDFactor] * va.d_[ iGeoFactor * m + iSVDFactor ];
            }
            geologicalFactors.push_back( std::move( geologicalFactor ) );
        }
    }

	//Get the Fourier transforms of the geological factors
	std::vector< spectral::complex_array > geologicalFactorsFTs;
	{
		std::vector< spectral::array >::iterator it = geologicalFactors.begin();
		for( ; it != geologicalFactors.end(); ++it ){
			spectral::array& geologicalFactor = *it;
			spectral::complex_array tmp;
			lck.lock();                                //
			spectral::foward( tmp, geologicalFactor ); //fftw crashes when called simultaneously
			lck.unlock();                              //
			geologicalFactorsFTs.push_back( std::move( tmp ));
		}
	}

	//Get the varmaps from the FTs of the geological factors.
	std::vector< spectral::array > geologicalFactorsVarmaps;
	{
		std::vector< spectral::complex_array >::iterator it = geologicalFactorsFTs.begin();
		for( ; it != geologicalFactorsFTs.end(); ++it )
		{
			spectral::complex_array& geologicalFactorFT = *it;
			spectral::array gridVarmap( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
			{
				spectral::complex_array gridNormSquaredAndZeroPhase( nI, nJ, nK );
				for(int k = 0; k < nK; ++k) {
					for(int j = 0; j < nJ; ++j){
						for(int i = 0; i < nI; ++i){
							std::complex<double> value;
							//the scan order of fftw follows is the opposite of the GSLib convention
							int idx = k + nK * (j + nJ * i );
							//compute the complex number norm squared
							double normSquared = geologicalFactorFT.d_[idx][0]*geologicalFactorFT.d_[idx][0] +
												 geologicalFactorFT.d_[idx][1]*geologicalFactorFT.d_[idx][1];
							double phase = 0.0;
							//convert to rectangular form
							value = std::polar( normSquared, phase );
							//save the rectangular form in the grid
							gridNormSquaredAndZeroPhase.d_[idx][0] = value.real();
							gridNormSquaredAndZeroPhase.d_[idx][1] = value.imag();
						}
					}
				}
				lck.lock();                                                    //
				spectral::backward( gridVarmap, gridNormSquaredAndZeroPhase ); //fftw crashes when called simultaneously
				lck.unlock();                                                  //
			}
			//divide the varmap (due to fftw3's RFFT implementation) values by the number of cells of the grid.
			gridVarmap = gridVarmap * (1.0/(nI*nJ*nK));
			geologicalFactorsVarmaps.push_back( std::move( gridVarmap ) );
		}
	}

	//Get isocontours/isosurfaces from the varmaps.
    std::vector< vtkSmartPointer<vtkPolyData> > geolgicalFactorsVarmapsIsosurfaces;
	{
		std::vector< spectral::array >::iterator it = geologicalFactorsVarmaps.begin();
		for( int i = 0 ; it != geologicalFactorsVarmaps.end(); ++it, ++i )
		{
            // Get the geological factor's varmap.
			spectral::array& geologicalFactorVarmap = *it;
            // Get the geological factor's varmap with h=0 in the center of the grid.
			spectral::array geologicalFactorVarmapShifted = spectral::shiftByHalf( geologicalFactorVarmap );
            // Get the isocontour/isosurface.
			lck.lock(); // not all VTK algorithms are thread-safe, so put all VTK-using code in a critical zone just in case.
			vtkSmartPointer<vtkPolyData> poly;
			{
				poly = ImageJockeyUtils::computeIsosurfaces( geologicalFactorVarmapShifted,
															 nIsosurfs,
															 geologicalFactorVarmapShifted.min(),
															 geologicalFactorVarmapShifted.max() );
				// Get the isomap's bounding box.
				double bbox[6];
				poly->GetBounds( bbox );

				// Remove open isocontours/isosurfaces.
				//TODO: currently ineffective with 3D models (isosurfaces)
				ImageJockeyUtils::removeOpenPolyLines( poly );

				// Remove the non-concentric iscontours/isosurfaces.
                ImageJockeyUtils::removeNonConcentricPolyLines( poly,
																(bbox[1]+bbox[0])/2,
																(bbox[3]+bbox[2])/2,
																(bbox[5]+bbox[4])/2,
																 1.0,
                                                                 nMinIsoVertexes );
				///Visualizing the results on the fly is optional/////////////
				q3Dv[i]->clearScene();
				q3Dv[i]->display( poly, 0, 255, 255 );
				//////////////////////////////////////////////////////////////
			}
			lck.unlock();

			geolgicalFactorsVarmapsIsosurfaces.push_back( poly );
        }
	}

	// Fit ellipses to the isocontours/isosurfaces of the varmaps, computing the fitting error.
	double objectiveFunctionValue = 0.0;
    double angle_variance_mean = 0.0; // mean of the variances of the ellipses angles in the geological factors. Zero is ideal.
    double ratio_variance_mean = 0.0; // mean of the variances of the ellipses aspect ratio in the geological factors. Zero is ideal.
    double angle_mean_variance = 0.0; // variance of the means of the ellipses angles in the geological factors.  The greater, the better.
    double ratio_mean_variance = 0.0; // variance of the means of the ellipses aspect ratio in the geological factors. The greater, the better.
    {
        // Collect the ellipse angle and ratio means of each geological factor.
        std::vector<double> angle_means, ratio_means;

        // For each geological factor.
		std::vector< vtkSmartPointer<vtkPolyData> >::iterator it = geolgicalFactorsVarmapsIsosurfaces.begin();
		for( int i = 0 ; it != geolgicalFactorsVarmapsIsosurfaces.end(); ++it, ++i )
		{
			// Get the isocontours/isosurfaces.
			vtkSmartPointer<vtkPolyData> isos = *it;

			// Fit ellipses to them.
			lck.lock(); // not all VTK algorithms are thread-safe, so put all VTK-using code in a critical zone just in case.
			{
				vtkSmartPointer<vtkPolyData> ellipses = vtkSmartPointer<vtkPolyData>::New(); //nullptr == disables display of ellipses == faster execution.
				double max_error, mean_error, sum_error;
                double angle_variance, ratio_variance, angle_mean, ratio_mean;
				ImageJockeyUtils::fitEllipses( isos, ellipses, mean_error, max_error, sum_error, angle_variance, ratio_variance, angle_mean, ratio_mean, nSkipOutermost );
				objectiveFunctionValue += sum_error;
                angle_variance_mean += angle_variance;
                ratio_variance_mean += ratio_variance;
                angle_means.push_back( angle_mean );
                ratio_means.push_back( ratio_mean );
				///Visualizing the results on the fly is optional/////////////
				q3Dv[i]->display( ellipses, 255, 0, 0 );
				//////////////////////////////////////////////////////////////
			}
			lck.unlock();
		}

        // Compute the means of the variances of ellipses angle and ratio all geological factors.
        angle_variance_mean /= n;
        ratio_variance_mean /= n;

        // Compute the variances of the ellipses angle and ratio means of each geological factor.
        double unused;
        ImageJockeyUtils::getStats( angle_means, angle_mean_variance, unused );
        ImageJockeyUtils::getStats( ratio_means, ratio_mean_variance, unused );
    }

    //Compute the penalty caused by the angles between the vectors formed by the fundamental factors in each geological factor
    //The more orthogonal (angle == PI/2) the better.  Low angles result in more penalty.
    //The penalty value equals the smallest angle in radians between any pair of weights vectors.
    double orthogonalityPenalty = 1.0;
    if( addOrthogonalityPenalty ){
        //make the vectors of weights for each geological factor
        std::vector<spectral::array*> vectors;
        spectral::array* currentVector = new spectral::array();
		for( size_t i = 0; i < va.d_.size(); ++i){
            if( currentVector->size() < n )
                currentVector->d_.push_back( va.d_[i] );
            else{
                vectors.push_back( currentVector );
                currentVector = new spectral::array();
            }
        }
        //compute the penalty
		for( size_t i = 0; i < vectors.size()-1; ++i ){
			for( size_t j = i+1; j < vectors.size(); ++j ){
                spectral::array* vectorA = vectors[i];
                spectral::array* vectorB = vectors[j];
                double angle = spectral::angle( *vectorA, *vectorB );
                if( angle < orthogonalityPenalty )
                    orthogonalityPenalty = angle;
            }
        }
        orthogonalityPenalty = 1.0 - orthogonalityPenalty/1.571; //1.571 radians ~ 90 degrees
        //cleanup the vectors
		for( size_t i = 0; i < vectors.size(); ++i )
            delete vectors[i];
    }

    // Finally, return the objective function value.
    return  std::pow( objectiveFunctionValue,    off.f1 ) *
            std::pow( sparsityPenalty,           off.f2 ) *
            std::pow( orthogonalityPenalty,      off.f3 ) *
            std::pow( angle_variance_mean,       off.f4 ) *
            std::pow( ratio_variance_mean,       off.f5 ) *
            std::pow( angle_mean_variance,       off.f6 ) *
            std::pow( ratio_mean_variance,       off.f7 ) ;
}

/** Utilitary function that encapsulates variographic surface generation from
 * variogram model parameters.
 * @param gridWithGeometry A grid object whose geometry will be copied to the generated grid.
 * @param vectorOfParameters The column-vector with the free paramateres.
 * @param m The desired number of geological factors.
 */
spectral::array generateVariographicSurface(IJAbstractCartesianGrid& gridWithGeometry,
                                            const spectral::array &vectorOfParameters,
                                            const int m ){
    //get grid parameters
    int nI = gridWithGeometry.getNI();
    int nJ = gridWithGeometry.getNJ();
    int nK = gridWithGeometry.getNK();
    //create a grid compatible with the input varmap
    spectral::array variographicSurface( nI, nJ, nK, 0.0 );
    //for each geological factor
    for( int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor ){
        //create a variographic structure
        IJVariographicStructure2D varEllip(0.0, 0.0, 0.0, 0.0);
        //for each variographic parameter
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i ){
            //set it to the variographic ellipse
            varEllip.setParameter( iPar, vectorOfParameters[i] );
        }
        //make the variographic surface
        varEllip.addContributionToModelGrid( gridWithGeometry,
                                             variographicSurface,
                                             IJVariogramPermissiveModel::SPHERIC,
                                             true );
    }
    return variographicSurface;
}

/** The objective function for the optimization process (direct variographic fitting).
 * See complete theory in the program manual for in-depth explanation of the method's parameters below.
 * @param gridWithGeometry A grid object whose geometry will be copied to the generated grid with the varmap for objective function evaluation.
 * @param varmapOfInput The grid with the varmap of inpit data for comparison.
 * @param vectorOfParameters The column-vector with the free paramateres.
 * @param m The desired number of geological factors.
 * @return A distance/difference measure.
 */
double F3( IJAbstractCartesianGrid& gridWithGeometry,
           const spectral::array &varmapOfInput,
           const spectral::array &vectorOfParameters,
           const int m ){

    //get grid parameters
    int nI = gridWithGeometry.getNI();
    int nJ = gridWithGeometry.getNJ();
    int nK = gridWithGeometry.getNK();

    //generate the variogram model surface
    spectral::array variographicSurface = generateVariographicSurface( gridWithGeometry,
                                                                       vectorOfParameters,
                                                                       m );

    //compute the objective function metric
    double sum = 0.0;
    for( int k = 0; k < nK; ++k )
        for( int j = 0; j < nJ; ++j )
            for( int i = 0; i < nI; ++i )
//                if( std::abs( varmapOfInput(i,j,k) - variographicSurface(i,j,k) ) > 10.0 )
//                    sum += 1.0;

//                if( ! ImageJockeyUtils::almostEqual2sComplement( varmapOfInput(i,j,k), 0.0, 1 ) )
//                    sum += std::abs( ( varmapOfInput(i,j,k) - variographicSurface(i,j,k) ) / varmapOfInput(i,j,k) );

                  sum += std::abs( ( varmapOfInput(i,j,k) - variographicSurface(i,j,k) ) );

    sum /= varmapOfInput.size();

    // Finally, return the objective function value.
    return sum;
}

/** The objective function for the optimization process (indirect variographic fitting).
 * See complete theory in the program manual for in-depth explanation of the method's parameters below.
 * @param gridWithGeometry A grid object whose geometry will be copied to the generated grid with the varmap for objective function evaluation.
 * @param inputData The grid data for comparison.
 * @param vectorOfParameters The column-vector with the free paramateres (variogram parameters).
 * @param m The desired number of variographic nested structures.
 * @return A distance/difference measure.
 */
double F4( IJAbstractCartesianGrid& gridWithGeometry,
           const spectral::array &inputGridData,
           const spectral::array &vectorOfParameters,
           const int m ){

    //get grid parameters
    int nI = gridWithGeometry.getNI();
    int nJ = gridWithGeometry.getNJ();
    int nK = gridWithGeometry.getNK();

    //generate the variogram model surface
    spectral::array theoreticalVariographicSurface = generateVariographicSurface( gridWithGeometry,
                                                                       vectorOfParameters,
                                                                       m );

    //Compute FFT of input
    spectral::array inputFFTrealPart;
    spectral::array inputFFTimagPart;
    spectral::array inputFFTimagPhase;
    {
        spectral::array tmp( inputGridData );
        spectral::complex_array inputFFT;
        spectral::foward( inputFFT, tmp );
        inputFFTrealPart = spectral::real( inputFFT );
        inputFFTimagPart = spectral::imag( inputFFT );
        spectral::complex_array inputFFTpolar = spectral::to_polar_form( inputFFT );
        inputFFTimagPhase = spectral::imag( inputFFTpolar );
    }

    //Apply the principle of the Fourier Integral Method to obtain what would the map be
    //if it actually had the theoretical variogram model
    spectral::array mapFromTheoreticalVariogramModel( nI, nJ, nK, 0.0 );
    {
        //compute FFT of the theoretical varmap (into polar form)
        spectral::complex_array theoreticalVarMapFFT( nI, nJ, nK );
        spectral::array tmp = spectral::shiftByHalf( theoreticalVariographicSurface );
        spectral::foward( theoreticalVarMapFFT, tmp );
        spectral::complex_array theoreticalVarMapFFTpolar = spectral::to_polar_form( theoreticalVarMapFFT );
        spectral::array theoreticalVarMapFFTamplitudes = spectral::real( theoreticalVarMapFFTpolar );

        //get the square root of the amplitudes of the varmap FFT
        spectral::array theoreticalVarmapFFTamplitudesSQRT = theoreticalVarMapFFTamplitudes.sqrt();

        //convert sqrt(varmap) and the phases of the input to rectangular form
        spectral::complex_array mapFFTpolar = spectral::to_complex_array(
                                                       theoreticalVarmapFFTamplitudesSQRT,
                                                       inputFFTimagPhase
                                                    );
        spectral::complex_array mapFFT = spectral::to_rectangular_form( mapFFTpolar );

        //compute the reverse FFT to get the geological factor
        spectral::backward( mapFromTheoreticalVariogramModel, mapFFT );

        //fftw3's reverse FFT requires that the values of output be divided by the number of cells
        mapFromTheoreticalVariogramModel = mapFromTheoreticalVariogramModel / (double)( nI * nJ * nK );
    }

    //compute the objective function metric
    double sum = 0.0;
    for( int k = 0; k < nK; ++k )
        for( int j = 0; j < nJ; ++j )
            for( int i = 0; i < nI; ++i )
                sum += std::abs( ( inputGridData(i,j,k) - mapFromTheoreticalVariogramModel(i,j,k) ) );
    sum /= inputGridData.size();

    // Finally, return the objective function value.
    return sum;
}


/**
 * The code for multithreaded gradient vector calculation for objective function F().
 */
void taskOnePartialDerivative(
							   const spectral::array& vw,
							   const std::vector< int >& parameterIndexBin,
							   const double epsilon,
							   const spectral::array* gridData,
							   const spectral::array& A,
							   const spectral::array& Adagger,
							   const spectral::array& B,
							   const spectral::array& I,
							   const int m,
							   const std::vector< spectral::array >& svdFactors,
							   const spectral::complex_array& gridMagnitudeAndPhaseParts,
							   const bool addSparsityPenalty,
							   const bool addOrthogonalityPenalty,
                               const double sparsityThreshold,
							   spectral::array* gradient //output object: for some reason, the thread object constructor does not compile with non-const references.
							   ){
	std::vector< int >::const_iterator it = parameterIndexBin.cbegin();
	for(; it != parameterIndexBin.cend(); ++it ){
		int iParameter = *it;
		//Make a set of parameters slightly shifted to the right (more positive) along one parameter.
		spectral::array vwFromRight( vw );
		vwFromRight(iParameter) = vwFromRight(iParameter) + epsilon;
		//Make a set of parameters slightly shifted to the left (more negative) along one parameter.
		spectral::array vwFromLeft( vw );
		vwFromLeft(iParameter) = vwFromLeft(iParameter) - epsilon;
		//Compute (numerically) the partial derivative with respect to one parameter.
        (*gradient)(iParameter) = (F( *gridData, vwFromRight, A, Adagger, B, I, m, svdFactors, gridMagnitudeAndPhaseParts, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold )
									 -
                                   F( *gridData, vwFromLeft, A, Adagger, B, I, m, svdFactors, gridMagnitudeAndPhaseParts, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold ))
									 /
								   ( 2 * epsilon );
	}
}

/**
 * The code for multithreaded gradient vector calculation for objective function F2().
 */
void taskOnePartialDerivative2(
							   const spectral::array& vw,
							   const std::vector< int >& parameterIndexBin,
							   const double epsilon,
							   const spectral::array* gridData,
							   const spectral::array& A,
							   const spectral::array& Adagger,
							   const spectral::array& B,
							   const spectral::array& I,
							   const int m,
							   const std::vector< spectral::array >& svdFactors,
							   const bool addSparsityPenalty,
							   const bool addOrthogonalityPenalty,
                               const double sparsityThreshold,
							   const int nSkipOutermost,
							   const int nIsosurfs,
                               const int nMinIsoVertexes,
                               const objectiveFunctionFactors& off,
							   spectral::array* gradient //output object: for some reason, the thread object constructor does not compile with non-const references.
							   ){
	std::vector< int >::const_iterator it = parameterIndexBin.cbegin();
	for(; it != parameterIndexBin.cend(); ++it ){
		int iParameter = *it;
		//Make a set of parameters slightly shifted to the right (more positive) along one parameter.
		spectral::array vwFromRight( vw );
		vwFromRight(iParameter) = vwFromRight(iParameter) + epsilon;
		//Make a set of parameters slightly shifted to the left (more negative) along one parameter.
		spectral::array vwFromLeft( vw );
		vwFromLeft(iParameter) = vwFromLeft(iParameter) - epsilon;
		//Compute (numerically) the partial derivative with respect to one parameter.
        (*gradient)(iParameter) = (F2( *gridData, vwFromRight, A, Adagger, B, I, m, svdFactors, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold, nSkipOutermost, nIsosurfs, nMinIsoVertexes, off )
									 -
                                   F2( *gridData, vwFromLeft, A, Adagger, B, I, m, svdFactors, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold, nSkipOutermost, nIsosurfs, nMinIsoVertexes, off ))
									 /
								   ( 2 * epsilon );
	}
}

/**
 * The code for multithreaded gradient vector calculation for objective function F3().
 */
void taskOnePartialDerivative3(
                               const spectral::array& vw,
                               const std::vector< int >& parameterIndexBin,
                               const double epsilon,
                               IJAbstractCartesianGrid* gridWithGeometry,
                               const spectral::array& varMap,
                               const int m,
                               spectral::array* gradient //output object: for some reason, the thread object constructor does not compile with non-const references.
                               ){
    std::vector< int >::const_iterator it = parameterIndexBin.cbegin();
    for(; it != parameterIndexBin.cend(); ++it ){
        int iParameter = *it;
        //Make a set of parameters slightly shifted to the right (more positive) along one parameter.
        spectral::array vwFromRight( vw );
        vwFromRight(iParameter) = vwFromRight(iParameter) + epsilon;
        //Make a set of parameters slightly shifted to the left (more negative) along one parameter.
        spectral::array vwFromLeft( vw );
        vwFromLeft(iParameter) = vwFromLeft(iParameter) - epsilon;
        //Compute (numerically) the partial derivative with respect to one parameter.
        (*gradient)(iParameter) = (F3( *gridWithGeometry, varMap, vwFromRight, m )
                                     -
                                   F3( *gridWithGeometry, varMap, vwFromLeft , m ))
                                     /
                                   ( 2 * epsilon );
    }
}


VariographicDecompositionDialog::VariographicDecompositionDialog(const std::vector<IJAbstractCartesianGrid *> &&grids, QWidget *parent) :
    QDialog(parent),
	ui(new Ui::VariographicDecompositionDialog),
	m_grids( std::move( grids ) )
{
    ui->setupUi(this);

	//deletes dialog from memory upon user closing it
	this->setAttribute(Qt::WA_DeleteOnClose);

	setWindowTitle( "Variographic Decomposition" );

	//the combo box to choose a Cartesian grid containing a Fourier image
    m_gridSelector = new IJCartesianGridSelector( m_grids );
	ui->frmGridSelectorPlaceholder->layout()->addWidget( m_gridSelector );

	//the combo box to choose the variable
	m_variableSelector = new IJVariableSelector();
	ui->frmAttributeSelectorPlaceholder->layout()->addWidget( m_variableSelector );
	connect( m_gridSelector, SIGNAL(cartesianGridSelected(IJAbstractCartesianGrid*)),
			 m_variableSelector, SLOT(onListVariables(IJAbstractCartesianGrid*)) );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_gridSelector->onSelection( 0 );

	//get the number of threads from logical CPUs or number of free parameters (whichever is the lowest)
	ui->spinNumberOfThreads->setValue( (int)std::thread::hardware_concurrency() );
}

VariographicDecompositionDialog::~VariographicDecompositionDialog()
{
    delete ui;
}

void VariographicDecompositionDialog::doVariographicDecomposition()
{
	// Get the data objects.
	IJAbstractCartesianGrid* grid = m_gridSelector->getSelectedGrid();
	IJAbstractVariable* variable = m_variableSelector->getSelectedVariable();

	// Get the grid's dimensions.
	unsigned int nI = grid->getNI();
	unsigned int nJ = grid->getNJ();
	unsigned int nK = grid->getNK();

	// Fetch data from the data source.
	grid->dataWillBeRequested();

	// The user-given number of geological factors (m).
	int m = ui->spinNumberOfGeologicalFactors->value();

    // The user-given epsilon (useful for numerical calculus).
	double epsilon = std::pow(10, ui->spinLogEpsilon->value() );

	// The other user-given parameters
	int maxNumberOfOptimizationSteps = ui->spinMaxSteps->value();
	double initialAlpha = ui->spinInitialAlpha->value();
	int maxNumberOfAlphaReductionSteps = ui->spinMaxStepsAlphaReduction->value();
	double convergenceCriterion = std::pow(10, ui->spinConvergenceCriterion->value() );
	double infoContentToKeepForSVD = ui->spinInfoContentToKeepForSVD->value() / 100.0;
	bool addSparsityPenalty = ui->chkEnableSparsityPenalty->isChecked();
	bool addOrthogonalityPenalty = ui->chkEnableOrthogonalityPenalty->isChecked();
    double sparsityThreshold = ui->spinSparsityThreshold->value();

	//-------------------------------------------------------------------------------------------------
	//-----------------------------------PREPARATION STEPS---------------------------------------------
	//-------------------------------------------------------------------------------------------------

	// PRODUCTS: 1) grid with phase of FFT transform of the input variable.
	//           2) collection of grids with the fundamental SVD factors of the variable's varmap.
	//           3) n: number of fundamental factors.
	spectral::complex_array gridMagnitudeAndPhaseParts( nI, nJ, nK );
	std::vector< spectral::array > svdFactors;
	int n = 0;
	{
        // Perform FFT to get a grid of complex numbers in rectangular form: real part (a), imaginary part (b).
		// PRODUCTS: 2 grids: real and imaginary parts.
		spectral::complex_array gridRealAndImaginaryParts;
		{
			QProgressDialog progressDialog;
			progressDialog.setRange(0,0);
			progressDialog.show();
			progressDialog.setLabelText("Computing FFT...");
			QCoreApplication::processEvents(); //let Qt repaint widgets
			spectral::array *gridData = grid->createSpectralArray( variable->getIndexInParentGrid() );
			spectral::foward( gridRealAndImaginaryParts, *gridData );
			delete gridData;
		}

		// 1) Convert real and imaginary parts to magnitude and phase (phi).
		// 2) Make ||z|| = zz* = (a+bi)(a-bi) = a^2+b^2 (Convolution Theorem: a convolution reduces to a cell-to-cell product in frequency domain).
		// 3) RFFT of ||z|| as magnitude and a grid filled with zeros as phase (zero phase).
		// PRODUCTS: 3 grids: magnitude and phase parts; variographic map.
		spectral::array gridVarmap( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
		{
			QProgressDialog progressDialog;
			progressDialog.setRange(0,0);
			progressDialog.show();
			progressDialog.setLabelText("Converting FFT results to polar form...");
			spectral::complex_array gridNormSquaredAndZeroPhase( nI, nJ, nK );
			QCoreApplication::processEvents(); //let Qt repaint widgets
			for(unsigned int k = 0; k < nK; ++k) {
				for(unsigned int j = 0; j < nJ; ++j){
					for(unsigned int i = 0; i < nI; ++i){
						std::complex<double> value;
						//the scan order of fftw follows is the opposite of the GSLib convention
						int idx = k + nK * (j + nJ * i );
						//get the complex number values
						value.real( gridRealAndImaginaryParts.d_[idx][0] ); //real part
						value.imag( gridRealAndImaginaryParts.d_[idx][1] ); //imaginary part
						//fills the output array with the angular form
						gridMagnitudeAndPhaseParts.d_[idx][0] = std::abs( value ); //magnitude part
						gridMagnitudeAndPhaseParts.d_[idx][1] = std::arg( value ); //phase part
						//compute the complex number norm squared
						double normSquared = gridRealAndImaginaryParts.d_[idx][0]*gridRealAndImaginaryParts.d_[idx][0] +
											 gridRealAndImaginaryParts.d_[idx][1]*gridRealAndImaginaryParts.d_[idx][1];
						double phase = 0.0;
						//convert to rectangular form
                        value = std::polar( normSquared, phase );
						//save the rectangular form in the grid
						gridNormSquaredAndZeroPhase.d_[idx][0] = value.real();
						gridNormSquaredAndZeroPhase.d_[idx][1] = value.imag();
					}
				}
			}
			progressDialog.setLabelText("Computing RFFT to get varmap...");
			QCoreApplication::processEvents(); //let Qt repaint widgets
			spectral::backward( gridVarmap, gridNormSquaredAndZeroPhase );
		}

        //divide the varmap (due to fftw3's RFFT implementation) values by the number of cells of the grid.
        gridVarmap = gridVarmap * (1.0/(nI*nJ*nK));

		//Compute SVD of varmap
		{
			//Get the number of usable fundamental SVD factors.
			{
				QProgressDialog progressDialog;
				progressDialog.setRange(0,0);
				progressDialog.setLabelText("Computing SVD factors of varmap...");
				progressDialog.show();
				QCoreApplication::processEvents();
				doSVDonData( &gridVarmap, infoContentToKeepForSVD, svdFactors );
				n = svdFactors.size();
				progressDialog.hide();
			}
		}
	}

	//---------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------SET UP THE LINEAR SYSTEM TO IMPOSE THE CONSERVATION CONSTRAINTS---------------------
	//---------------------------------------------------------------------------------------------------------------------------

	//Make the A matrix of the linear system originated by the information
	//conservation constraint (see program manual for the complete theory).
	//elements are initialized to zero.
	spectral::array A( (spectral::index)n, (spectral::index)m*n );
	for( int line = 0; line < n; ++line )
		for( int column = line * m; column < ((line+1) * m); ++column)
			 A( line, column ) = 1.0;

	//Make the B matrix of said system
	spectral::array B( (spectral::index)n, (spectral::index)1 );
	for( int line = 0; line < n; ++line )
		B( line, 0 ) = 1.0;

    //Make the identity matrix to find the fundamental factors weights [a] from the parameters w:
    //     [a] = Adagger.B + (I-Adagger.A)[w]
	spectral::array I( (spectral::index)m*n, (spectral::index)m*n );
	for( int i = 0; i < m*n; ++i )
		I( i, i ) = 1.0;

	//Get the U, Sigma and V* matrices from SVD on A.
	spectral::SVD svd = spectral::svd( A );
	spectral::array U = svd.U();
	spectral::array Sigma = svd.S();
    spectral::array V = svd.V(); //SVD yields V* already transposed, that is V, but to check, you must transpose V
                                 //to get A = U.Sigma.V*

	//Make a full Sigma matrix (to be compatible with multiplication with the other matrices)
	{
		spectral::array SigmaTmp( (spectral::index)n, (spectral::index)m*n );
		for( int i = 0; i < n; ++i)
			SigmaTmp(i, i) = Sigma.d_[i];
		Sigma = SigmaTmp;
	}

	//Make U*
	//U contains only real numbers, thus U's transpose conjugate is its transpose.
	spectral::array Ustar( U );
	//transpose U to get U*
	{
		Eigen::MatrixXd tmp = spectral::to_2d( Ustar );
		tmp.transposeInPlace();
		Ustar = spectral::to_array( tmp );
	}

	//Make Sigmadagger (pseudoinverse of Sigma)
	spectral::array SigmaDagger( Sigma );
	{
		//compute reciprocals of the non-zero elements in the main diagonal.
		for( int i = 0; i < n; ++i){
			double value = SigmaDagger(i, i);
			//only absolute values greater than the machine epsilon are considered non-zero.
			if( std::abs( value ) > std::numeric_limits<double>::epsilon() )
				SigmaDagger( i, i ) = 1.0 / value;
			else
				SigmaDagger( i, i ) = 0.0;
		}
		//transpose
		Eigen::MatrixXd tmp = spectral::to_2d( SigmaDagger );
		tmp.transposeInPlace();
		SigmaDagger = spectral::to_array( tmp );
	}

	//Make Adagger (pseudoinverse of A) by "reversing" the transform U.Sigma.V*,
	//hence, Adagger = V.Sigmadagger.Ustar .
	spectral::array Adagger;
	{
		Eigen::MatrixXd eigenV = spectral::to_2d( V );
		Eigen::MatrixXd eigenSigmadagger = spectral::to_2d( SigmaDagger );
		Eigen::MatrixXd eigenUstar = spectral::to_2d( Ustar );
		Eigen::MatrixXd eigenAdagger = eigenV * eigenSigmadagger * eigenUstar;
		Adagger = spectral::to_array( eigenAdagger );
	}

    //Initialize the vector of linear system parameters [w]=[0]
	spectral::array vw( (spectral::index)m*n );

	//---------------------------------------------------------------------------------------------------------------
	//-------------------------SIMULATED ANNEALING TO INITIALIZE THE PARAMETERS [w] NEAR A GLOBAL MINIMUM------------
	//---------------------------------------------------------------------------------------------------------------
	{
		//...................................Annealing Parameters.................................
		//Intial temperature.
		double f_Tinitial = ui->spinInitialTemperature->value();
		//Final temperature.
		double f_Tfinal = ui->spinFinalTemperature->value();
		//Max number of SA steps.
		int i_kmax = ui->spinMaxStepsSA->value();
		//Minimum value allowed for the parameters w (all zeros). DOMAIN CONSTRAINT
		spectral::array L_wMin( vw.size(), 0.0d );
		//Maximum value allowed for the parameters w (all ones). DOMAIN CONSTRAINT
		spectral::array L_wMax( vw.size(), 1.0d );
		/*Factor used to control the size of the random state “hop”.  For example, if the maximum “hop” must be
		 10% of domain size, set 0.1.  Small values (e.g. 0.001) result in slow, but more accurate convergence.
		 Large values (e.g. 100.0) covers more space faster, but falls outside the domain are more frequent,
		 resulting in more re-searches due to more invalid parameter value penalties. */
		double f_factorSearch = ui->spinMaxHopFactor->value();
		//Intialize the random number generator with the same seed
		std::srand ((unsigned)ui->spinSeed->value());
		//.................................End of Annealing Parameters.............................
		//Returns the current “temperature” of the system.  It yields a log curve that decays as the step number increase.
		// The initial temperature plays an important role: curve starting with 5.000 is steeper than another that starts with 1.000.
		//  This means the the lower the temperature, the more linear the temperature decreases.
		// i_stepNumber: the current step number of the annealing process ( 0 = first ).
		auto temperature = [=](int i_stepNumber) { return f_Tinitial * std::exp( -i_stepNumber / (double)1000 * (1.5 * std::log10( f_Tinitial ) ) ); };
		/*Returns the probability of acceptance of the energy state for the next iteration.
		  This allows acceptance of higher values to break free from local minima.
		  f_eCurrent: current energy of the system.
		  f_eNew: energy level of the next step.
		  f_T: current “temperature” of the system.*/
		auto probAcceptance = [=]( double f_eCurrent, double f_eNewLocal, double f_T ) {
		   //If the new state is more energetic, calculates a probability of acceptance
		   //which is as high as the current “temperature” of the process.  The “temperature”
		   //diminishes with iterations.
		   if ( f_eNewLocal > f_eCurrent )
			  return ( f_T - f_Tfinal ) / ( f_Tinitial - f_Tfinal );
		   //If the new state is less energetic, the probability of acceptance is 100% (natural search for minima).
		   else
			  return 1.0;
		};
		//Get the number of parameters.
		int i_nPar = vw.size();
		//Make a copy of the initial state (parameter set.
		spectral::array L_wCurrent( vw );
		//The parameters variations (maxes - mins)
		spectral::array L_wDelta = L_wMax - L_wMin;
		//Get the input data.
		spectral::array *gridData = grid->createSpectralArray( variable->getIndexInParentGrid() );
		//...................Main annealing loop...................
		QProgressDialog progressDialog;
		progressDialog.setRange(0,0);
		progressDialog.show();
		progressDialog.setLabelText("Simulated Annealing in progress...");
		QCoreApplication::processEvents();
		double f_eNew = std::numeric_limits<double>::max();
		double f_lowestEnergyFound = std::numeric_limits<double>::max();
		spectral::array L_wOfLowestEnergyFound;
		int k = 0;
		for( ; k < i_kmax; ++k ){
			emit info( "Commencing SA step #" + QString::number( k ) );
			//Get current temperature.
			double f_T = temperature( k );
			//Quit if temperature is lower than the minimum annealing temperature.
			if( f_T < f_Tfinal )
				break;
			//Randomly searches for a neighboring state with respect to current state.
			spectral::array L_wNew(L_wCurrent);
			for( int i = 0; i < i_nPar; ++i ){ //for each parameter
			   //Ensures that the values randomly obtained are inside the domain.
			   double f_tmp = 0.0;
			   while( true ){
				  double LO = L_wCurrent[i] - (f_factorSearch * L_wDelta[i]);
				  double HI = L_wCurrent[i] + (f_factorSearch * L_wDelta[i]);
				  f_tmp = LO + std::rand() / (RAND_MAX/(HI-LO)) ;
				  if ( f_tmp >= L_wMin[i] && f_tmp <= L_wMax[i] )
					 break;
			   }
			   //Updates the parameter value.
			   L_wNew[i] = f_tmp;
			}
			//Computes the “energy” of the current state (set of parameters).
			//The “energy” in this case is how different the image as given the parameters is with respect
			//the data grid, considered the reference image.
            double f_eCurrent = F( *gridData, L_wCurrent, A, Adagger, B, I, m, svdFactors, gridMagnitudeAndPhaseParts, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold );
			//Computes the “energy” of the neighboring state.
            f_eNew = F( *gridData, L_wNew, A, Adagger, B, I, m, svdFactors, gridMagnitudeAndPhaseParts, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold );
			//Changes states stochastically.  There is a probability of acceptance of a more energetic state so
			//the optimization search starts near the global minimum and is not trapped in local minima (hopefully).
			double f_probMov = probAcceptance( f_eCurrent, f_eNew, f_T );
			if( f_probMov >= ( (double)std::rand() / RAND_MAX ) ) {//draws a value between 0.0 and 1.0
				L_wCurrent = L_wNew; //replaces the current state with the neighboring random state
				emit info("  moved to energy level " + QString::number( f_eNew ));
				//if the energy is the record low, store it, just in case the SA loop ends without converging.
				if( f_eNew < f_lowestEnergyFound ){
					f_lowestEnergyFound = f_eNew;
					L_wOfLowestEnergyFound = spectral::array( L_wCurrent );
				}
			}
		}
		// The input data is no longer necessary.
		delete gridData;
		// Delivers the set of parameters near the global minimum (hopefully) for the Gradient Descent algorithm.
		// The SA loop may end in a higher energy state, so we return the lowest found in that case
		if( k == i_kmax && f_lowestEnergyFound < f_eNew )
			emit info( "SA completed by number of steps." );
		else
			emit info( "SA completed by reaching the lowest temperature." );
		vw = L_wOfLowestEnergyFound;
		emit info( "Using the state of lowest energy found (" + QString::number( f_lowestEnergyFound ) + ")" );
	}

	//---------------------------------------------------------------------------------------------------------
	//--------------------------------------OPTIMIZATION LOOP (GRADIENT DESCENT)-------------------------------
	//---------------------------------------------------------------------------------------------------------
	unsigned int nThreads = ui->spinNumberOfThreads->value();
	QProgressDialog progressDialog;
	progressDialog.setRange(0,0);
	progressDialog.show();
	progressDialog.setLabelText("Gradient Descent in progress...");
	QCoreApplication::processEvents();
	int iOptStep = 0;
	spectral::array va;
	for( ; iOptStep < maxNumberOfOptimizationSteps; ++iOptStep ){

		emit info( "Commencing GD step #" + QString::number( iOptStep ) );

		//Compute the vector of weights [a] = Adagger.B + (I-Adagger.A)[w] (see program manual for theory)
        {
            Eigen::MatrixXd eigenAdagger = spectral::to_2d( Adagger );
            Eigen::MatrixXd eigenB = spectral::to_2d( B );
            Eigen::MatrixXd eigenI = spectral::to_2d( I );
            Eigen::MatrixXd eigenA = spectral::to_2d( A );
            Eigen::MatrixXd eigenvw = spectral::to_2d( vw );
			Eigen::MatrixXd eigenva = eigenAdagger * eigenB + ( eigenI - eigenAdagger * eigenA ) * eigenvw;
            va = spectral::to_array( eigenva );
        }

		{
			spectral::array debug_va( va );
			debug_va.set_size( (spectral::index)n, (spectral::index)m );
		}

        //Compute the gradient vector of objective function F with the current [w] parameters.
        spectral::array gradient( vw.size() );
        {
            spectral::array *gridData = grid->createSpectralArray( variable->getIndexInParentGrid() );

			//distribute the parameter indexes among the n-threads
			std::vector<int> parameterIndexBins[nThreads];
			int parameterIndex = 0;
			for( unsigned int iThread = 0; parameterIndex < vw.size(); ++parameterIndex, ++iThread)
				parameterIndexBins[ iThread % nThreads ].push_back( parameterIndex );

			//create and run the partial derivative calculation threads
			std::thread threads[nThreads];
			for( unsigned int iThread = 0; iThread < nThreads; ++iThread){
				threads[iThread] = std::thread( taskOnePartialDerivative,
												vw,
												parameterIndexBins[iThread],
												epsilon,
												gridData,
												A,
												Adagger,
												B,
												I,
												m,
												svdFactors,
												gridMagnitudeAndPhaseParts,
												addSparsityPenalty,
												addOrthogonalityPenalty,
                                                sparsityThreshold,
												&gradient);
			}

			//wait for the threads to finish.
			for( unsigned int iThread = 0; iThread < nThreads; ++iThread)
				threads[iThread].join();

            delete gridData;
        }

        //Update the system's parameters according to gradient descent.
		double currentF = 999.0;
		double nextF = 1.0;
		{
            spectral::array *gridData = grid->createSpectralArray( variable->getIndexInParentGrid() );
			double alpha = initialAlpha;
			//halves alpha until we get a descent (current gradient vector may result in overshooting)
			int iAlphaReductionStep = 0;
			for( ; iAlphaReductionStep < maxNumberOfAlphaReductionSteps; ++iAlphaReductionStep ){
                spectral::array new_vw = vw - gradient * alpha;
                //Impose domain constraints to the parameters.
                for( int i = 0; i < new_vw.size(); ++i){
					if( new_vw.d_[i] < 0.0 )
                        new_vw.d_[i] = 0.0;
                    if( new_vw.d_[i] > 1.0 )
                        new_vw.d_[i] = 1.0;
                }
                currentF = F( *gridData, vw, A, Adagger, B, I, m, svdFactors, gridMagnitudeAndPhaseParts, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold );
                nextF = F( *gridData, new_vw, A, Adagger, B, I, m, svdFactors, gridMagnitudeAndPhaseParts, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold );
                if( nextF < currentF ){
                    vw = new_vw;
                    break;
                }
                alpha /= 2.0;
            }
			if( iAlphaReductionStep == maxNumberOfAlphaReductionSteps )
				emit warning( "WARNING: reached maximum alpha reduction steps." );
            delete gridData;
        }

		//Check the convergence criterion.
		double ratio = currentF / nextF;
		if( ratio  < (1.0 + convergenceCriterion) )
			break;

		emit info( "F(k)/F(k+1) ratio: " + QString::number( ratio ) );

	}
	progressDialog.hide();

	//-------------------------------------------------------------------------------------------------
	//------------------------------------PRESENT THE RESULTS------------------------------------------
	//-------------------------------------------------------------------------------------------------

	if( iOptStep == maxNumberOfOptimizationSteps )
		QMessageBox::warning( this, "Warning", "Completed by reaching maximum number of optimization steps. Check results.");
	else
		QMessageBox::information( this, "Info", "Completed by satisfaction of convergence criterion.");

	std::vector< spectral::array > grids;
	std::vector< std::string > titles;
	std::vector< bool > shiftByHalves;

	spectral::array derivedGrid( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
	{
		//Make the m geological factors (expected variographic structures)
		std::vector< spectral::array > geologicalFactors;
		{
			QProgressDialog progressDialog;
			progressDialog.setRange(0,0);
			progressDialog.show();
			progressDialog.setLabelText("Making the geological factors...");
			QCoreApplication::processEvents();
			for( int iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor){
				spectral::array geologicalFactor( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
				for( int iSVDFactor = 0; iSVDFactor < n; ++iSVDFactor){
					double weight = va.d_[ iGeoFactor * m + iSVDFactor ];
					geologicalFactor += svdFactors[iSVDFactor] * weight;
				}
				geologicalFactors.push_back( std::move( geologicalFactor ) );
			}
		}

		//Compute the grid derived form the geological factors (ideally it must match the input grid)
		{
			//Sum up all geological factors.
			spectral::array sum( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
			std::vector< spectral::array >::iterator it = geologicalFactors.begin();
			for( ; it != geologicalFactors.end(); ++it ){
				sum += *it;
			}
			//Compute FFT of the sum
			spectral::complex_array tmp;
			spectral::foward( tmp, sum ); //fftw crashes when called simultaneously
			//inbue the sum's FFT with the phase field of the original data.
			for( int idx = 0; idx < tmp.size(); ++idx)
			{
				std::complex<double> value;
				//get the complex number value in rectangular form
				value.real( tmp.d_[idx][0] ); //real part
				value.imag( tmp.d_[idx][1] ); //imaginary part
				//convert to polar form
				//but phase is replaced with that of the original data
				tmp.d_[idx][0] = std::abs( value ); //magnitude part
				tmp.d_[idx][1] = gridMagnitudeAndPhaseParts.d_[idx][1]; //std::arg( value ); //phase part (this should be zero all over the grid)
				//convert back to rectangular form (recall that the varmap holds covariance values, then it is necessary to take its square root)
				value = std::polar( std::sqrt(tmp.d_[idx][0]), tmp.d_[idx][1] );
				tmp.d_[idx][0] = value.real();
				tmp.d_[idx][1] = value.imag();
			}
			//Compute RFFT (with the phase of the original data imbued)
			spectral::array rfftResult( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
			spectral::backward( rfftResult, tmp ); //fftw crashes when called simultaneously
			//Divide the RFFT result (due to fftw3's RFFT implementation) by the number of grid cells
			rfftResult = rfftResult * (1.0/(nI*nJ*nK));
			derivedGrid += rfftResult;
		}

		//Change the weights m*n vector to a n by m matrix for displaying (fundamental factors as columns and geological factors as lines)
		va.set_size( (spectral::index)n, (spectral::index)m );
		displayGrids( {va}, {"parameters"}, {false} );

		//Collect the geological factors (variographic structures and reconstructed information)
		//as separate grids for display.
		std::vector< spectral::array >::iterator it = geologicalFactors.begin();
		for( int iGeoFactor = 0; it != geologicalFactors.end(); ++it, ++iGeoFactor ){
			//Compute FFT of the geological factor
			spectral::complex_array tmp;
			spectral::foward( tmp, *it );
			//inbue the geological factor's FFT with the phase field of the original data.
			for( int idx = 0; idx < tmp.size(); ++idx)
			{
				std::complex<double> value;
				//get the complex number value in rectangular form
				value.real( tmp.d_[idx][0] ); //real part
				value.imag( tmp.d_[idx][1] ); //imaginary part
				//convert to polar form
				//but phase is replaced with that of the original data
				tmp.d_[idx][0] = std::abs( value ); //magnitude part
				tmp.d_[idx][1] = gridMagnitudeAndPhaseParts.d_[idx][1]; //std::arg( value ); //phase part (this should be zero all over the grid)
				//convert back to rectangular form
				value = std::polar( std::sqrt(tmp.d_[idx][0]), tmp.d_[idx][1] );
				tmp.d_[idx][0] = value.real();
				tmp.d_[idx][1] = value.imag();
			}
			//Compute RFFT (with the phase of the original data imbued)
			spectral::array rfftResult( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
			spectral::backward( rfftResult, tmp );
			//Divide the RFFT result (due to fftw3's RFFT implementation) by the number of grid cells
			rfftResult = rfftResult * (1.0/(nI*nJ*nK));
			//Collect the grids.
			QString title = QString("Factor #") + QString::number(iGeoFactor+1);
			titles.push_back( title.toStdString() );
			grids.push_back( std::move( rfftResult ) );
			shiftByHalves.push_back( false );
			title = QString("Variographic structure #") + QString::number(iGeoFactor+1);
			titles.push_back( title.toStdString() );
			grids.push_back( std::move( *it ) );
			shiftByHalves.push_back( true );
		}
	}

	//Display the derived grid and its difference with respect to the original grid.
	//Also display the geological factors and their resulting partial grids.
	{
		spectral::array *gridData = grid->createSpectralArray( variable->getIndexInParentGrid() );
		spectral::array diff = *gridData - derivedGrid;
		grids.push_back( *gridData ); titles.push_back( "Original grid"); shiftByHalves.push_back( false );
		grids.push_back( derivedGrid ); titles.push_back( "Derived grid"); shiftByHalves.push_back( false );
		grids.push_back( diff ); titles.push_back( "difference"); shiftByHalves.push_back( false );
		displayGrids( grids, titles, shiftByHalves );
		delete gridData;
	}
}

void VariographicDecompositionDialog::displayGrids(const std::vector<spectral::array> &grids,
												   const std::vector<std::string> &titles,
												   const std::vector<bool> & shiftByHalves)
{
    //Create the structure to store the geological factors
    SVDFactorTree * factorTree = new SVDFactorTree( 0.0 ); //the split factor of 0.0 has no special meaning here
    //Populate the factor container with the geological factors.
    std::vector< spectral::array >::const_iterator it = grids.begin();
    std::vector< std::string >::const_iterator itTitles = titles.begin();
	std::vector< bool >::const_iterator itShiftByHalves = shiftByHalves.begin();
	for(int i = 1; it != grids.end(); ++it, ++i, ++itTitles, ++itShiftByHalves){
        //make a local copy of the geological factor data
        spectral::array geoFactorDataCopy;
		if( *itShiftByHalves )
            geoFactorDataCopy = spectral::shiftByHalf( *it );
        else
            geoFactorDataCopy = spectral::array( *it );
        //Create a displayble object from the geological factor data
        //This pointer will be managed by the SVDFactorTree object.
        SVDFactor* geoFactor = new SVDFactor( std::move(geoFactorDataCopy), i, 1/(grids.size()),
                                           0, 0, 0, 1, 1, 1, 0.0);
        //Declare it as a geological factor (decomposable, not fundamental)
        geoFactor->setType( SVDFactorType::GEOLOGICAL );
        geoFactor->setCustomName( QString( (*itTitles).c_str() ) );
        //add the displayable object to the factor tree (container)
        factorTree->addFirstLevelFactor( geoFactor );
    }
    //use the SVD analysis dialog to display the geological factors.
    //NOTE: do not use heap to allocate the dialog, unless you remove the Qt::WA_DeleteOnClose behavior of the dialog.
	SVDAnalysisDialog* svdad = new SVDAnalysisDialog( this );
    svdad->setTree( factorTree );
    svdad->setDeleteTreeOnClose( true ); //the three and all data it contains will be deleted on dialog close
    connect( svdad, SIGNAL(sumOfFactorsComputed(spectral::array*)),
             this, SLOT(onSumOfFactorsWasComputed(spectral::array*)) );
    svdad->exec(); //open the dialog modally
}

void VariographicDecompositionDialog::onSumOfFactorsWasComputed(spectral::array *gridData)
{
    IJAbstractCartesianGrid* grid = m_gridSelector->getSelectedGrid();
    emit saveArray( gridData, grid );
}

void VariographicDecompositionDialog::doVariographicDecomposition2(
        FundamentalFactorType fundamentalFactorType )
{
    // Get the data objects.
    IJAbstractCartesianGrid* grid = m_gridSelector->getSelectedGrid();
    IJAbstractVariable* variable = m_variableSelector->getSelectedVariable();

    // Get the grid's dimensions.
    unsigned int nI = grid->getNI();
    unsigned int nJ = grid->getNJ();
    unsigned int nK = grid->getNK();

    // Fetch data from the data source.
    grid->dataWillBeRequested();

    // The user-given number of geological factors (m).
    int m = ui->spinNumberOfGeologicalFactors->value();

    // The user-given epsilon (useful for numerical calculus).
    double epsilon = std::pow(10, ui->spinLogEpsilon->value() );

    // The other user-given parameters
    int maxNumberOfOptimizationSteps = ui->spinMaxSteps->value();
    double initialAlpha = ui->spinInitialAlpha->value();
    int maxNumberOfAlphaReductionSteps = ui->spinMaxStepsAlphaReduction->value();
    double convergenceCriterion = std::pow(10, ui->spinConvergenceCriterion->value() );
    double infoContentToKeepForSVD = ui->spinInfoContentToKeepForSVD->value() / 100.0;
    bool addSparsityPenalty = ui->chkEnableSparsityPenalty->isChecked();
    bool addOrthogonalityPenalty = ui->chkEnableOrthogonalityPenalty->isChecked();
    double sparsityThreshold = ui->spinSparsityThreshold->value();
	int nTracks = ui->spinNumberOfSpectrumTracks->value();
	int nSkipOutermost = ui->spinSkipOuterNIsolinesEllipseFitting->value();
	int nIsosurfs = ui->spinNumberOfIsolines->value();
    int nMinIsoVertexes = ui->spinIsoMinVertexes->value();
    bool allowNegativeWeights = ui->chkAllowNegativeWeights->isChecked();
    objectiveFunctionFactors off;
    {
        off.f1 = ui->spinOJFactor_1->value();
        off.f2 = ui->spinOJFactor_2->value();
        off.f3 = ui->spinOJFactor_3->value();
        off.f4 = ui->spinOJFactor_4->value();
        off.f5 = ui->spinOJFactor_5->value();
        off.f6 = ui->spinOJFactor_6->value();
        off.f7 = ui->spinOJFactor_7->value();
    }

    GaborAnalysisParameters gaborParameters;
    gaborParameters.initialFrequency = 0.003;
    gaborParameters.finalFrequency = 0.09;
    gaborParameters.frequencyStep = 0.01; //orig = 0.0001
    gaborParameters.azimuthStep = 15.0;
    gaborParameters.kernelSize = 50;
    gaborParameters.kernelMeanMajorAxis = 127.5; //max. resolution size is 255, this 127.5 is in the middle
    gaborParameters.kernelMeanMinorAxis = 127.5;
    gaborParameters.kernelSigmaMajorAxis = 50.0;
    gaborParameters.kernelSigmaMinorAxis = 50.0;


    //-------------------------------------------------------------------------------------------------
    //-----------------------------------PREPARATION STEPS---------------------------------------------
    //-------------------------------------------------------------------------------------------------

	// PRODUCT: a collection of grids with the fundamental factors of the variable.
	std::vector< spectral::array > fundamentalFactors;
    int n = 0;
    {
		//Atom learning method: SVD of input variable.
        if( fundamentalFactorType == FundamentalFactorType::SVD_SINGULAR_FACTOR ){
			//Get the number of usable fundamental factors.
			QProgressDialog progressDialog;
			progressDialog.setRange(0,0);
			progressDialog.setLabelText("Computing SVD factors of input data...");
			progressDialog.show();
			QCoreApplication::processEvents();
			spectral::array* gridInputData = grid->createSpectralArray( variable->getIndexInParentGrid() );
			doSVDonData( gridInputData, infoContentToKeepForSVD, fundamentalFactors );
			delete gridInputData;
			n = fundamentalFactors.size();
		//Atom learning method: frequency spectrum partitioning of input variable.
        } else if ( fundamentalFactorType == FundamentalFactorType::FFT_SPECTRUM_PARTITION ) {
			QProgressDialog progressDialog;
			progressDialog.setRange(0,0);
			progressDialog.setLabelText("Computing Fourier partitioning of input data...");
			progressDialog.show();
			QCoreApplication::processEvents();
			spectral::array* gridInputData = grid->createSpectralArray( variable->getIndexInParentGrid() );
			doFourierPartitioningOnData( gridInputData, fundamentalFactors, nTracks );
			delete gridInputData;
			n = fundamentalFactors.size();
        //Atom learning method: Gabor analysis of input variable.
        } else {
            QProgressDialog progressDialog;
            progressDialog.setRange(0,0);
            progressDialog.setLabelText("Computing Gabor analysis of input data...");
            progressDialog.show();
            QCoreApplication::processEvents();
            spectral::array* gridInputData = grid->createSpectralArray( variable->getIndexInParentGrid() );
            doGaborAnalysisOnData( gridInputData, fundamentalFactors, gaborParameters );
            delete gridInputData;
            n = fundamentalFactors.size();
        }
    }

	if( n == 0 ){
		emit error( "VariographicDecompositionDialog::doVariographicDecomposition2(): No fundamental factors. Aborted." );
		return;
	}

    //---------------------------------------------------------------------------------------------------------------------------
    //---------------------------------------SET UP THE LINEAR SYSTEM TO IMPOSE THE CONSERVATION CONSTRAINTS---------------------
    //---------------------------------------------------------------------------------------------------------------------------

    //Make the A matrix of the linear system originated by the information
    //conservation constraint (see program manual for the complete theory).
    //elements are initialized to zero.
    spectral::array A( (spectral::index)n, (spectral::index)m*n );
    for( int line = 0; line < n; ++line )
        for( int column = line * m; column < ((line+1) * m); ++column)
             A( line, column ) = 1.0;

    //Make the B matrix of said system
    spectral::array B( (spectral::index)n, (spectral::index)1 );
    for( int line = 0; line < n; ++line )
        B( line, 0 ) = 1.0;

    //Make the identity matrix to find the fundamental factors weights [a] from the parameters w:
    //     [a] = Adagger.B + (I-Adagger.A)[w]
    spectral::array I( (spectral::index)m*n, (spectral::index)m*n );
    for( int i = 0; i < m*n; ++i )
        I( i, i ) = 1.0;

    //Get the U, Sigma and V* matrices from SVD on A.
    spectral::SVD svd = spectral::svd( A );
    spectral::array U = svd.U();
    spectral::array Sigma = svd.S();
    spectral::array V = svd.V(); //SVD yields V* already transposed, that is V, but to check, you must transpose V
                                 //to get A = U.Sigma.V*

    //Make a full Sigma matrix (to be compatible with multiplication with the other matrices)
    {
        //All elementes are initialized with zeros
        spectral::array SigmaTmp( (spectral::index)n, (spectral::index)m*n );
        for( int i = 0; i < n; ++i)
            SigmaTmp(i, i) = Sigma.d_[i];
        Sigma = SigmaTmp;
    }

    //Make U*
    //U contains only real numbers, thus U's transpose conjugate is its transpose.
    spectral::array Ustar( U );
    //transpose U to get U*
    {
        Eigen::MatrixXd tmp = spectral::to_2d( Ustar );
        tmp.transposeInPlace();
        Ustar = spectral::to_array( tmp );
    }

    //Make Sigmadagger (pseudoinverse of Sigma)
    spectral::array SigmaDagger( Sigma );
    {
        //compute reciprocals of the non-zero elements in the main diagonal.
        for( int i = 0; i < n; ++i){
            double value = SigmaDagger(i, i);
            //only absolute values greater than the machine epsilon are considered non-zero.
            if( std::abs( value ) > std::numeric_limits<double>::epsilon() )
                SigmaDagger( i, i ) = 1.0 / value;
            else
                SigmaDagger( i, i ) = 0.0;
        }
        //transpose
        Eigen::MatrixXd tmp = spectral::to_2d( SigmaDagger );
        tmp.transposeInPlace();
        SigmaDagger = spectral::to_array( tmp );
    }

    //Make Adagger (pseudoinverse of A) by "reversing" the transform U.Sigma.V*,
    //hence, Adagger = V.Sigmadagger.Ustar .
    spectral::array Adagger;
    {
        Eigen::MatrixXd eigenV = spectral::to_2d( V );
        Eigen::MatrixXd eigenSigmadagger = spectral::to_2d( SigmaDagger );
        Eigen::MatrixXd eigenUstar = spectral::to_2d( Ustar );
        Eigen::MatrixXd eigenAdagger = eigenV * eigenSigmadagger * eigenUstar;
        Adagger = spectral::to_array( eigenAdagger );
    }

    //Initialize the vector of linear system parameters [w]=[0]
    spectral::array vw( (spectral::index)m*n );

    //---------------------------------------------------------------------------------------------------------------
    //-------------------------SIMULATED ANNEALING TO INITIALIZE THE PARAMETERS [w] NEAR A GLOBAL MINIMUM------------
    //---------------------------------------------------------------------------------------------------------------
    {
        //...................................Annealing Parameters.................................
        //Intial temperature.
        double f_Tinitial = ui->spinInitialTemperature->value();
        //Final temperature.
        double f_Tfinal = ui->spinFinalTemperature->value();
        //Max number of SA steps.
        int i_kmax = ui->spinMaxStepsSA->value();
        //Minimum value allowed for the parameters w (all zeros). DOMAIN CONSTRAINT
        spectral::array L_wMin( vw.size(), 0.0d );
        //Maximum value allowed for the parameters w (all ones). DOMAIN CONSTRAINT
        spectral::array L_wMax( vw.size(), 1.0d );
        /*Factor used to control the size of the random state “hop”.  For example, if the maximum “hop” must be
         10% of domain size, set 0.1.  Small values (e.g. 0.001) result in slow, but more accurate convergence.
         Large values (e.g. 100.0) covers more space faster, but falls outside the domain are more frequent,
         resulting in more re-searches due to more invalid parameter value penalties. */
        double f_factorSearch = ui->spinMaxHopFactor->value();
        //Intialize the random number generator with the same seed
        std::srand ((unsigned)ui->spinSeed->value());
        //.................................End of Annealing Parameters.............................
        //Returns the current “temperature” of the system.  It yields a log curve that decays as the step number increase.
        // The initial temperature plays an important role: curve starting with 5.000 is steeper than another that starts with 1.000.
        //  This means the the lower the temperature, the more linear the temperature decreases.
        // i_stepNumber: the current step number of the annealing process ( 0 = first ).
        auto temperature = [=](int i_stepNumber) { return f_Tinitial * std::exp( -i_stepNumber / (double)1000 * (1.5 * std::log10( f_Tinitial ) ) ); };
        /*Returns the probability of acceptance of the energy state for the next iteration.
          This allows acceptance of higher values to break free from local minima.
          f_eCurrent: current energy of the system.
          f_eNew: energy level of the next step.
          f_T: current “temperature” of the system.*/
        auto probAcceptance = [=]( double f_eCurrent, double f_eNewLocal, double f_T ) {
           //If the new state is more energetic, calculates a probability of acceptance
           //which is as high as the current “temperature” of the process.  The “temperature”
           //diminishes with iterations.
           if ( f_eNewLocal > f_eCurrent )
              return ( f_T - f_Tfinal ) / ( f_Tinitial - f_Tfinal );
           //If the new state is less energetic, the probability of acceptance is 100% (natural search for minima).
           else
              return 1.0;
        };
        //Get the number of parameters.
        int i_nPar = vw.size();
        //Make a copy of the initial state (parameter set.
        spectral::array L_wCurrent( vw );
        //The parameters variations (maxes - mins)
        spectral::array L_wDelta = L_wMax - L_wMin;
        //Get the input data.
        spectral::array *gridData = grid->createSpectralArray( variable->getIndexInParentGrid() );
        //...................Main annealing loop...................
        QProgressDialog progressDialog;
        progressDialog.setRange(0,0);
        progressDialog.show();
        progressDialog.setLabelText("Simulated Annealing in progress...");
		double f_eNew = std::numeric_limits<double>::max();
        double f_lowestEnergyFound = std::numeric_limits<double>::max();
        spectral::array L_wOfLowestEnergyFound;
        int k = 0;
        for( ; k < i_kmax; ++k ){
            emit info( "Commencing SA step #" + QString::number( k ) );
            //Get current temperature.
            double f_T = temperature( k );
            //Quit if temperature is lower than the minimum annealing temperature.
            if( f_T < f_Tfinal )
                break;
            //Randomly searches for a neighboring state with respect to current state.
            spectral::array L_wNew(L_wCurrent);
            for( int i = 0; i < i_nPar; ++i ){ //for each parameter
               //Ensures that the values randomly obtained are inside the domain.
               double f_tmp = 0.0;
               while( true ){
                   //set the value interval for the draw
                  double LO = L_wCurrent[i] - (f_factorSearch * L_wDelta[i]);
                  double HI = L_wCurrent[i] + (f_factorSearch * L_wDelta[i]);
                  // draw a value for the parameter
                  f_tmp = LO + std::rand() / (RAND_MAX/(HI-LO)) ;
                  //if the drawn parameter is within the domain.
                  if ( f_tmp >= L_wMin[i] && f_tmp <= L_wMax[i] ){
                      //if the resulting set of fundamental factor weights is valid.
                      spectral::array L_wTest(L_wNew);
                      L_wTest[i] = f_tmp;
                      if( allowNegativeWeights ||
                              isSetOfFreeParametersValid( L_wTest, A, Adagger, B, I ) )
                          //approve the new parameter draw
                         break;
                  }
               }
               //Updates the parameter value.
               L_wNew[i] = f_tmp;
			}
            //Computes the “energy” of the current state (set of parameters).
            //The “energy” in this case is how different the image as given the parameters is with respect
            //the data grid, considered the reference image.
            double f_eCurrent = F2( *gridData, L_wCurrent, A, Adagger, B, I, m, fundamentalFactors, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold, nSkipOutermost, nIsosurfs, nMinIsoVertexes, off );
            //Computes the “energy” of the neighboring state.
            f_eNew = F2( *gridData, L_wNew, A, Adagger, B, I, m, fundamentalFactors, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold, nSkipOutermost, nIsosurfs, nMinIsoVertexes, off );
            //Changes states stochastically.  There is a probability of acceptance of a more energetic state so
            //the optimization search starts near the global minimum and is not trapped in local minima (hopefully).
            double f_probMov = probAcceptance( f_eCurrent, f_eNew, f_T );
            if( f_probMov >= ( (double)std::rand() / RAND_MAX ) ) {//draws a value between 0.0 and 1.0 and tests whether is less than or equal to the acceptance probability of "going uphill"
                L_wCurrent = L_wNew; //replaces the current state with the neighboring random state
                emit info("  moved to energy level " + QString::number( f_eNew ));
                //if the energy is the record low, store it, just in case the SA loop ends without converging.
                if( f_eNew < f_lowestEnergyFound ){
                    f_lowestEnergyFound = f_eNew;
                    L_wOfLowestEnergyFound = spectral::array( L_wCurrent );
                }
            }
			if( ! (k % 10) ) //to avoid excess calls to processEvents.
				QCoreApplication::processEvents();
		}
        // The input data is no longer necessary.
        delete gridData;
        // Delivers the set of parameters near the global minimum (hopefully) for the Gradient Descent algorithm.
        // The SA loop may end in a higher energy state, so we return the lowest found in that case
        if( k == i_kmax && f_lowestEnergyFound < f_eNew )
            emit info( "SA completed by number of steps." );
        else
            emit info( "SA completed by reaching the lowest temperature." );
        vw = L_wOfLowestEnergyFound;
        emit info( "Using the state of lowest energy found (" + QString::number( f_lowestEnergyFound ) + ")" );
	}

	//---------------------------------------------------------------------------------------------------------
	//--------------------------------------OPTIMIZATION LOOP (GRADIENT DESCENT)-------------------------------
	//---------------------------------------------------------------------------------------------------------
	unsigned int nThreads = ui->spinNumberOfThreads->value();
	QProgressDialog progressDialog;
	progressDialog.setRange(0,0);
	progressDialog.show();
	progressDialog.setLabelText("Gradient Descent in progress...");
	int iOptStep = 0;
	spectral::array va;
	for( ; iOptStep < maxNumberOfOptimizationSteps; ++iOptStep ){

		emit info( "Commencing GD step #" + QString::number( iOptStep ) );


		//Compute the vector of weights [a] = Adagger.B + (I-Adagger.A)[w] (see program manual for theory)
		{
			Eigen::MatrixXd eigenAdagger = spectral::to_2d( Adagger );
			Eigen::MatrixXd eigenB = spectral::to_2d( B );
			Eigen::MatrixXd eigenI = spectral::to_2d( I );
			Eigen::MatrixXd eigenA = spectral::to_2d( A );
			Eigen::MatrixXd eigenvw = spectral::to_2d( vw );
			Eigen::MatrixXd eigenva = eigenAdagger * eigenB + ( eigenI - eigenAdagger * eigenA ) * eigenvw;
			va = spectral::to_array( eigenva );
		}

		{
			spectral::array debug_va( va );
			debug_va.set_size( (spectral::index)n, (spectral::index)m );
		}

		//Compute the gradient vector of objective function F with the current [w] parameters.
		spectral::array gradient( vw.size() );
		{
			spectral::array *gridData = grid->createSpectralArray( variable->getIndexInParentGrid() );

			//distribute the parameter indexes among the n-threads
			std::vector<int> parameterIndexBins[nThreads];
			int parameterIndex = 0;
			for( unsigned int iThread = 0; parameterIndex < vw.size(); ++parameterIndex, ++iThread)
				parameterIndexBins[ iThread % nThreads ].push_back( parameterIndex );

			//create and run the partial derivative calculation threads
			std::thread threads[nThreads];
			for( unsigned int iThread = 0; iThread < nThreads; ++iThread){
				threads[iThread] = std::thread( taskOnePartialDerivative2,
												vw,
												parameterIndexBins[iThread],
												epsilon,
												gridData,
												A,
												Adagger,
												B,
												I,
												m,
												fundamentalFactors,
												addSparsityPenalty,
												addOrthogonalityPenalty,
                                                sparsityThreshold,
												nSkipOutermost,
												nIsosurfs,
                                                nMinIsoVertexes,
                                                off,
												&gradient);
			}

			//wait for the threads to finish.
			for( unsigned int iThread = 0; iThread < nThreads; ++iThread)
				threads[iThread].join();

			delete gridData;
		}

		//Update the system's parameters according to gradient descent.
		double currentF = 999.0;
		double nextF = 1.0;
		{
			spectral::array *gridData = grid->createSpectralArray( variable->getIndexInParentGrid() );
			double alpha = initialAlpha;
			//halves alpha until we get a descent (current gradient vector may result in overshooting)
			int iAlphaReductionStep = 0;
			for( ; iAlphaReductionStep < maxNumberOfAlphaReductionSteps; ++iAlphaReductionStep ){
				spectral::array new_vw = vw - gradient * alpha;
				//Impose domain constraints to the parameters.
				for( int i = 0; i < new_vw.size(); ++i){
					if( new_vw.d_[i] < 0.0 )
						new_vw.d_[i] = 0.0;
					if( new_vw.d_[i] > 1.0 )
						new_vw.d_[i] = 1.0;
				}
                currentF = F2( *gridData, vw, A, Adagger, B, I, m, fundamentalFactors, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold, nSkipOutermost, nIsosurfs, nMinIsoVertexes, off );
                nextF = F2( *gridData, new_vw, A, Adagger, B, I, m, fundamentalFactors, addSparsityPenalty, addOrthogonalityPenalty, sparsityThreshold, nSkipOutermost, nIsosurfs, nMinIsoVertexes, off );
				if( nextF < currentF ){
					vw = new_vw;
					break;
				}
				alpha /= 2.0;
			}
			if( iAlphaReductionStep == maxNumberOfAlphaReductionSteps )
				emit warning( "WARNING: reached maximum alpha reduction steps." );
			delete gridData;
		}

		//Check the convergence criterion.
		double ratio = currentF / nextF;
		if( ratio  < (1.0 + convergenceCriterion) )
			break;

		emit info( "F2(k)/F2(k+1) ratio: " + QString::number( ratio ) );

		if( ! ( iOptStep % 10) ) //to avoid excess calls to processEvents.
			QCoreApplication::processEvents();
	}
	progressDialog.hide();

	//-------------------------------------------------------------------------------------------------
	//------------------------------------PRESENT THE RESULTS------------------------------------------
	//-------------------------------------------------------------------------------------------------

	if( iOptStep == maxNumberOfOptimizationSteps )
		QMessageBox::warning( this, "Warning", "Completed by reaching maximum number of optimization steps. Check results.");
	else
		QMessageBox::information( this, "Info", "Completed by satisfaction of convergence criterion.");

	std::vector< spectral::array > grids;
	std::vector< std::string > titles;
	std::vector< bool > shiftByHalves;

	spectral::array derivedGrid( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
	{
		//Make the m geological factors (expected maps with different variographic structures)
		std::vector< spectral::array > geologicalFactors;
		{
			QProgressDialog progressDialog;
			progressDialog.setRange(0,0);
			progressDialog.show();
			progressDialog.setLabelText("Making the geological factors...");
			for( int iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor){
				spectral::array geologicalFactor( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
				for( int iSVDFactor = 0; iSVDFactor < n; ++iSVDFactor){
					double weight = va.d_[ iGeoFactor * m + iSVDFactor ];
					geologicalFactor += fundamentalFactors[iSVDFactor] * weight;
				}
				geologicalFactors.push_back( std::move( geologicalFactor ) );
				QCoreApplication::processEvents();
			}
		}

		//Change the weights m*n vector to a n by m matrix for displaying (fundamental factors as columns and geological factors as lines)
		va.set_size( (spectral::index)n, (spectral::index)m );
		displayGrids( {va}, {"parameters"}, {false} );

		//Collect the geological factors (expected maps with different variographic structures)
		//as separate grids for display.
		std::vector< spectral::array >::iterator it = geologicalFactors.begin();
		for( int iGeoFactor = 0; it != geologicalFactors.end(); ++it, ++iGeoFactor ){
			spectral::array rfftResult( *it );
			//Collect the grids.
			QString title = QString("Factor #") + QString::number(iGeoFactor+1);
			titles.push_back( title.toStdString() );
			grids.push_back( std::move( rfftResult ) );
			shiftByHalves.push_back( false );
		}
	}

	//Display the derived grid and its difference with respect to the original grid.
	//Also display the geological factors and their resulting partial grids.
    displayGrids( grids, titles, shiftByHalves );
}

bool VariographicDecompositionDialog::isSetOfFreeParametersValid( const spectral::array &vectorOfParameters,
                                                                  const spectral::array &A,
                                                                  const spectral::array &Adagger,
                                                                  const spectral::array &B,
                                                                  const spectral::array &I) const
{
    //Compute the vector of weights [a] = Adagger.B + (I-Adagger.A)[w]
    spectral::array va;
    {
        Eigen::MatrixXd eigenAdagger = spectral::to_2d( Adagger );
        Eigen::MatrixXd eigenB = spectral::to_2d( B );
        Eigen::MatrixXd eigenI = spectral::to_2d( I );
        Eigen::MatrixXd eigenA = spectral::to_2d( A );
        Eigen::MatrixXd eigenvw = spectral::to_2d( vectorOfParameters );
        Eigen::MatrixXd eigenva = eigenAdagger * eigenB + ( eigenI - eigenAdagger * eigenA ) * eigenvw;
        va = spectral::to_array( eigenva );
    }

    int nPar = va.size();
    for( int i = 0; i < nPar; ++i ) //for each parameter
      if ( va[i] < 0.0 || va[i] > 1.0 )
         return false;

    return true;
}

void VariographicDecompositionDialog::doVariographicDecomposition3()
{
    doVariographicDecomposition2( FundamentalFactorType::FFT_SPECTRUM_PARTITION );
}

void VariographicDecompositionDialog::doVariographicDecomposition4()
{
    doVariographicDecomposition2( FundamentalFactorType::GABOR_ANALYSIS_FACTOR );
}

void VariographicDecompositionDialog::doVariographicParametersAnalysis(FundamentalFactorType fundamentalFactorType)
{
    ///Visualizing the results on the fly is optional/////////////
    IJQuick3DViewer q3Dv;
    q3Dv.show();
    ///////////////////////////////////////////////////////////////

    // Get the data objects.
    IJAbstractCartesianGrid* grid = m_gridSelector->getSelectedGrid();
    IJAbstractVariable* variable = m_variableSelector->getSelectedVariable();

    // Get the grid's dimensions.
    unsigned int nI = grid->getNI();
    unsigned int nJ = grid->getNJ();
    unsigned int nK = grid->getNK();

    // Fetch data from the data source.
    grid->dataWillBeRequested();

    //get user settings
    int nTracks = ui->spinNumberOfSpectrumTracks->value();
    GaborAnalysisParameters gaborParameters;
    gaborParameters.initialFrequency = 0.003;
    gaborParameters.finalFrequency = 0.09;
    gaborParameters.frequencyStep = 0.01; //orig = 0.0001
    gaborParameters.azimuthStep = 15.0;
    gaborParameters.kernelSize = 50;
    gaborParameters.kernelMeanMajorAxis = 127.5; //max. resolution size is 255, this 127.5 is in the middle
    gaborParameters.kernelMeanMinorAxis = 127.5;
    gaborParameters.kernelSigmaMajorAxis = 50.0;
    gaborParameters.kernelSigmaMinorAxis = 50.0;

    // Get the fundamental factors of the input variable.
    std::vector< spectral::array > fundamentalFactors;
    int n = 0; //n is the number of fundamental factors
    {
        //Atom learning method: frequency spectrum partitioning of input variable.
        if ( fundamentalFactorType == FundamentalFactorType::FFT_SPECTRUM_PARTITION ) {
            QProgressDialog progressDialog;
            progressDialog.setRange(0,0);
            progressDialog.setLabelText("Computing Fourier partitioning of input data...");
            progressDialog.show();
            QCoreApplication::processEvents();
            spectral::array* gridInputData = grid->createSpectralArray( variable->getIndexInParentGrid() );
            doFourierPartitioningOnData( gridInputData, fundamentalFactors, nTracks );
            delete gridInputData;
            n = fundamentalFactors.size();
        //Atom learning method: Gabor analysis of input variable.
        } else {
            QProgressDialog progressDialog;
            progressDialog.setRange(0,0);
            progressDialog.setLabelText("Computing Gabor analysis of input data...");
            progressDialog.show();
            QCoreApplication::processEvents();
            spectral::array* gridInputData = grid->createSpectralArray( variable->getIndexInParentGrid() );
            doGaborAnalysisOnData( gridInputData, fundamentalFactors, gaborParameters );
            delete gridInputData;
            n = fundamentalFactors.size();
        }
    }

    //make an array full of zeroes (useful for certain steps in the workflow)
    spectral::array zeros( nI, nJ, nK, 0.0 );

    //compute the FFT for each fundamental factor
    // TODO: this loop is parallelizable
    for( int iFF = 0; iFF < n; ++iFF ){
        spectral::array& fundamentalFactor = fundamentalFactors[iFF];

        //Compute FFT of the fundamentalFactor
        spectral::array ffFFTrealPart;
        spectral::array ffFFTimagPart;
        {
            spectral::complex_array ffFFT;
            spectral::foward( ffFFT, fundamentalFactor );
            ffFFTrealPart = spectral::real( ffFFT );
            ffFFTimagPart = spectral::imag( ffFFT );
        }

        //do a^2 + b^2
        // where a = real part of FFT; b = imaginary part of FFT.
        spectral::array ffMagSquared = spectral::hadamard( ffFFTrealPart, ffFFTrealPart ) +
                                       spectral::hadamard( ffFFTimagPart, ffFFTimagPart );

        //make a complex array from a^2 + b^2 as magnitude and zeros as phase (polar form)
        //this corresponds to the FFT of a variographic map
        spectral::complex_array ffFFTVarMapPolarForm = spectral::to_complex_array( ffMagSquared, zeros );
        ffMagSquared = spectral::array(); //keep memory usage at bay

        //convert the previous complex array to the rectangular form
        spectral::complex_array ffFFTVarMapRectangularForm = spectral::to_rectangular_form( ffFFTVarMapPolarForm );
        ffFFTVarMapPolarForm = spectral::complex_array(); //keep memory usage at bay

        //get the varmap of the fundamental factor by reverse FFT
        spectral::array ffVarMap( nI, nJ, nK, 0.0 );
        spectral::backward( ffVarMap, ffFFTVarMapRectangularForm );
        ffFFTVarMapRectangularForm = spectral::complex_array(); //keep memory usage at bay

        //put h=0 of the varmap at the center of the grid
        {
            spectral::array ffVarMapTMP = spectral::shiftByHalf( ffVarMap );
            ffVarMap = ffVarMapTMP;
        }

        ///Visualizing the results on the fly is optional/////////////
        {
            q3Dv.clearScene();
            q3Dv.display( ffVarMap, ffVarMap.min(), ffVarMap.max() );
            QApplication::processEvents();
            QMessageBox::information(this, "aaaa", "aaaa");
        }
        ////////////////////////////////////////////////////////////

        //NOT DONE: apply marching squares to get varmap isocontours

        //NOT DONE: discard open or not centered isocontours

        //NOT DONE: fit ellipses to isocontours

        //NOT DONE: get ellipse geometric parameters (axes and azimuth)

    }

    //NOT DONE: plot geometric parameters in feature space (e.g. cross plot)
}

void VariographicDecompositionDialog::doVariographicParametersAnalysisWithGabor()
{
    doVariographicParametersAnalysis( FundamentalFactorType::GABOR_ANALYSIS_FACTOR );
}

void VariographicDecompositionDialog::doVariographicParametersAnalysisWithSpectrumPart()
{
    doVariographicParametersAnalysis( FundamentalFactorType::FFT_SPECTRUM_PARTITION );
}

void VariographicDecompositionDialog::doVariographicDecomposition5_WITH_SA_AND_GD()
{
    ///Visualizing the results on the fly is optional/////////////
    IJQuick3DViewer q3Dv;
    q3Dv.show();
    ///////////////////////////////////////////////////////////////

    //get user configuration
    int m = ui->spinNumberOfGeologicalFactors->value();
    int maxNumberOfOptimizationSteps = ui->spinMaxSteps->value();
    // The user-given epsilon (useful for numerical calculus).
    double epsilon = std::pow(10, ui->spinLogEpsilon->value() );
    double initialAlpha = ui->spinInitialAlpha->value();
    double maxNumberOfAlphaReductionSteps = ui->spinMaxStepsAlphaReduction->value();
    double convergenceCriterion = std::pow(10, ui->spinConvergenceCriterion->value() );

    // Get the data objects.
    IJAbstractCartesianGrid* inputGrid = m_gridSelector->getSelectedGrid();
    IJAbstractVariable* variable = m_variableSelector->getSelectedVariable();

    // Get the grid's dimensions.
    unsigned int nI = inputGrid->getNI();
    unsigned int nJ = inputGrid->getNJ();
    unsigned int nK = inputGrid->getNK();

    // Fetch data from the data source.
    inputGrid->dataWillBeRequested();

    //========================= GET VARMAP OF THE INPUT DATA =====================================

    //make an array full of zeroes (useful for certain steps in the workflow)
    spectral::array zeros( nI, nJ, nK, 0.0 );

    // Get the input data as a spectral::array object
    spectral::arrayPtr inputData( inputGrid->createSpectralArray( variable->getIndexInParentGrid() ) );

    //Compute FFT of input
    spectral::array inputFFTrealPart;
    spectral::array inputFFTimagPart;
    {
        spectral::complex_array inputFFT;
        spectral::foward( inputFFT, *inputData );
        inputFFTrealPart = spectral::real( inputFFT );
        inputFFTimagPart = spectral::imag( inputFFT );
    }

    //do a^2 + b^2
    // where a = real part of FFT; b = imaginary part of FFT.
    spectral::array inputMagSquared = spectral::hadamard( inputFFTrealPart, inputFFTrealPart ) +
                                      spectral::hadamard( inputFFTimagPart, inputFFTimagPart );

    //make a complex array from a^2 + b^2 as magnitude and zeros as phase (polar form)
    //this corresponds to the FFT of a variographic map
    spectral::complex_array inputVarmapFFTPolarForm = spectral::to_complex_array( inputMagSquared, zeros );
    inputMagSquared = spectral::array(); //garbage collection

    //convert the previous complex array to the rectangular form
    spectral::complex_array inputVarmapFFTRectangularForm = spectral::to_rectangular_form( inputVarmapFFTPolarForm );
    inputVarmapFFTPolarForm = spectral::complex_array(); //garbage collection

    //get the varmap of the input data by reverse FFT
    spectral::array inputVarmap( nI, nJ, nK, 0.0 );
    spectral::backward( inputVarmap, inputVarmapFFTRectangularForm );
    inputVarmapFFTRectangularForm = spectral::complex_array(); //garbage collection

    //fftw requires that the values of r-FFT be divided by the number of cells
    inputVarmap = inputVarmap / (double)( nI * nJ * nK );

    //put h=0 of the varmap at the center of the grid
    {
        spectral::array ffVarMapTMP = spectral::shiftByHalf( inputVarmap );
        inputVarmap = ffVarMapTMP;
    }

    //================================== PREPARE OPTIMIZATION STEPS ==========================

    //define the domain
    double minAxis         = 0.0  ; double maxAxis         = inputGrid->getDiagonalLength() / 2.0;
    double minRatio        = 0.001; double maxRatio        = 1.0;
    double minAzimuth      = 0.0  ; double maxAzimuth      = ImageJockeyUtils::PI;
    double minContribution = 0.0  ; double maxContribution = inputVarmap.max();

    //create one variographic ellipse for each geological factor wanted by the user
    //the parameters are initialized near in the center of the domain
    std::vector< IJVariographicStructure2D > variographicEllipses;
    for( int i = 0; i < m; ++i )
        variographicEllipses.push_back( IJVariographicStructure2D ( ( maxAxis         + minAxis         ) / 2.0,
                                                                    ( maxRatio        + minRatio        ) / 2.0,
                                                                    ( minAzimuth      + maxAzimuth      ) / 2.0,
                                                                    ( maxContribution - minContribution ) / 4.0 ) );

    //Initialize the vector of linear system parameters [w]=[axis0,ratio0,az0,cc0,axis1,ratio1,...]
    spectral::array vw( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
    for( int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            vw[i] = variographicEllipses[iGeoFactor].getParameter( iPar );

    //-------------------------------------------------------------------------------------------------------------
    //-------------------------SIMULATED ANNEALING TO INITIALIZE THE PARAMETERS [w] NEAR A GLOBAL MINIMUM------------
    //---------------------------------------------------------------------------------------------------------------
    {
        //...................................Annealing Parameters.................................
        //Intial temperature.
        double f_Tinitial = ui->spinInitialTemperature->value();
        //Final temperature.
        double f_Tfinal = ui->spinFinalTemperature->value();
        //Max number of SA steps.
        int i_kmax = ui->spinMaxStepsSA->value();
        //Minimum value allowed for the parameters w (see min* variables further up). DOMAIN CONSTRAINT
        spectral::array L_wMin( vw.size(), 0.0d );
        for(int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
            for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
                switch( iPar ){
                case 0: L_wMin[i] = minAxis;         break;
                case 1: L_wMin[i] = minRatio;        break;
                case 2: L_wMin[i] = minAzimuth;      break;
                case 3: L_wMin[i] = minContribution; break;
                }
        //Maximum value allowed for the parameters w (see max* variables further up). DOMAIN CONSTRAINT
        spectral::array L_wMax( vw.size(), 1.0d );
        for(int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
            for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
                switch( iPar ){
                case 0: L_wMax[i] = maxAxis;         break;
                case 1: L_wMax[i] = maxRatio;        break;
                case 2: L_wMax[i] = maxAzimuth;      break;
                case 3: L_wMax[i] = maxContribution; break;
                }
        /*Factor used to control the size of the random state “hop”.  For example, if the maximum “hop” must be
         10% of domain size, set 0.1.  Small values (e.g. 0.001) result in slow, but more accurate convergence.
         Large values (e.g. 100.0) covers more space faster, but falls outside the domain are more frequent,
         resulting in more re-searches due to more invalid parameter value penalties. */
        double f_factorSearch = ui->spinMaxHopFactor->value();
        //Intialize the random number generator with the same seed
        std::srand ((unsigned)ui->spinSeed->value());
        //.................................End of Annealing Parameters.............................
        //Returns the current “temperature” of the system.  It yields a log curve that decays as the step number increase.
        // The initial temperature plays an important role: curve starting with 5.000 is steeper than another that starts with 1.000.
        //  This means the the lower the temperature, the more linear the temperature decreases.
        // i_stepNumber: the current step number of the annealing process ( 0 = first ).
        auto temperature = [=](int i_stepNumber) { return f_Tinitial * std::exp( -i_stepNumber / (double)1000 * (1.5 * std::log10( f_Tinitial ) ) ); };
        /*Returns the probability of acceptance of the energy state for the next iteration.
          This allows acceptance of higher values to break free from local minima.
          f_eCurrent: current energy of the system.
          f_eNew: energy level of the next step.
          f_T: current “temperature” of the system.*/
        auto probAcceptance = [=]( double f_eCurrent, double f_eNewLocal, double f_T ) {
           //If the new state is more energetic, calculates a probability of acceptance
           //which is as high as the current “temperature” of the process.  The “temperature”
           //diminishes with iterations.
           if ( f_eNewLocal > f_eCurrent )
              return ( f_T - f_Tfinal ) / ( f_Tinitial - f_Tfinal );
           //If the new state is less energetic, the probability of acceptance is 100% (natural search for minima).
           else
              return 1.0 - ( f_T - f_Tfinal ) / ( f_Tinitial - f_Tfinal );
        };
        //Get the number of parameters.
        int i_nPar = vw.size();
        //Make a copy of the initial state (parameter set.
        spectral::array L_wCurrent( vw );
        //The parameters variations (maxes - mins)
        spectral::array L_wDelta = L_wMax - L_wMin;
        //...................Main annealing loop...................
        QProgressDialog progressDialog;
        progressDialog.setRange(0,0);
        progressDialog.show();
        progressDialog.setLabelText("Simulated Annealing in progress...");
        QCoreApplication::processEvents();
        double f_eNew = std::numeric_limits<double>::max();
        double f_lowestEnergyFound = std::numeric_limits<double>::max();
        spectral::array L_wOfLowestEnergyFound;
        int k = 0;
        for( ; k < i_kmax; ++k ){
            emit info( "Commencing SA step #" + QString::number( k ) );
            //Get current temperature.
            double f_T = temperature( k );
            //Quit if temperature is lower than the minimum annealing temperature.
            if( f_T < f_Tfinal )
                /*break*/;
            //Randomly searches for a neighboring state with respect to current state.
            spectral::array L_wNew(L_wCurrent);
            for( int i = 0; i < i_nPar; ++i ){ //for each parameter
               //Ensures that the values randomly obtained are inside the domain.
               double f_tmp = 0.0;
               while( true ){
                  double LO = L_wCurrent[i] - (f_factorSearch * L_wDelta[i]);
                  double HI = L_wCurrent[i] + (f_factorSearch * L_wDelta[i]);
                  f_tmp = LO + std::rand() / (RAND_MAX/(HI-LO)) ;
                  if ( f_tmp >= L_wMin[i] && f_tmp <= L_wMax[i] )
                     break;
               }
               //Updates the parameter value.
               L_wNew[i] = f_tmp;
            }
            //Computes the “energy” of the current state (set of parameters).
            //The “energy” in this case is how different the image as given the parameters is with respect
            //the data grid, considered the reference image.
            //double f_eCurrent = F3( *inputGrid, inputVarmap, L_wCurrent, m );
            double f_eCurrent = F4( *inputGrid, *inputData, L_wCurrent, m );

            //Computes the “energy” of the neighboring state.
            //f_eNew = F3( *inputGrid, inputVarmap, L_wNew, m );
            f_eNew = F4( *inputGrid, *inputData, L_wNew, m );
            //Changes states stochastically.  There is a probability of acceptance of a more energetic state so
            //the optimization search starts near the global minimum and is not trapped in local minima (hopefully).
            double f_probMov = probAcceptance( f_eCurrent, f_eNew, f_T );
            if( f_probMov >= ( (double)std::rand() / RAND_MAX ) ) {//draws a value between 0.0 and 1.0
                L_wCurrent = L_wNew; //replaces the current state with the neighboring random state
                emit info("  moved to energy level " + QString::number( f_eNew ));
                //if the energy is the record low, store it, just in case the SA loop ends without converging.
                if( f_eNew < f_lowestEnergyFound ){
                    f_lowestEnergyFound = f_eNew;
                    L_wOfLowestEnergyFound = spectral::array( L_wCurrent );
                }
            }
        }
        // Delivers the set of parameters near the global minimum (hopefully) for the Gradient Descent algorithm.
        // The SA loop may end in a higher energy state, so we return the lowest found in that case
        if( k == i_kmax && f_lowestEnergyFound < f_eNew )
            emit info( "SA completed by number of steps." );
        else
            emit info( "SA completed by reaching the lowest temperature." );
        vw = L_wOfLowestEnergyFound;
        emit info( "Using the state of lowest energy found (" + QString::number( f_lowestEnergyFound ) + ")" );
    }

    //---------------------------------------------------------------------------------------------------------
    //--------------------------------------OPTIMIZATION LOOP (GRADIENT DESCENT)-------------------------------
    //---------------------------------------------------------------------------------------------------------
    unsigned int nThreads = ui->spinNumberOfThreads->value();
    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.show();
    progressDialog.setLabelText("Gradient Descent in progress...");
    int iOptStep = 0;
    for( ; iOptStep < maxNumberOfOptimizationSteps; ++iOptStep ){

        emit info( "Commencing GD step #" + QString::number( iOptStep ) );

        //Compute the gradient vector of objective function F with the current [w] parameters.
        spectral::array gradient( vw.size() );
        {

            //distribute the parameter indexes among the n-threads
            std::vector<int> parameterIndexBins[nThreads];
            int parameterIndex = 0;
            for( unsigned int iThread = 0; parameterIndex < vw.size(); ++parameterIndex, ++iThread)
                parameterIndexBins[ iThread % nThreads ].push_back( parameterIndex );

            //create and run the partial derivative calculation threads
            std::thread threads[nThreads];
            for( unsigned int iThread = 0; iThread < nThreads; ++iThread){
                threads[iThread] = std::thread( taskOnePartialDerivative3,
                                                vw,
                                                parameterIndexBins[iThread],
                                                epsilon,
                                                inputGrid,
                                                inputVarmap,
                                                m,
                                                &gradient);
            }

            //wait for the threads to finish.
            for( unsigned int iThread = 0; iThread < nThreads; ++iThread)
                threads[iThread].join();

        }

        //Update the system's parameters according to gradient descent.
        double currentF = std::numeric_limits<double>::max();
        double nextF = 1.0;
        {
            spectral::array *gridData = inputGrid->createSpectralArray( variable->getIndexInParentGrid() );
            double alpha = initialAlpha;
            //halves alpha until we get a descent (current gradient vector may result in overshooting)
            int iAlphaReductionStep = 0;
            for( ; iAlphaReductionStep < maxNumberOfAlphaReductionSteps; ++iAlphaReductionStep ){
                spectral::array new_vw = vw - gradient * alpha;
                //Impose domain constraints to the parameters.
                for( int i = 0; i < new_vw.size(); ++i){
                    if( new_vw.d_[i] < 0.0 )
                        new_vw.d_[i] = 0.0;
                    if( new_vw.d_[i] > 1.0 )
                        new_vw.d_[i] = 1.0;
                }
//                currentF = F3( *inputGrid, inputVarmap, vw, m );
//                nextF = F3( *inputGrid, inputVarmap, new_vw, m );
                currentF = F4( *inputGrid, *inputData, vw, m );
                nextF = F4( *inputGrid, *inputData, new_vw, m );
                if( nextF < currentF ){
                    vw = new_vw;
                    break;
                }
                alpha /= 2.0;
            }
            if( iAlphaReductionStep == maxNumberOfAlphaReductionSteps )
                emit warning( "WARNING: reached maximum alpha reduction steps." );
            delete gridData;
        }

        //Check the convergence criterion.
        double ratio = currentF / nextF;
        if( ratio  < (1.0 + convergenceCriterion) )
            break;

        emit info( "F2(k)/F2(k+1) ratio: " + QString::number( ratio ) );

        if( ! ( iOptStep % 10) ) //to avoid excess calls to processEvents.
            QCoreApplication::processEvents();
    }
    progressDialog.hide();

    //Read the optimized variogram model parameters back to the variographic structures
    for( int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            variographicEllipses[iGeoFactor].setParameter( iPar, vw[i] );

    ///Visualizing the results on the fly is optional/////////////
    {
        spectral::array finalVariogramModelSurface( nI, nJ, nK, 0.0 );
        for( int iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
            variographicEllipses[iGeoFactor].addContributionToModelGrid( *inputGrid,
                                                                         finalVariogramModelSurface,
                                                                         IJVariogramPermissiveModel::SPHERIC,
                                                                         true );
        q3Dv.clearScene();
        q3Dv.display( finalVariogramModelSurface, finalVariogramModelSurface.min(), finalVariogramModelSurface.max() );
        QApplication::processEvents();
        QMessageBox::information(this, "aaaa", "aaaa");
    }
    ////////////////////////////////////////////////////////////
}

void VariographicDecompositionDialog::doVariographicDecomposition5()
{
    QMessageBox msgBox;
    msgBox.setText("Perform optimization with:");
    QAbstractButton* pButtonUseSAandGS = msgBox.addButton("SA + GD", QMessageBox::YesRole);
    QAbstractButton* pButtonUsePSO = msgBox.addButton("PSO", QMessageBox::ApplyRole);
    QAbstractButton* pButtonUseGenetic = msgBox.addButton("Genetic.", QMessageBox::AcceptRole);
    QAbstractButton* pButtonUseBrute = msgBox.addButton("Brute Force.", QMessageBox::HelpRole);
    msgBox.addButton("LSRS", QMessageBox::NoRole);
    msgBox.exec();
    if ( msgBox.clickedButton() == pButtonUseSAandGS )
        doVariographicDecomposition5_WITH_SA_AND_GD();
    else if ( msgBox.clickedButton() == pButtonUsePSO )
        doVariographicDecomposition5_WITH_PSO();
    else if ( msgBox.clickedButton() == pButtonUseGenetic )
        doVariographicDecomposition5_WITH_Genetic();
    else if ( msgBox.clickedButton() == pButtonUseBrute )
        doVariographicDecomposition5_WITH_BruteForce();
    else
        doVariographicDecomposition5_WITH_LSRS();
}

void VariographicDecompositionDialog::doVariographicDecomposition5_WITH_LSRS()
{

    ///Visualizing the results on the fly is optional/////////////
//    IJQuick3DViewer q3Dv;
//    q3Dv.show();
    ///////////////////////////////////////////////////////////////

    //get user configuration
    int m = ui->spinNumberOfGeologicalFactors->value();
    int maxNumberOfOptimizationSteps = ui->spinMaxSteps->value();
    // The user-given epsilon (useful for numerical calculus).
    double epsilon = std::pow(10, ui->spinLogEpsilon->value() );
    int nStartingPoints = 20; //number of random starting points in the domain
    int nRestarts = 40; //number of restarts

    // Get the data objects.
    IJAbstractCartesianGrid* inputGrid = m_gridSelector->getSelectedGrid();
    IJAbstractVariable* variable = m_variableSelector->getSelectedVariable();

    // Get the grid's dimensions.
    unsigned int nI = inputGrid->getNI();
    unsigned int nJ = inputGrid->getNJ();
    unsigned int nK = inputGrid->getNK();

    // Fetch data from the data source.
    inputGrid->dataWillBeRequested();

    //========================= GET VARMAP OF THE INPUT DATA =====================================

    //make an array full of zeroes (useful for certain steps in the workflow)
    spectral::array zeros( nI, nJ, nK, 0.0 );

    // Get the input data as a spectral::array object
    spectral::arrayPtr inputData( inputGrid->createSpectralArray( variable->getIndexInParentGrid() ) );

    //Compute FFT of input
    spectral::array inputFFTrealPart;
    spectral::array inputFFTimagPart;
    spectral::array inputFFTimagPhase;
    {
        spectral::complex_array inputFFT;
        spectral::foward( inputFFT, *inputData );
        inputFFTrealPart = spectral::real( inputFFT );
        inputFFTimagPart = spectral::imag( inputFFT );
        spectral::complex_array inputFFTpolar = spectral::to_polar_form( inputFFT );
        inputFFTimagPhase = spectral::imag( inputFFTpolar );
    }

    //do a^2 + b^2
    // where a = real part of FFT; b = imaginary part of FFT.
    spectral::array inputMagSquared = spectral::hadamard( inputFFTrealPart, inputFFTrealPart ) +
                                      spectral::hadamard( inputFFTimagPart, inputFFTimagPart );

    //make a complex array from a^2 + b^2 as magnitude and zeros as phase (polar form)
    //this corresponds to the FFT of a variographic map
    spectral::complex_array inputVarmapFFTPolarForm = spectral::to_complex_array( inputMagSquared, zeros );
    inputMagSquared = spectral::array(); //garbage collection

    //convert the previous complex array to the rectangular form
    spectral::complex_array inputVarmapFFTRectangularForm = spectral::to_rectangular_form( inputVarmapFFTPolarForm );
    inputVarmapFFTPolarForm = spectral::complex_array(); //garbage collection

    //get the varmap of the input data by reverse FFT
    spectral::array inputVarmap( nI, nJ, nK, 0.0 );
    spectral::backward( inputVarmap, inputVarmapFFTRectangularForm );
    inputVarmapFFTRectangularForm = spectral::complex_array(); //garbage collection

    //fftw requires that the values of r-FFT be divided by the number of cells
    inputVarmap = inputVarmap / (double)( nI * nJ * nK );

    //put h=0 of the varmap at the center of the grid
    {
        spectral::array ffVarMapTMP = spectral::shiftByHalf( inputVarmap );
        inputVarmap = ffVarMapTMP;
    }

    //================================== PREPARE OPTIMIZATION STEPS ==========================

    //define the domain
    double minCellSize = std::min( inputGrid->getCellSizeI(), inputGrid->getCellSizeJ() );
    double minAxis         = minCellSize;               double maxAxis         = inputGrid->getDiagonalLength() / 2.0;
    double minRatio        = 0.001;                     double maxRatio        = 1.0;
    double minAzimuth      = 0.0  ;                     double maxAzimuth      = ImageJockeyUtils::PI;
    double minContribution = inputVarmap.max() / 100.0; double maxContribution = inputVarmap.max();
    double deltaAxis = maxAxis - minAxis;
    double deltaRatio = maxRatio - minRatio;
    double deltaAzimuth = maxAzimuth - minAzimuth;
    double deltaContribution = maxContribution - minContribution;

    //create one variographic ellipse for each geological factor wanted by the user
    //the parameters are initialized near in the center of the domain
    //the starting values are not particuarly important.
    std::vector< IJVariographicStructure2D > variographicEllipses;
    for( int i = 0; i < m; ++i )
        variographicEllipses.push_back( IJVariographicStructure2D ( ( maxAxis         + minAxis         ) / 2.0,
                                                                    ( maxRatio        + minRatio        ) / 2.0,
                                                                    ( minAzimuth      + maxAzimuth      ) / 2.0,
                                                                    maxContribution / m ) ); //split evenly the total contribution among the geologic factors

    //Initialize the vector of linear system parameters [w]=[axis0,ratio0,az0,cc0,axis1,ratio1,...]
    // from the variographic parameters for each geological factor
    //the starting values are not particuarly important.
    spectral::array vw( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
    for( int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            vw[i] = variographicEllipses[iGeoFactor].getParameter( iPar );

    //Create a vector with the minimum values allowed for the parameters w
    //(see min* variables further up). DOMAIN CONSTRAINT
    spectral::array L_wMin( vw.size(), 0.0d );
    for(int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            switch( iPar ){
            case 0: L_wMin[i] = minAxis;         break;
            case 1: L_wMin[i] = minRatio;        break;
            case 2: L_wMin[i] = minAzimuth;      break;
            case 3: L_wMin[i] = minContribution; break;
            }

    //Create a vector with the maximum values allowed for the parameters w
    //(see max* variables further up). DOMAIN CONSTRAINT
    spectral::array L_wMax( vw.size(), 1.0d );
    for(int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            switch( iPar ){
            case 0: L_wMax[i] = maxAxis;         break;
            case 1: L_wMax[i] = maxRatio;        break;
            case 2: L_wMax[i] = maxAzimuth;      break;
            case 3: L_wMax[i] = maxContribution; break;
            }

    //-------------------------------------------------------------------------------------------------------------
    //------------------------- THE MODIFIED LINE SEARCH ALGORITH AS PROPOSED BY Grosan and Abraham (2009)---------
    //---------------------------A Novel Global Optimization Technique for High Dimensional Functions--------------
    //-------------------------------------------------------------------------------------------------------------

    //Intialize the random number generator with the same seed
    std::srand ((unsigned)ui->spinSeed->value());

    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.show();
    progressDialog.setLabelText("Line Search with Restart in progress...");

    //the line search restarting loop
    spectral::array vw_bestSolution( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
    for( int t = 0; t < nRestarts; ++t){

        //generate sarting points randomly within the domain
        std::vector< spectral::array > startingPoints;
        for( int iSP = 0; iSP < nStartingPoints; ++iSP ){
            spectral::array vw_StartingPoint( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
            for( int i = 0; i < vw_StartingPoint.size(); ++i ){
                double LO = L_wMin[i];
                double HI = L_wMax[i];
                vw_StartingPoint[i] = LO + std::rand() / (RAND_MAX/(HI-LO));
            }
            startingPoints.push_back( vw_StartingPoint );
        }

        //define the step as a function of iteration number (the alpha-k in Grosan and Abraham (2009))
        //first iteration must be 1.
        auto alpha_k = [=](int k) { return 2.0 + 3.0 / std::pow(2, k*k + 1); };

        //----------------loop of line search algorithm----------------
        double fOfBestSolution = std::numeric_limits<double>::max();
        //for each step
        for( int k = 1; k <= maxNumberOfOptimizationSteps; ++k ){
            //for each starting point (in the parameter space).
            for( int i = 0; i < nStartingPoints; ++i ){
                //make a candidate point with a vector from the current point.
                spectral::array vw_candidate( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
                for( int j = 0; j < vw.size(); ++j ){
                    double p_k = -1.0 + std::rand() / ( RAND_MAX / 2.0);//author suggests -1 or drawn from [0.0 1.0] for best results

                    double delta = 0.0;
                    switch( j % IJVariographicStructure2D::getNumberOfParameters() ){
                    case 0: delta = deltaAxis;         break;
                    case 1: delta = deltaRatio;        break;
                    case 2: delta = deltaAzimuth;      break;
                    case 3: delta = deltaContribution; break;
                    }

                    vw_candidate[j] = startingPoints[i][j] + p_k * delta * alpha_k( k );
                    if( vw_candidate[j] > L_wMax[j] )
                        vw_candidate[j] = L_wMax[j];
                    if( vw_candidate[j] < L_wMin[j] )
                        vw_candidate[j] = L_wMin[j];

                }
                //evaluate the objective function for the current point and for the candidate point
                double fCurrent = F3( *inputGrid, inputVarmap, startingPoints[i], m );
                double fCandidate = F3( *inputGrid, inputVarmap, vw_candidate, m );
                //if the candidate point improves the objective function...
                if( fCandidate < fCurrent ){
                    //...make it the current point.
                    startingPoints[i] = vw_candidate;
                    //keep track of the best solution
                    if( fCandidate < fOfBestSolution ){
                        fOfBestSolution = fCandidate;
                        vw_bestSolution = vw_candidate;
                    }
                }
            } //for each starting point
            QApplication::processEvents();
        } // search for best solution
        //---------------------------------------------------------------------------

        //for each parameter of the best solution
        for( int iParameter = 0; iParameter < vw.size(); ++iParameter ){
            //Make a set of parameters slightly shifted to the right (more positive) along one parameter.
            spectral::array vwFromRight = vw_bestSolution;
            vwFromRight(iParameter) = vw_bestSolution(iParameter) + epsilon;
            //Make a set of parameters slightly shifted to the left (more negative) along one parameter.
            spectral::array vwFromLeft = vw_bestSolution;
            vwFromLeft(iParameter) = vw_bestSolution(iParameter) - epsilon;
            //compute the partial derivative along one parameter
            double partialDerivative =  (F3( *inputGrid, inputVarmap, vwFromRight, m )
                                         -
                                         F3( *inputGrid, inputVarmap, vwFromLeft, m ))
                                         /
                                         ( 2 * epsilon );
            //update the domain limits depending on the partial derivative result
            //this usually reduces the size of the domain so the next set of starting
            //points have a higher probability to be drawn near a global optimum.
            if( partialDerivative > 0 )
                L_wMax[ iParameter ] = vw_bestSolution[ iParameter ];
            else if( partialDerivative < 0 )
                L_wMin[ iParameter ] = vw_bestSolution[ iParameter ];
        } // reduce the domain to a smaller hyper volume around the suspected optimum

    } //restart loop
    progressDialog.hide();

    //Read the optimized variogram model parameters back to the variographic structures
    for( int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            variographicEllipses[iGeoFactor].setParameter( iPar, vw_bestSolution[i] );


    //Apply the principle of the Fourier Integral Method
    //use a variographic map as the magnitudes and the FFT phases of
    //the original data to a reverse FFT in polar form to achieve a
    //Factorial Kriging-like separation
    std::vector< spectral::array > geoFactors;
    std::vector< std::string > titles;
    std::vector< bool > shiftFlags;
    for( int iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor ) {
        //compute the varmap of the theoretical varmap for one geologic factor
        spectral::array geoFactorVarMap( nI, nJ, nK, 0.0 );
        variographicEllipses[iGeoFactor].addContributionToModelGrid( *inputGrid,
                                                                     geoFactorVarMap,
                                                                     IJVariogramPermissiveModel::SPHERIC,
                                                                     true );

        //collect the theoretical varmap for display
        geoFactors.push_back( geoFactorVarMap );
        titles.push_back( QString( "Varmap " + QString::number( iGeoFactor ) ).toStdString() );
        shiftFlags.push_back( false );

        //compute FFT of the theoretical varmap (into polar form)
        spectral::complex_array geoFactorVarMapFFT( nI, nJ, nK );
        spectral::array tmp = spectral::shiftByHalf( geoFactorVarMap );
        spectral::foward( geoFactorVarMapFFT, tmp );
        spectral::complex_array geoFactorVarMapFFTpolar = spectral::to_polar_form( geoFactorVarMapFFT );
        spectral::array geoFactorVarMapFFTamplitudes = spectral::real( geoFactorVarMapFFTpolar );

        //get the square root of the amplitudes of the varmap FFT
        spectral::array geoFactorVarmapFFTamplitudesSQRT = geoFactorVarMapFFTamplitudes.sqrt();

        //convert sqrt(varmap) and the phases of the input to rectangular form
        spectral::complex_array geoFactorFFTpolar = spectral::to_complex_array(
                                                       geoFactorVarmapFFTamplitudesSQRT,
                                                       inputFFTimagPhase
                                                    );
        spectral::complex_array geoFactorFFT = spectral::to_rectangular_form( geoFactorFFTpolar );

        //compute the reverse FFT to get the geological factor
        spectral::array geoFactor( nI, nJ, nK, 0.0 );
        spectral::backward( geoFactor, geoFactorFFT );

        //fftw3's reverse FFT requires that the values of output be divided by the number of cells
        geoFactor = geoFactor / (double)( nI * nJ * nK );

        geoFactors.push_back( geoFactor );
        titles.push_back( QString( "Factor " + QString::number( iGeoFactor ) ).toStdString() );
        shiftFlags.push_back( false );
    }
    displayGrids( geoFactors, titles, shiftFlags );


    ///Visualizing the results on the fly is optional/////////////
//    {
//        spectral::array finalVariogramModelSurface( nI, nJ, nK, 0.0 );
//        for( int iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
//            variographicEllipses[iGeoFactor].addContributionToModelGrid( *inputGrid,
//                                                                         finalVariogramModelSurface,
//                                                                         IJVariogramPermissiveModel::SPHERIC,
//                                                                         true );
//        q3Dv.clearScene();
//        q3Dv.display( finalVariogramModelSurface, finalVariogramModelSurface.min(), finalVariogramModelSurface.max() );
//        QApplication::processEvents();
//        QMessageBox::information(this, "aaaa", "aaaa");
//    }
    ////////////////////////////////////////////////////////////

}

void VariographicDecompositionDialog::doVariographicDecomposition5_WITH_PSO()
{
    //get user configuration
    int m = ui->spinNumberOfGeologicalFactors->value();
    int maxNumberOfOptimizationSteps = ui->spinMaxSteps->value();
    // The user-given epsilon (useful for numerical calculus).
    int nParticles = 80; //number of wandering particles
    double intertia_weight = 0.3;
    double acceleration_constant_1 = 5;
    double acceleration_constant_2 = 5;

    // Get the data objects.
    IJAbstractCartesianGrid* inputGrid = m_gridSelector->getSelectedGrid();
    IJAbstractVariable* variable = m_variableSelector->getSelectedVariable();

    // Get the grid's dimensions.
    unsigned int nI = inputGrid->getNI();
    unsigned int nJ = inputGrid->getNJ();
    unsigned int nK = inputGrid->getNK();

    // Fetch data from the data source.
    inputGrid->dataWillBeRequested();

    //========================= GET VARMAP OF THE INPUT DATA =====================================

    //make an array full of zeroes (useful for certain steps in the workflow)
    spectral::array zeros( nI, nJ, nK, 0.0 );

    // Get the input data as a spectral::array object
    spectral::arrayPtr inputData( inputGrid->createSpectralArray( variable->getIndexInParentGrid() ) );

    //Compute FFT of input
    spectral::array inputFFTrealPart;
    spectral::array inputFFTimagPart;
    spectral::array inputFFTimagPhase;
    {
        spectral::complex_array inputFFT;
        spectral::foward( inputFFT, *inputData );
        inputFFTrealPart = spectral::real( inputFFT );
        inputFFTimagPart = spectral::imag( inputFFT );
        spectral::complex_array inputFFTpolar = spectral::to_polar_form( inputFFT );
        inputFFTimagPhase = spectral::imag( inputFFTpolar );
    }

    //do a^2 + b^2
    // where a = real part of FFT; b = imaginary part of FFT.
    spectral::array inputMagSquared = spectral::hadamard( inputFFTrealPart, inputFFTrealPart ) +
                                      spectral::hadamard( inputFFTimagPart, inputFFTimagPart );

    //make a complex array from a^2 + b^2 as magnitude and zeros as phase (polar form)
    //this corresponds to the FFT of a variographic map
    spectral::complex_array inputVarmapFFTPolarForm = spectral::to_complex_array( inputMagSquared, zeros );
    inputMagSquared = spectral::array(); //garbage collection

    //convert the previous complex array to the rectangular form
    spectral::complex_array inputVarmapFFTRectangularForm = spectral::to_rectangular_form( inputVarmapFFTPolarForm );
    inputVarmapFFTPolarForm = spectral::complex_array(); //garbage collection

    //get the varmap of the input data by reverse FFT
    spectral::array inputVarmap( nI, nJ, nK, 0.0 );
    spectral::backward( inputVarmap, inputVarmapFFTRectangularForm );
    inputVarmapFFTRectangularForm = spectral::complex_array(); //garbage collection

    //fftw requires that the values of r-FFT be divided by the number of cells
    inputVarmap = inputVarmap / (double)( nI * nJ * nK );

    //put h=0 of the varmap at the center of the grid
    {
        spectral::array ffVarMapTMP = spectral::shiftByHalf( inputVarmap );
        inputVarmap = ffVarMapTMP;
    }

    //================================== PREPARE OPTIMIZATION STEPS ==========================

    //define the domain
    double minCellSize = std::min( inputGrid->getCellSizeI(), inputGrid->getCellSizeJ() );
    double minAxis         = minCellSize;               double maxAxis         = inputGrid->getDiagonalLength() / 2.0;
    double minRatio        = 0.001;                     double maxRatio        = 1.0;
    double minAzimuth      = 0.0  ;                     double maxAzimuth      = ImageJockeyUtils::PI;
    double minContribution = inputVarmap.max() / 100.0; double maxContribution = inputVarmap.max();
    double deltaAxis = maxAxis - minAxis;
    double deltaRatio = maxRatio - minRatio;
    double deltaAzimuth = maxAzimuth - minAzimuth;
    double deltaContribution = maxContribution - minContribution;

    //create one variographic ellipse for each geological factor wanted by the user
    //the parameters are initialized near in the center of the domain
    //the starting values are not particuarly important.
    std::vector< IJVariographicStructure2D > variographicEllipses;
    for( int i = 0; i < m; ++i )
        variographicEllipses.push_back( IJVariographicStructure2D ( ( maxAxis         + minAxis         ) / 2.0,
                                                                    ( maxRatio        + minRatio        ) / 2.0,
                                                                    ( minAzimuth      + maxAzimuth      ) / 2.0,
                                                                    maxContribution / m ) ); //split evenly the total contribution among the geologic factors

    //Initialize the vector of linear system parameters [w]=[axis0,ratio0,az0,cc0,axis1,ratio1,...]
    // from the variographic parameters for each geological factor
    //the starting values are not particuarly important.
    spectral::array vw( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
    for( int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            vw[i] = variographicEllipses[iGeoFactor].getParameter( iPar );

    //Create a vector with the minimum values allowed for the parameters w
    //(see min* variables further up). DOMAIN CONSTRAINT
    spectral::array L_wMin( vw.size(), 0.0d );
    for(int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            switch( iPar ){
            case 0: L_wMin[i] = minAxis;         break;
            case 1: L_wMin[i] = minRatio;        break;
            case 2: L_wMin[i] = minAzimuth;      break;
            case 3: L_wMin[i] = minContribution; break;
            }

    //Create a vector with the maximum values allowed for the parameters w
    //(see max* variables further up). DOMAIN CONSTRAINT
    spectral::array L_wMax( vw.size(), 1.0d );
    for(int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            switch( iPar ){
            case 0: L_wMax[i] = maxAxis;         break;
            case 1: L_wMax[i] = maxRatio;        break;
            case 2: L_wMax[i] = maxAzimuth;      break;
            case 3: L_wMax[i] = maxContribution; break;
            }

    //-------------------------------------------------------------------------------------------------------------
    //------------------------------------- THE PARTICLE SWARM OPTIMIZATION ALGORITHM -----------------------------
    //-------------------------------------------------------------------------------------------------------------

    //Intialize the random number generator with the same seed
    std::srand ((unsigned)ui->spinSeed->value());

    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.show();
    progressDialog.setLabelText("Line Search with Restart in progress...");

    //Init the population of particles, their velocity vectors and their best position
    std::vector< spectral::array > particles_pw;
    std::vector< spectral::array > velocities_vw;
    std::vector< spectral::array > pbests_pbw;
    std::vector< double > fOfpbests;
    for( int iParticle = 0; iParticle < nParticles; ++iParticle ){
        //create a particle (one array of parameters)
        spectral::array pw( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
        //create a velocity vector (one array of velocities)
        spectral::array vw( pw.size() );
        //randomize the particle's position in the domain.
        for( int i = 0; i < pw.size(); ++i ){
            double LO = L_wMin[i];
            double HI = L_wMax[i];
            pw[i] = LO + std::rand() / (RAND_MAX/(HI-LO));
        }
        particles_pw.push_back( pw );
        //the velocities are initialized with zeros
        velocities_vw.push_back( vw );
        //the best position of a particle is initialized as the starting position
        spectral::array pbw( pw );
        pbests_pbw.push_back( pbw );
        //initialize the objective function value of the particle best as +inifinite
        fOfpbests.push_back( std::numeric_limits<double>::max() );
    }

    //Init the global best postion (best of the best positions amongst the particles)
    spectral::array gbest_pw;
    double fOfgbest = std::numeric_limits<double>::max();
    {
        double fOfBest = std::numeric_limits<double>::max();
        for( int iParticle = 0; iParticle < nParticles; ++iParticle ){
            //get the best postition of a particle
            spectral::array& pbw = pbests_pbw[ iParticle ] ;
            //evaluate the objective function with the best position of a particle
            double f = F3( *inputGrid, inputVarmap, pbw, m );
            //if it improves the value so far...
            if( f < fOfBest ){
                //...updates the best value record
                fOfBest = f;
                //...assigns the best of a particle as the global best
                gbest_pw = pbw;
            }
        }
    }

    //optimization steps
    for( int iStep = 0; iStep < maxNumberOfOptimizationSteps; ++iStep){

        //let Qt repaint the GUI every opt. step.
        QApplication::processEvents();

        //for each particle (vector of parameters)
        for( int iParticle = 0; iParticle < nParticles; ++iParticle ){

            //get the particle, its velocity and its best postion so far
            spectral::array& pw = particles_pw[ iParticle ];
            spectral::array& vw = velocities_vw[ iParticle ];
            spectral::array& pbw = pbests_pbw[ iParticle ];

            //get a candidate position and velocity of a particle
            spectral::array candidate_particle( pw.size() );
            spectral::array candidate_velocity( pw.size() );

            double rand1 = (std::rand()/(double)RAND_MAX);
            double rand2 = (std::rand()/(double)RAND_MAX);

            for( int i = 0; i < pw.size(); ++i ){
                candidate_velocity[i] = intertia_weight * vw[i] +
                                        acceleration_constant_1 * rand1 * ( pbw[i] - pw[i] ) +
                                        acceleration_constant_2 * rand2 * ( gbest_pw[i] - pw[i] );
                candidate_particle[i] = pw[i] + candidate_velocity[i];

                //performs a "bounce" of the particle if it "hits" the boundaries of the domain
                double overshoot = candidate_particle[i] - L_wMax[i];
                if( overshoot > 0 )
                    candidate_particle[i] = L_wMax[i] - overshoot;
                double undershoot = L_wMin[i] - candidate_particle[i];
                if( undershoot > 0 )
                    candidate_particle[i] = L_wMin[i] + undershoot;
            }

            //evaluate the objective function for current and candidate positions
            double fCurrent = F3( *inputGrid, inputVarmap, pw, m );
            double fCandidate = F3( *inputGrid, inputVarmap, candidate_particle, m );

            //if the candidate position improves the objective function
            if( fCandidate < fCurrent ){
                //update the postion
                pw = candidate_particle;
                //update the velocity
                vw = candidate_velocity;
            }

            //if the candidate position improves over the best of the particle
            if( fCandidate < fOfpbests[iParticle] ){
                //keep track of the best value of the objective function so far for the particle
                fOfpbests[iParticle] = fCandidate;
                //update the best position so far for the particle
                pbw = candidate_particle;
            }

            //if the candidate position improves over the global best
            if( fCandidate < fOfgbest ){
                //keep track of the global best value of the objective function
                fOfgbest = fCandidate;
                //update the global best position
                gbest_pw = candidate_particle;

                std::cout << fOfgbest << std::endl;
            }

        } // for each particle
    } // for each step

    //-------------------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------------------------
    progressDialog.hide();

    //Read the optimized variogram model parameters back to the variographic structures
    for( int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            variographicEllipses[iGeoFactor].setParameter( iPar, gbest_pw[i] );


    //Apply the principle of the Fourier Integral Method
    //use a variographic map as the magnitudes and the FFT phases of
    //the original data to a reverse FFT in polar form to achieve a
    //Factorial Kriging-like separation
    std::vector< spectral::array > geoFactors;
    std::vector< std::string > titles;
    std::vector< bool > shiftFlags;
    for( int iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor ) {
        //compute the varmap of the theoretical varmap for one geologic factor
        spectral::array geoFactorVarMap( nI, nJ, nK, 0.0 );
        variographicEllipses[iGeoFactor].addContributionToModelGrid( *inputGrid,
                                                                     geoFactorVarMap,
                                                                     IJVariogramPermissiveModel::SPHERIC,
                                                                     true );

        //collect the theoretical varmap for display
        geoFactors.push_back( geoFactorVarMap );
        titles.push_back( QString( "Varmap " + QString::number( iGeoFactor ) ).toStdString() );
        shiftFlags.push_back( false );

        //compute FFT of the theoretical varmap (into polar form)
        spectral::complex_array geoFactorVarMapFFT( nI, nJ, nK );
        spectral::array tmp = spectral::shiftByHalf( geoFactorVarMap );
        spectral::foward( geoFactorVarMapFFT, tmp );
        spectral::complex_array geoFactorVarMapFFTpolar = spectral::to_polar_form( geoFactorVarMapFFT );
        spectral::array geoFactorVarMapFFTamplitudes = spectral::real( geoFactorVarMapFFTpolar );

        //get the square root of the amplitudes of the varmap FFT
        spectral::array geoFactorVarmapFFTamplitudesSQRT = geoFactorVarMapFFTamplitudes.sqrt();

        //convert sqrt(varmap) and the phases of the input to rectangular form
        spectral::complex_array geoFactorFFTpolar = spectral::to_complex_array(
                                                       geoFactorVarmapFFTamplitudesSQRT,
                                                       inputFFTimagPhase
                                                    );
        spectral::complex_array geoFactorFFT = spectral::to_rectangular_form( geoFactorFFTpolar );

        //compute the reverse FFT to get the geological factor
        spectral::array geoFactor( nI, nJ, nK, 0.0 );
        spectral::backward( geoFactor, geoFactorFFT );

        //fftw3's reverse FFT requires that the values of output be divided by the number of cells
        geoFactor = geoFactor / (double)( nI * nJ * nK );

        geoFactors.push_back( geoFactor );
        titles.push_back( QString( "Factor " + QString::number( iGeoFactor ) ).toStdString() );
        shiftFlags.push_back( false );
    }
    displayGrids( geoFactors, titles, shiftFlags );
}

void VariographicDecompositionDialog::doVariographicDecomposition5_WITH_Genetic()
{
    //get user configuration
    int m = ui->spinNumberOfGeologicalFactors->value();
    int maxNumberOfGenerations = ui->spinMaxSteps->value();
    uint nPopulationSize = ui->spinMaxStepsAlphaReduction->value(); //number of individuals (sets of parameters)
    //selection pressure. 1.0 means only the best individual is selected during
    //the binary tournaments.  Lower values mean that the 2nd, 3rd, etc. best ones
    //have a chance to be selected
    uint nSelectionSize = ui->spinMaxStepsSA->value(); //the size of the selection pool (must be < nPopulationSize)
    double probabilityOfCrossOver = ui->spinMaxHopFactor->value();
    uint pointOfCrossover = ui->spinFinalTemperature->value(); //the index where crossover switches (must be less than the total number of parameters per individual)
    //mutation rate means how many paramaters are expected to change per mutation
    //the probability of any parameter parameter (gene) to be changed is 1/nParameters * mutationRate
    //thus, 1.0 means that one gene will surely be mutated per mutation on average.  Fractionary
    //values are possible. 0.0 means no mutation will take place.
    double mutationRate = ui->spinInitialAlpha->value();

    //the total number of genes (parameters) per individual.
    uint totalNumberOfParameters = m * IJVariographicStructure2D::getNumberOfParameters();

    //sanity checks
    if( nSelectionSize >= nPopulationSize ){
        QMessageBox::critical( this, "Error", "VariographicDecompositionDialog::doVariographicDecomposition5_WITH_Genetic(): Selection pool size must be less than population size.");
        return;
    }
    if( nPopulationSize % 2 + nSelectionSize % 2 ){
        QMessageBox::critical( this, "Error", "VariographicDecompositionDialog::doVariographicDecomposition5_WITH_Genetic(): Sizes must be even numbers.");
        return;
    }
    if( pointOfCrossover >= totalNumberOfParameters  ){
        QMessageBox::critical( this, "Error", "VariographicDecompositionDialog::doVariographicDecomposition5_WITH_Genetic(): Point of crossover must be less than the number of parameters.");
        return;
    }

    // Get the data objects.
    IJAbstractCartesianGrid* inputGrid = m_gridSelector->getSelectedGrid();
    IJAbstractVariable* variable = m_variableSelector->getSelectedVariable();

    // Get the grid's dimensions.
    unsigned int nI = inputGrid->getNI();
    unsigned int nJ = inputGrid->getNJ();
    unsigned int nK = inputGrid->getNK();

    // Fetch data from the data source.
    inputGrid->dataWillBeRequested();

    //========================= GET VARMAP OF THE INPUT DATA =====================================

    //make an array full of zeroes (useful for certain steps in the workflow)
    spectral::array zeros( nI, nJ, nK, 0.0 );

    // Get the input data as a spectral::array object
    spectral::arrayPtr inputData( inputGrid->createSpectralArray( variable->getIndexInParentGrid() ) );

    //Compute FFT of input
    spectral::array inputFFTrealPart;
    spectral::array inputFFTimagPart;
    spectral::array inputFFTimagPhase;
    {
        spectral::complex_array inputFFT;
        spectral::foward( inputFFT, *inputData );
        inputFFTrealPart = spectral::real( inputFFT );
        inputFFTimagPart = spectral::imag( inputFFT );
        spectral::complex_array inputFFTpolar = spectral::to_polar_form( inputFFT );
        inputFFTimagPhase = spectral::imag( inputFFTpolar );
    }

    //do a^2 + b^2
    // where a = real part of FFT; b = imaginary part of FFT.
    spectral::array inputMagSquared = spectral::hadamard( inputFFTrealPart, inputFFTrealPart ) +
                                      spectral::hadamard( inputFFTimagPart, inputFFTimagPart );

    //make a complex array from a^2 + b^2 as magnitude and zeros as phase (polar form)
    //this corresponds to the FFT of a variographic map
    spectral::complex_array inputVarmapFFTPolarForm = spectral::to_complex_array( inputMagSquared, zeros );
    inputMagSquared = spectral::array(); //garbage collection

    //convert the previous complex array to the rectangular form
    spectral::complex_array inputVarmapFFTRectangularForm = spectral::to_rectangular_form( inputVarmapFFTPolarForm );
    inputVarmapFFTPolarForm = spectral::complex_array(); //garbage collection

    //get the varmap of the input data by reverse FFT
    spectral::array inputVarmap( nI, nJ, nK, 0.0 );
    spectral::backward( inputVarmap, inputVarmapFFTRectangularForm );
    inputVarmapFFTRectangularForm = spectral::complex_array(); //garbage collection

    //fftw requires that the values of r-FFT be divided by the number of cells
    inputVarmap =  inputVarmap / (double)( nI * nJ * nK );

    ///////////////////////////////////////////////////
    for( int i = 0; i < inputVarmap.size(); ++i )
        if( inputVarmap[i] < 0.0 )
            inputVarmap[i] = 0.0;
    ///////////////////////////////////////////////////

    //put h=0 of the varmap at the center of the grid
    {
        spectral::array ffVarMapTMP = spectral::shiftByHalf( inputVarmap );
        inputVarmap = ffVarMapTMP;
    }

    //================================== PREPARE OPTIMIZATION STEPS ==========================

    //define the domain
    double minCellSize = std::min( inputGrid->getCellSizeI(), inputGrid->getCellSizeJ() );
    double minAxis         = minCellSize;               double maxAxis         = inputGrid->getDiagonalLength() / 2.0;
    double minRatio        = 0.001;                     double maxRatio        = 1.0;
    double minAzimuth      = 0.0  ;                     double maxAzimuth      = ImageJockeyUtils::PI;
    double minContribution = inputVarmap.max() / 100.0; double maxContribution = inputVarmap.max();

    //create one variographic ellipse for each geological factor wanted by the user
    //the parameters are initialized near in the center of the domain
    //the starting values are not particuarly important.
    std::vector< IJVariographicStructure2D > variographicEllipses;
    for( int i = 0; i < m; ++i )
        variographicEllipses.push_back( IJVariographicStructure2D ( ( maxAxis         + minAxis         ) / 2.0,
                                                                    ( maxRatio        + minRatio        ) / 2.0,
                                                                    ( minAzimuth      + maxAzimuth      ) / 2.0,
                                                                    maxContribution / m ) ); //split evenly the total contribution among the geologic factors

    //Initialize the vector of linear system parameters [w]=[axis0,ratio0,az0,cc0,axis1,ratio1,...]
    // from the variographic parameters for each geological factor
    //the starting values are not particuarly important.
    spectral::array vw( (spectral::index)totalNumberOfParameters );
    for( int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            vw[i] = variographicEllipses[iGeoFactor].getParameter( iPar );

    //Create a vector with the minimum values allowed for the parameters w
    //(see min* variables further up). DOMAIN CONSTRAINT
    spectral::array L_wMin( vw.size(), 0.0d );
    for(int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            switch( iPar ){
            case 0: L_wMin[i] = minAxis;         break;
            case 1: L_wMin[i] = minRatio;        break;
            case 2: L_wMin[i] = minAzimuth;      break;
            case 3: L_wMin[i] = minContribution; break;
            }

    //Create a vector with the maximum values allowed for the parameters w
    //(see max* variables further up). DOMAIN CONSTRAINT
    spectral::array L_wMax( vw.size(), 1.0d );
    for(int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            switch( iPar ){
            case 0: L_wMax[i] = maxAxis;         break;
            case 1: L_wMax[i] = maxRatio;        break;
            case 2: L_wMax[i] = maxAzimuth;      break;
            case 3: L_wMax[i] = maxContribution; break;
            }

    //=========================================THE GENETIC ALGORITHM==================================================

    //Intialize the random number generator with the same seed
    std::srand ((unsigned)ui->spinSeed->value());

    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.show();
    progressDialog.setLabelText("Genetic Algorithm in progress...");

    //the main algorithm loop
    std::vector< Individual > population;
    for( int iGen = 0; iGen < maxNumberOfGenerations; ++iGen ){

        //let Qt do repainting and respond to events during processing
        QApplication::processEvents();

        //Init or refill the population with randomly generated individuals.
        while( population.size() < nPopulationSize ){
            //create an individual (one array of parameters)
            spectral::array pw( (spectral::index)totalNumberOfParameters );
            //randomize the individual's position in the domain.
            for( int i = 0; i < pw.size(); ++i ){
                double LO = L_wMin[i];
                double HI = L_wMax[i];
                pw[i] = LO + std::rand() / (RAND_MAX/(HI-LO));
            }
            Individual ind( pw );
            population.push_back( ind );
        }

        //evaluate the individuals of current population
        for( uint iInd = 0; iInd < population.size(); ++iInd ){
            Individual& ind = population[iInd];
            ind.fValue = F3( *inputGrid, inputVarmap, ind.parameters, m );
        }

        //sort the population in ascending order (lower value == better fitness)
        std::sort( population.begin(), population.end() );

        //clip the population (the excessive worst fit individuals die)
        while( population.size() > nPopulationSize )
            population.pop_back();

        //perform selection by binary tournament
        std::vector< Individual > selection;
        for( uint iSel = 0; iSel < nSelectionSize; ++iSel ){
            //perform binary tournament
            std::vector< Individual > tournament;
            {
                //draw two different individuals at random from the population for the tournament.
                int tournCandidate1 = std::rand() / (double)RAND_MAX * ( population.size() - 1 );
                int tournCandidate2 = tournCandidate1;
                while( tournCandidate2 == tournCandidate1 )
                    tournCandidate2 = std::rand() / (double)RAND_MAX * ( population.size() - 1 );
                //add the participants in the tournament
                tournament.push_back( population[tournCandidate1] );
                tournament.push_back( population[tournCandidate2] );
                //sort the binary tournament
                std::sort( tournament.begin(), tournament.end());
            }
            //add the best of tournament to the selection pool
            selection.push_back( tournament.front() );
        }

        //perform crossover and mutation on the selected individuals
        std::vector< Individual > nextGen;
        while( ! selection.empty() ){
            //draw two different selected individuals at random for crossover.
            int parentIndex1 = std::rand() / (double)RAND_MAX * ( selection.size() - 1 );
            int parentIndex2 = parentIndex1;
            while( parentIndex2 == parentIndex1 )
                parentIndex2 = std::rand() / (double)RAND_MAX * ( selection.size() - 1 );
            Individual parent1 = selection[ parentIndex1 ];
            Individual parent2 = selection[ parentIndex2 ];
            selection.erase( selection.begin() + parentIndex1 );
            selection.erase( selection.begin() + parentIndex2 );
            //draw a value between 0.0 and 1.0 from an uniform distribution
            double p = std::rand() / (double)RAND_MAX;
            //if crossover is due...
            if( p < probabilityOfCrossOver ){
                //crossover
                std::pair< Individual, Individual> offspring = parent1.crossOver( parent2, pointOfCrossover );
                Individual child1 = offspring.first;
                Individual child2 = offspring.second;
                //mutate all
                child1.mutate( mutationRate, L_wMin, L_wMax );
                child2.mutate( mutationRate, L_wMin, L_wMax );
                parent1.mutate( mutationRate, L_wMin, L_wMax );
                parent2.mutate( mutationRate, L_wMin, L_wMax );
                //add them to the next generation pool
                nextGen.push_back( child1 );
                nextGen.push_back( child2 );
                nextGen.push_back( parent1 );
                nextGen.push_back( parent2 );
            } else { //no crossover took place
                //simply mutate and insert the parents into the next generation pool
                parent1.mutate( mutationRate, L_wMin, L_wMax );
                parent2.mutate( mutationRate, L_wMin, L_wMax );
                nextGen.push_back( parent1 );
                nextGen.push_back( parent2 );
            }
        }

        //make the next generation
        population = nextGen;

    } //main algorithm loop

    //=====================================GET RESULTS========================================
    progressDialog.hide();

    //evaluate the individuals of final population
    for( uint iInd = 0; iInd < population.size(); ++iInd ){
        Individual& ind = population[iInd];
        //////////////////////////////
//        ind.parameters[0] = 1.0;
//        ind.parameters[1] = 1.0;
//        ind.parameters[2] = 0.0;
//        ind.parameters[3] = 0.0;
//        ind.parameters[4] = 30.0;
//        ind.parameters[5] = 1.0/3.0;
//        ind.parameters[6] = - ImageJockeyUtils::PI * 41 / 180.0 ;
//        ind.parameters[7] = inputVarmap.max();
        //////////////////////////////
        ind.fValue = F3( *inputGrid, inputVarmap, ind.parameters, m );
    }

    //sort the population in ascending order (lower value == better fitness)
    std::sort( population.begin(), population.end() );

    //get the parameters of the best individual (set of parameters)
    spectral::array gbest_pw = population[0].parameters;

    std::cout << population[0].fValue << std::endl;

    //Read the optimized variogram model parameters back to the variographic structures
    for( int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            variographicEllipses[iGeoFactor].setParameter( iPar, gbest_pw[i] );

    //Apply the principle of the Fourier Integral Method
    //use a variographic map as the magnitudes and the FFT phases of
    //the original data to a reverse FFT in polar form to achieve a
    //Factorial Kriging-like separation
    std::vector< spectral::array > geoFactors;
    std::vector< std::string > titles;
    std::vector< bool > shiftFlags;
    for( int iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor ) {
        //compute the varmap of the theoretical varmap for one geologic factor
        spectral::array geoFactorVarMap( nI, nJ, nK, 0.0 );
        variographicEllipses[iGeoFactor].addContributionToModelGrid( *inputGrid,
                                                                     geoFactorVarMap,
                                                                     IJVariogramPermissiveModel::SPHERIC,
                                                                     true );

        //collect the theoretical varmap for display
        geoFactors.push_back( geoFactorVarMap );
        titles.push_back( QString( "Varmap " + QString::number( iGeoFactor ) ).toStdString() );
        shiftFlags.push_back( false );

        //compute FFT of the theoretical varmap (into polar form)
        spectral::complex_array geoFactorVarMapFFT( nI, nJ, nK );
        spectral::array tmp = spectral::shiftByHalf( geoFactorVarMap );
        spectral::foward( geoFactorVarMapFFT, tmp );
        spectral::complex_array geoFactorVarMapFFTpolar = spectral::to_polar_form( geoFactorVarMapFFT );
        spectral::array geoFactorVarMapFFTamplitudes = spectral::real( geoFactorVarMapFFTpolar );

        //get the square root of the amplitudes of the varmap FFT
        spectral::array geoFactorVarmapFFTamplitudesSQRT = geoFactorVarMapFFTamplitudes.sqrt();

        //convert sqrt(varmap) and the phases of the input to rectangular form
        spectral::complex_array geoFactorFFTpolar = spectral::to_complex_array(
                                                       geoFactorVarmapFFTamplitudesSQRT,
                                                       inputFFTimagPhase
                                                    );
        spectral::complex_array geoFactorFFT = spectral::to_rectangular_form( geoFactorFFTpolar );

        //compute the reverse FFT to get the geological factor
        spectral::array geoFactor( nI, nJ, nK, 0.0 );
        spectral::backward( geoFactor, geoFactorFFT );

        //fftw3's reverse FFT requires that the values of output be divided by the number of cells
        geoFactor = geoFactor / (double)( nI * nJ * nK );

        geoFactors.push_back( geoFactor );
        titles.push_back( QString( "Factor " + QString::number( iGeoFactor ) ).toStdString() );
        shiftFlags.push_back( false );
    }
    ////////////////////////////////////////
    spectral::array variograficSurface( nI, nJ, nK, 0.0 );
    for( spectral::array& geoFactor : geoFactors ){
        variograficSurface += geoFactor;
    }
    geoFactors.push_back( variograficSurface );
    titles.push_back( QString( "variogram model surface" ).toStdString() );
    shiftFlags.push_back( false );
    ////////////////////////////////////////
    ////////////////////////////////////////
    geoFactors.push_back( inputVarmap );
    titles.push_back( QString( "Varmap of input" ).toStdString() );
    shiftFlags.push_back( false );
    ////////////////////////////////////////
    displayGrids( geoFactors, titles, shiftFlags );
}

void VariographicDecompositionDialog::doVariographicDecomposition5_WITH_BruteForce()
{
    //get user configuration
    int m = ui->spinNumberOfGeologicalFactors->value();
    int maxNumberOfSteps = ui->spinMaxSteps->value();
    uint nPopulationSize = ui->spinMaxStepsAlphaReduction->value(); //number of solutions (sets of parameters)

    //the total number of genes (parameters) per individual.
    uint totalNumberOfParameters = m * IJVariographicStructure2D::getNumberOfParameters();

    // Get the data objects.
    IJAbstractCartesianGrid* inputGrid = m_gridSelector->getSelectedGrid();
    IJAbstractVariable* variable = m_variableSelector->getSelectedVariable();

    // Get the grid's dimensions.
    unsigned int nI = inputGrid->getNI();
    unsigned int nJ = inputGrid->getNJ();
    unsigned int nK = inputGrid->getNK();

    // Fetch data from the data source.
    inputGrid->dataWillBeRequested();

    //========================= GET VARMAP OF THE INPUT DATA =====================================

    //make an array full of zeroes (useful for certain steps in the workflow)
    spectral::array zeros( nI, nJ, nK, 0.0 );

    // Get the input data as a spectral::array object
    spectral::arrayPtr inputData( inputGrid->createSpectralArray( variable->getIndexInParentGrid() ) );

    //Compute FFT of input
    spectral::array inputFFTrealPart;
    spectral::array inputFFTimagPart;
    spectral::array inputFFTimagPhase;
    {
        spectral::complex_array inputFFT;
        spectral::foward( inputFFT, *inputData );
        inputFFTrealPart = spectral::real( inputFFT );
        inputFFTimagPart = spectral::imag( inputFFT );
        spectral::complex_array inputFFTpolar = spectral::to_polar_form( inputFFT );
        inputFFTimagPhase = spectral::imag( inputFFTpolar );
    }

    //do a^2 + b^2
    // where a = real part of FFT; b = imaginary part of FFT.
    spectral::array inputMagSquared = spectral::hadamard( inputFFTrealPart, inputFFTrealPart ) +
                                      spectral::hadamard( inputFFTimagPart, inputFFTimagPart );

    //make a complex array from a^2 + b^2 as magnitude and zeros as phase (polar form)
    //this corresponds to the FFT of a variographic map
    spectral::complex_array inputVarmapFFTPolarForm = spectral::to_complex_array( inputMagSquared, zeros );
    inputMagSquared = spectral::array(); //garbage collection

    //convert the previous complex array to the rectangular form
    spectral::complex_array inputVarmapFFTRectangularForm = spectral::to_rectangular_form( inputVarmapFFTPolarForm );
    inputVarmapFFTPolarForm = spectral::complex_array(); //garbage collection

    //get the varmap of the input data by reverse FFT
    spectral::array inputVarmap( nI, nJ, nK, 0.0 );
    spectral::backward( inputVarmap, inputVarmapFFTRectangularForm );
    inputVarmapFFTRectangularForm = spectral::complex_array(); //garbage collection

    //fftw requires that the values of r-FFT be divided by the number of cells
    inputVarmap = inputVarmap / (double)( nI * nJ * nK );

    //put h=0 of the varmap at the center of the grid
    {
        spectral::array ffVarMapTMP = spectral::shiftByHalf( inputVarmap );
        inputVarmap = ffVarMapTMP;
    }

    //================================== PREPARE OPTIMIZATION STEPS ==========================

    //define the domain
    double minCellSize = std::min( inputGrid->getCellSizeI(), inputGrid->getCellSizeJ() );
    double minAxis         = minCellSize;               double maxAxis         = inputGrid->getDiagonalLength() / 2.0;
    double minRatio        = 0.001;                     double maxRatio        = 1.0;
    double minAzimuth      = 0.0  ;                     double maxAzimuth      = ImageJockeyUtils::PI;
    double minContribution = inputVarmap.max() / 100.0; double maxContribution = inputVarmap.max();

    //create one variographic ellipse for each geological factor wanted by the user
    //the parameters are initialized near in the center of the domain
    //the starting values are not particuarly important.
    std::vector< IJVariographicStructure2D > variographicEllipses;
    for( int i = 0; i < m; ++i )
        variographicEllipses.push_back( IJVariographicStructure2D ( ( maxAxis         + minAxis         ) / 2.0,
                                                                    ( maxRatio        + minRatio        ) / 2.0,
                                                                    ( minAzimuth      + maxAzimuth      ) / 2.0,
                                                                    maxContribution / m ) ); //split evenly the total contribution among the geologic factors

    //Initialize the vector of linear system parameters [w]=[axis0,ratio0,az0,cc0,axis1,ratio1,...]
    // from the variographic parameters for each geological factor
    //the starting values are not particuarly important.
    spectral::array vw( (spectral::index)totalNumberOfParameters );
    for( int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            vw[i] = variographicEllipses[iGeoFactor].getParameter( iPar );

    //Create a vector with the minimum values allowed for the parameters w
    //(see min* variables further up). DOMAIN CONSTRAINT
    spectral::array L_wMin( vw.size(), 0.0d );
    for(int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            switch( iPar ){
            case 0: L_wMin[i] = minAxis;         break;
            case 1: L_wMin[i] = minRatio;        break;
            case 2: L_wMin[i] = minAzimuth;      break;
            case 3: L_wMin[i] = minContribution; break;
            }

    //Create a vector with the maximum values allowed for the parameters w
    //(see max* variables further up). DOMAIN CONSTRAINT
    spectral::array L_wMax( vw.size(), 1.0d );
    for(int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            switch( iPar ){
            case 0: L_wMax[i] = maxAxis;         break;
            case 1: L_wMax[i] = maxRatio;        break;
            case 2: L_wMax[i] = maxAzimuth;      break;
            case 3: L_wMax[i] = maxContribution; break;
            }

    //=========================================THE BRUTE FORCE ALGORITHM==================================================

    //Intialize the random number generator with the same seed
    std::srand ((unsigned)ui->spinSeed->value());

    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.show();
    progressDialog.setLabelText("Brute force algorithm in progress...");

    //the main algorithm loop
    Solution bestSolution( totalNumberOfParameters );
    for( int iStep = 0; iStep < maxNumberOfSteps; ++iStep ){
        //let Qt do repainting and respond to events during processing
        QApplication::processEvents();

        std::vector< Solution > solutions;

        //Init the randomly generated solutions within the domain.
        while( solutions.size() < nPopulationSize ){
            //create a solution (one array of parameters)
            spectral::array parameters( (spectral::index)totalNumberOfParameters );
            //randomize the solution's parameters in the domain.
            for( int i = 0; i < parameters.size(); ++i ){
                double LO = L_wMin[i];
                double HI = L_wMax[i];
                parameters[i] = LO + std::rand() / (RAND_MAX/(HI-LO));
            }
            solutions.push_back( Solution( parameters ) );
        }

        //evaluate the solutions of current set
        for( uint iSol = 0; iSol < solutions.size(); ++iSol ){
            Solution& solution = solutions[iSol];
            solution.fValue = F3( *inputGrid, inputVarmap, solution.parameters, m );
        }

        //sort the solution set in ascending order (lower value == better fitness)
        std::sort( solutions.begin(), solutions.end() );

        //saves the best solution if it was improved
        if( bestSolution.fValue > solutions[0].fValue ){
            bestSolution = solutions[0];
            std::cout << bestSolution.fValue << std::endl;
        }

    } //main algorithm loop

    //=====================================GET RESULTS========================================
    progressDialog.hide();

    //get the parameters of the best solution (set of parameters)
    spectral::array gbest_pw = bestSolution.parameters;

    //Read the optimized variogram model parameters back to the variographic structures
    for( int i = 0, iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            variographicEllipses[iGeoFactor].setParameter( iPar, gbest_pw[i] );

    //Apply the principle of the Fourier Integral Method
    //use a variographic map as the magnitudes and the FFT phases of
    //the original data to a reverse FFT in polar form to achieve a
    //Factorial Kriging-like separation
    std::vector< spectral::array > geoFactors;
    std::vector< std::string > titles;
    std::vector< bool > shiftFlags;
    for( int iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor ) {
        //compute the varmap of the theoretical varmap for one geologic factor
        spectral::array geoFactorVarMap( nI, nJ, nK, 0.0 );
        variographicEllipses[iGeoFactor].addContributionToModelGrid( *inputGrid,
                                                                     geoFactorVarMap,
                                                                     IJVariogramPermissiveModel::SPHERIC,
                                                                     true );

        //collect the theoretical varmap for display
        geoFactors.push_back( geoFactorVarMap );
        titles.push_back( QString( "Varmap " + QString::number( iGeoFactor ) ).toStdString() );
        shiftFlags.push_back( false );

        //compute FFT of the theoretical varmap (into polar form)
        spectral::complex_array geoFactorVarMapFFT( nI, nJ, nK );
        spectral::array tmp = spectral::shiftByHalf( geoFactorVarMap );
        spectral::foward( geoFactorVarMapFFT, tmp );
        spectral::complex_array geoFactorVarMapFFTpolar = spectral::to_polar_form( geoFactorVarMapFFT );
        spectral::array geoFactorVarMapFFTamplitudes = spectral::real( geoFactorVarMapFFTpolar );

        //get the square root of the amplitudes of the varmap FFT
        spectral::array geoFactorVarmapFFTamplitudesSQRT = geoFactorVarMapFFTamplitudes.sqrt();

        //convert sqrt(varmap) and the phases of the input to rectangular form
        spectral::complex_array geoFactorFFTpolar = spectral::to_complex_array(
                                                       geoFactorVarmapFFTamplitudesSQRT,
                                                       inputFFTimagPhase
                                                    );
        spectral::complex_array geoFactorFFT = spectral::to_rectangular_form( geoFactorFFTpolar );

        //compute the reverse FFT to get the geological factor
        spectral::array geoFactor( nI, nJ, nK, 0.0 );
        spectral::backward( geoFactor, geoFactorFFT );

        //fftw3's reverse FFT requires that the values of output be divided by the number of cells
        geoFactor = geoFactor / (double)( nI * nJ * nK );

        geoFactors.push_back( geoFactor );
        titles.push_back( QString( "Factor " + QString::number( iGeoFactor ) ).toStdString() );
        shiftFlags.push_back( false );
    }
    displayGrids( geoFactors, titles, shiftFlags );

}

void VariographicDecompositionDialog::displayGrid(const spectral::array & grid, const std::string & title, bool shiftByHalf)
{
	//Create the structure to store the geological factors
	SVDFactorTree * factorTree = new SVDFactorTree( 0.0 ); //the split factor of 0.0 has no special meaning here
	//make a local copy of the grid data
	spectral::array geoFactorDataCopy;
	if( shiftByHalf )
		geoFactorDataCopy = spectral::shiftByHalf( grid );
	else
		geoFactorDataCopy = spectral::array( grid );
	//Create a displayble object from the grid data
	//This pointer will be managed by the SVDFactorTree object.
	SVDFactor* geoFactor = new SVDFactor( std::move(geoFactorDataCopy), 1, 1.0,
											0, 0, 0, 1, 1, 1, 0.0);
	//Declare it as a geological factor (decomposable, not fundamental)
	geoFactor->setType( SVDFactorType::GEOLOGICAL );
	geoFactor->setCustomName( QString( title.c_str() ) );
	//add the displayable object to the factor tree (container)
	factorTree->addFirstLevelFactor( geoFactor );
	//use the SVD analysis dialog to display the geological factors.
	//NOTE: do not use heap to allocate the dialog, unless you remove the Qt::WA_DeleteOnClose behavior of the dialog.
	SVDAnalysisDialog* svdad = new SVDAnalysisDialog( nullptr );
	svdad->setTree( factorTree );
	svdad->setDeleteTreeOnClose( true ); //the three and all data it contains will be deleted on dialog close
	svdad->exec(); //open the dialog modally
}

void VariographicDecompositionDialog::doSVDonData(const spectral::array* gridInputData,
												  double infoContentToKeepForSVD,
												  std::vector<spectral::array> & svdFactors)
{
	//Get the number of usable fundamental SVD factors.
	{
		int n = 0;
		spectral::SVD svd = spectral::svd( *gridInputData );
		//get the list with the factor weights (information quantity)
		spectral::array weights = svd.factor_weights();
		//get the number of fundamental factors that have the total information content as specified by the user.
		{
			double cumulative = 0.0;
			uint i = 0;
			for(; i < weights.size(); ++i){
				cumulative += weights.d_[i];
				if( cumulative > infoContentToKeepForSVD )
					break;
			}
			n = i+1;
		}
		if( n < 3 ){
			QMessageBox::warning( this, "Warning", "The data must be decomposable into at least three usable fundamental factors to proceed.");
			return;
		} else {
			emit info( "Using " + QString::number(n) + " fundamental factors out of " + QString::number(weights.size()) +
					   " to cover " + QString::number(ui->spinInfoContentToKeepForSVD->value()) + "% of information content." );
		}
		//Get the usable fundamental SVD factors.
		{
			QProgressDialog progressDialog;
			progressDialog.setRange(0,0);
			progressDialog.show();
			progressDialog.setLabelText("Retrieving fundamental SVD factors...");
			QCoreApplication::processEvents();
			for (long i = 0; i < n; ++i) {
				spectral::array factor = svd.factor(i);
				svdFactors.push_back( std::move( factor ) );
			}
		}
	}
}

void VariographicDecompositionDialog::doFourierPartitioningOnData(const spectral::array * gridInputData,
																  std::vector<spectral::array> & frequencyFactors,
																  int nTracks )
{
	///Visualizing the results on the fly is optional/////////////
	IJQuick3DViewer q3Dv;
	q3Dv.show();
	///////////////////////////////////////////////////////////////

	///////////////////////////////// ACESS GRID GEOMETRY ///////////////////////////

	double cellWidth = 1.0;
	double cellHeight = 1.0;
	// double cellThickness = 1.0; // distance criterion currently only in 2D.
	double gridWidth = gridInputData->M() * cellWidth;
	double gridHeight = gridInputData->N() * cellHeight;
	// double gridDepth = gridInputData.K() * cellThickness; // distance criterion currently only in 2D.
	int nI = gridInputData->M();
	int nJ = gridInputData->N();
	int nK = gridInputData->K();

	////////////////// DEFINE SOME STRUCTURES TO ORGANIZE CODE ///////////////////////

	// A Sector is a division of a HalfTrack spanning some angles.
	struct Sector{
		double startAzimuth;
		double endAzimuth;
		spectral::array grid;
		bool wasTouched = false;
		void makeGrid( int pnI, int pnJ, int pnK ){
			grid = spectral::array( pnI, pnJ, pnK, 0.0d );
		}
		bool isAligned( double azimuth ){
			return azimuth >= startAzimuth && azimuth < endAzimuth;
		}
		bool assignValue( double azimuth, double value, int i, int j, int k ){
			if( isAligned( azimuth )){
				grid( i, j, k ) = value;
				wasTouched = true;
				return true;
			}
			return false;
		}
	};

	// A HalfTrack is a half-circular band centered at the grid's center and arcing between N000E (north) and N180E (south)
	// passing through east (N090E).  The spectrogram of a real-valued grid is symmetric, meaning that a values
	// along the N045E azimuth, for example, equals those along the N225E azimuth, thus it is not necessary to model
	// an all-around track.	It is divided into Sectors.
	struct HalfTrack{
		double innerRadius;
		double outerRadius;
		int index;
		std::vector< Sector > sectors;
		void makeSectors( int pnI, int pnJ, int pnK ){
			int nSectors = 1;
			if( index > 0 )
				nSectors = 2 * (index+1); //innermost track = 1 sector, then 4, then 6, then 8, then 10...
			double azimuthSpan = 180.0d / nSectors;
			double currentStartAzimuth = 0.0;
			double currentEndAzimuth = azimuthSpan;
			for( int i = 0; i < nSectors; ++i ){
				Sector sector;
				sector.startAzimuth = currentStartAzimuth;
				sector.endAzimuth = currentEndAzimuth;
				sector.makeGrid( pnI, pnJ, pnK );
				currentStartAzimuth += azimuthSpan;
				currentEndAzimuth += azimuthSpan;
				sectors.push_back( sector );
			}
		}
		bool isInside( double distance ){
			return distance >= innerRadius && distance < outerRadius;
		}
		bool assignValue( double distance, double azimuth, double value, int i, int j, int k ){
			if( isInside( distance ) ){
				std::vector< Sector >::iterator itSector = sectors.begin();
				for( ; itSector != sectors.end(); ++itSector ){
					bool wasAssigned = (*itSector).assignValue( azimuth, value, i, j, k );
					if( wasAssigned )
						return true; //abort the search if a matching sector was found.
				}
			}
			return false;
		}
	};

	///////////////////// BEGIN OF ALGORITHM ITSELF //////////////////////////////////

	//Compute FFT of the input data
	spectral::array inputFFTmagnitudes;
	spectral::array inputFFTphases;
	{
		spectral::array inputCopy( *gridInputData );
		spectral::complex_array inputFFT;
		spectral::foward( inputFFT, inputCopy );
		inputFFT = spectral::to_polar_form( inputFFT );
		inputFFTmagnitudes = spectral::real( inputFFT );
		inputFFTphases = spectral::imag( inputFFT );
	}

	//Shift the Fourier image so that frequency zero is in the grid center.
	inputFFTmagnitudes = spectral::shiftByHalf( inputFFTmagnitudes );

	// Compute the track geometries.
	std::vector< HalfTrack > tracks;
	{
		// Get global geometry.
		double diagonal = std::sqrt( gridWidth*gridWidth + gridHeight*gridHeight );
		double trackWidth = (diagonal/2) / nTracks;
		double currentInnerRadius = 0.0;
		double currentOuterRadius = trackWidth;
		// Create the tracks from center outwards.
		for( int i = 0; i < nTracks; ++i){
			HalfTrack track;
			track.innerRadius = currentInnerRadius;
			track.outerRadius = currentOuterRadius;
			track.index = i;
			track.makeSectors( nI, nJ, nK );
			tracks.push_back( track );
			currentInnerRadius += trackWidth;
			currentOuterRadius += trackWidth;
		}
	}

	// Scan the Fourier image, assigning cell values to the tracks/sectors.
	double gridCenterX = gridWidth/2;
	double gridCenterY = gridHeight/2;
	// double gridCenterZ = gridDepth/2; // distance criterion currently only in 2D.
	for( int i = 0; i < inputFFTmagnitudes.M(); ++i ){
		double cellCenterX = cellWidth/2 + i * cellHeight;
		for( int j = 0; j < inputFFTmagnitudes.N(); ++j ){
			double cellCenterY = cellHeight/2 + j * cellHeight;
			for( int k = 0; k < inputFFTmagnitudes.K(); ++k ){
				// double cellCenterZ = cellThickness/2 + k * cellThickness; // distance criterion currently only in 2D.
				double dX = cellCenterX - gridCenterX;
				double dY = cellCenterY - gridCenterY;
				// double dZ = cellCenterZ - gridCenterZ; // distance criterion currently only in 2D.
				double distance = std::sqrt( dX*dX + dY*dY /*+ dZ*dZ*/ ); // distance criterion currently only in 2D.
				double azimuth = ImageJockeyUtils::getAzimuth( cellCenterX, cellCenterY, gridCenterX, gridCenterY, true );
				std::vector< HalfTrack >::iterator trackIt = tracks.begin();
				for( ; trackIt != tracks.end(); ++trackIt ){
					bool wasAssigned = (*trackIt).assignValue( distance, azimuth, inputFFTmagnitudes(i,j,k), i, j, k );
					if( wasAssigned )
						break; //abort the search if a matching sector in a matching track was found.
				}
			}
		}
	}

	// Discard all-zeros factors ( resulted from sectors out of grid boundaries ).
	{
		std::vector< HalfTrack >::iterator trackIt = tracks.begin();
		int totalTracks = 0;
		for( ; trackIt != tracks.end(); ++trackIt ){
			std::vector< Sector >::iterator itSector = (*trackIt).sectors.begin();
			for( ; itSector != (*trackIt).sectors.end(); ){ // erase() already increments the iterator.
				if( ! (*itSector).wasTouched )
					itSector = (*trackIt).sectors.erase( itSector );
				else{
					++itSector;
					++totalTracks;
				}
			}
		}
		emit info( "VariographicDecompositionDialog::doFourierPartitioningOnData(): " + QString::number( totalTracks ) + " fundamental frequency factors." );
	}

	// De-shift all factors so they become compatible with reverse FFT.
	{
		std::vector< HalfTrack >::iterator trackIt = tracks.begin();
		for( ; trackIt != tracks.end(); ++trackIt ){
			std::vector< Sector >::iterator itSector = (*trackIt).sectors.begin();
			for( ; itSector != (*trackIt).sectors.end(); ++itSector ){
				(*itSector).grid = spectral::shiftByHalf( (*itSector).grid );
			}
		}
	}

	// Compute RFFT for all factors and return them via the output collection.
	{
		std::vector< HalfTrack >::iterator trackIt = tracks.begin();
		for( ; trackIt != tracks.end(); ++trackIt ){
			std::vector< Sector >::iterator itSector = (*trackIt).sectors.begin();
			for( ; itSector != (*trackIt).sectors.end(); ++itSector ){
				spectral::complex_array input = spectral::to_complex_array( (*itSector).grid, inputFFTphases );
				spectral::array backtrans( nI, nJ, nK, 0.0d );
				input = spectral::to_rectangular_form( input );
				spectral::backward( backtrans, input );
				(*itSector).grid = backtrans / static_cast<double>(nI * nJ * nK);
				frequencyFactors.push_back( (*itSector).grid );
			}
		}
	}

	///Visualizing the results on the fly is optional/////////////
	{
		std::vector< HalfTrack >::iterator trackIt = tracks.begin();
		for( ; trackIt != tracks.end(); ++trackIt ){
			std::vector< Sector >::iterator itSector = (*trackIt).sectors.begin();
			for( ; itSector != (*trackIt).sectors.end(); ++itSector ){
				q3Dv.clearScene();
				q3Dv.display( (*itSector).grid, (*itSector).grid.min(), (*itSector).grid.max() );
			}
		}
	}
	////////////////////////////////////////////////////////////

}

void VariographicDecompositionDialog::doGaborAnalysisOnData(const spectral::array *gridInputData,
                                                            std::vector<spectral::array> &frequencyFactors,
                                                            const GaborAnalysisParameters &gaborParameters)
{
    ///Visualizing the results on the fly is optional/////////////
    IJQuick3DViewer q3Dv;
    q3Dv.show();
    ///////////////////////////////////////////////////////////////

    double absAvgInput = std::abs( gridInputData->avg() );

    //get input grid sizes
    spectral::index nI = gridInputData->M();
    spectral::index nJ = gridInputData->N();

    //compute the FFT of the input data in polar form (magnitude and phase).
    spectral::complex_array inputFFTpolar;
    {
        spectral::complex_array inputFFT;
        spectral::array inputData( *gridInputData );
        spectral::foward( inputFFT, inputData );
        inputFFTpolar = spectral::to_polar_form( inputFFT );
    }


    //this lambda interpolates between the initial frequency (f0)
    //and final frequency(f1) logarithmically (concavity upwards)
    //
    // readable formula:
    //
    //    log(f1) - log(f0)       s1 - s0
    //  --------------------- = -----------
    //    log(f)  - log(f0)        s - s0
    //
    //  Where:
    //
    //  f: output interpolated frequency
    //  s: input step number
    //  s1 and s0: final and intial step numbers.
    //  f1 and f0: final and initial frequencies.
    //
    int s1 = 10;
    int s0 = 0;
    double f1 = gaborParameters.finalFrequency;
    double f0 = gaborParameters.initialFrequency;
//    std::function<double (int)> f = [ s0, s1, f0, f1 ](int s)
//                          { return std::pow(10.0,
//                                  ( (s-s0)*(std::log10(f1)-std::log10(f0))/(s1-s0) ) + std::log10( f0 )
//                                            ); };
        std::function<double (int)> f = [ s0, s1, f0, f1 ](int s)
                              { return f1 - f0 - std::pow(10.0,
                                      ( (s-s0)*(std::log10(f0)-std::log10(f1))/(s1-s0) ) + std::log10( f1 )
                                                ); };


    //for each frequency
    int fCount = 0;
    for( int i = 0; i < s1; ++i, ++fCount ){

        double frequency = f(i);

        double azDiv = 1;
        if( fCount > 0 )
            azDiv = ui->txtAzimuthPartitionFactorForGabor->text().toDouble() * (fCount+1); //lowest frequency = 1 azimuth, then 4, then 6, then 8, then 10...
        double azStep = 180.0 / azDiv;

        //for each azimuth
        for( double azimuth = 0.0; azimuth <= 180.0; azimuth += azStep ){

            //compute the unitized (min. = 0.0, max. = 1.0) amplitude of FFT of the Gabor kernel of a given azimuth and frequency.
            spectral::array kernelSPaddedFFTamplUnitized;
            {
                //make a Gabor kernel
                GaborUtils::ImageTypePtr kernel = GaborUtils::createGaborKernel(frequency,
                                                                                azimuth,
                                                                                gaborParameters.kernelMeanMajorAxis,
                                                                                gaborParameters.kernelMeanMinorAxis,
                                                                                gaborParameters.kernelSigmaMajorAxis,
                                                                                gaborParameters.kernelSigmaMinorAxis,
                                                                                gaborParameters.kernelSize,
                                                                                gaborParameters.kernelSize,
                                                                                false );
                //convert it to a spectral::array
                spectral::array kernelS = GaborUtils::convertITKImageToSpectralArray( *kernel );

                {
                    q3Dv.clearScene();
                    spectral::array a(kernelS);
                    q3Dv.display( a, a.min(), a.max() );
                    QApplication::processEvents(); //let Qt repaint GUI elements
                    //QMessageBox::information( nullptr, "hhh", "aaa");
                }

                //makes it compatible with inner multiplication with the input grid (same size)
                spectral::array kernelSPadded = spectral::project( kernelS, nI, nJ, 1 );

                //compute the FFT of the kernel
                spectral::complex_array kernelSPaddedFFT;
                spectral::foward( kernelSPaddedFFT, kernelSPadded );

                //put the complex numbers into polar form
                spectral::complex_array kernelSPaddedFFTpolar =
                        spectral::to_polar_form( kernelSPaddedFFT );

                //get the amplitde values.
                spectral::array kernelSPaddedFFTampl = spectral::real( kernelSPaddedFFTpolar );

                //rescale the amplitudes to 0.0-1.0
                kernelSPaddedFFTamplUnitized = kernelSPaddedFFTampl / kernelSPaddedFFTampl.max();
            }

            //multiply the magnitude of the input with the magnitude of the kernel,
            //effectivelly separating the desired frequency/azimuth
            spectral::array filteredAmplitude = spectral::hadamard( spectral::real( inputFFTpolar ),
                                                                    kernelSPaddedFFTamplUnitized );

            //make a new FFT field by combining the filtered amplitudes with the untouched phase
            //field of the input
            spectral::complex_array filteredFFTpolar = spectral::to_complex_array(
                                                          filteredAmplitude ,
                                                          spectral::imag( inputFFTpolar) );

            //convert the filtered FFT field to Cartesian form (real and imaginary parts)
            spectral::complex_array filteredFFT = spectral::to_rectangular_form( filteredFFTpolar );


            //performs inverse FFT to get the filtered result in spatial domain.
            spectral::array fundamentalFactor( nI, nJ, 1, 0.0 );
            spectral::backward( fundamentalFactor, filteredFFT );

            //fftw3 requires that the result be divided by the number of grid cells to get
            //the correct scale
            fundamentalFactor = fundamentalFactor / ( nI * nJ );

            ///Visualizing the results on the fly is optional/////////////
//            {
//                q3Dv.clearScene();
//                q3Dv.display( fundamentalFactor, fundamentalFactor.min(), fundamentalFactor.max() );
//                QApplication::processEvents(); //let Qt repaint GUI elements
//            }
            ////////////////////////////////////////////////////////////

            //discard factors averages values less than 1% of that of the original grid.
            double absAvg = std::abs( fundamentalFactor.avg() );
            if( absAvg >= absAvgInput/ui->txtThresholdGaborFactors->text().toDouble() )
                frequencyFactors.push_back( fundamentalFactor );

        }//for each azimuth
    }//for each frequency
}

void VariographicDecompositionDialog::doVariographicDecomposition2( )
{
    doVariographicDecomposition2( FundamentalFactorType::SVD_SINGULAR_FACTOR );
}
