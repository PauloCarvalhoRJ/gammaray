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

#include <QMessageBox>
#include <QProgressDialog>

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

	if( ! grid && ! variable ){
		QMessageBox::critical( this, "Error", "Please, select a grid and a variable.");
		return;
	}

	// Get the grid's dimensions.
	unsigned int nI = grid->getNI();
	unsigned int nJ = grid->getNJ();
	unsigned int nK = grid->getNK();

	// Fetch data from the data source.
	grid->dataWillBeRequested();

	// The user-given number of geological factors (m).
	int m = ui->spinNumberOfGeologicalFactors->value();

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
			for(unsigned int k = 0; k < nK; ++k) {
				QCoreApplication::processEvents(); //let Qt repaint widgets
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
				spectral::SVD svd = spectral::svd( gridVarmap );
				//get the list with the factor weights (information quantity)
				spectral::array weights = svd.factor_weights();
				progressDialog.hide();
				//get the number of fundamental factors that have 100% of information content.
				{
					double cumulative = 0.0;
					uint i = 0;
					for(; i < weights.size(); ++i){
						cumulative += weights.d_[i];
						if( cumulative > 0.99999 )
							break;
					}
					n = i+1;
				}
				if( n < 3 ){
					QMessageBox::warning( this, "Warning", "The data's varmap must be decomposable in at least three usable fundamental factors to proceed.");
					return;
				}
				//Get the usable fundamental SVD factors.
				{
					QProgressDialog progressDialog;
					progressDialog.setRange(0,0);
					progressDialog.show();
					for (long i = 0; i < n; ++i) {
                        progressDialog.setLabelText("Retrieving fundamental SVD factor " + QString::number(i+1) +
                                                    " of " + QString::number(n) + "...");
						QCoreApplication::processEvents();
						spectral::array factor = svd.factor(i);
						svdFactors.push_back( std::move( factor ) );
					}
				}
			}
		}
	}

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

	//Make a full Sigma matrix (to be compatible with multiplication with the othe matrices)
	{
		spectral::array SigmaTmp( (spectral::index)n, (spectral::index)m*n );
		for( int i = 0; i < n; ++i)
			SigmaTmp(i, i) = Sigma.d_[i];
		Sigma = SigmaTmp;
	}

	//Make U*
	//U is square, symmetrical and contains only real number, thus U's transpose conjugate is itself.
	spectral::array Ustar( U );

	//Make Sigmadagger (pseudoinverse of Sigma)
	spectral::array SigmaDagger( Sigma );
	{
		//compute reciprocals of the non-zero elements in the main diagonal.
		for( int i = 0; i < n; ++i){
			double value = SigmaDagger(i, i);
			if( ! ImageJockeyUtils::almostEqual2sComplement( value, 0.0, 1 ) )
				SigmaDagger( i, i ) = 1.0 / value;
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

	//-------------------------OPTIMIZATION LOOP BEGINS HERE-------------------------------------------
    while(true){
        //Compute the vector of weights [a] = Adagger.B + (I-Adagger.A)[w]
        spectral::array va;
        {
            Eigen::MatrixXd eigenAdagger = spectral::to_2d( Adagger );
            Eigen::MatrixXd eigenB = spectral::to_2d( B );
            Eigen::MatrixXd eigenI = spectral::to_2d( I );
            Eigen::MatrixXd eigenA = spectral::to_2d( A );
            Eigen::MatrixXd eigenvw = spectral::to_2d( vw );
            Eigen::MatrixXd eigenva = eigenAdagger * eigenB + ( eigenI - eigenAdagger * eigenA ) * eigenvw;
            va = spectral::to_array( eigenva );
        }

        //Make the m geological factors (expected variographic structures)
        std::vector< spectral::array > geologicalFactors;
        std::vector< std::string > titles;
        {
            QProgressDialog progressDialog;
            progressDialog.setRange(0,0);
            progressDialog.show();
            progressDialog.setLabelText("Making the geological factors...");
            for( int iGeoFactor = 0; iGeoFactor < m; ++iGeoFactor){
                spectral::array geologicalFactor( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
                for( int iSVDFactor = 0; iSVDFactor < n; ++iSVDFactor){
                    QCoreApplication::processEvents();
                    geologicalFactor += svdFactors[iSVDFactor] * va.d_[ iGeoFactor * m + iSVDFactor ];
                }
                QString title = QString("Geological factor #") + QString::number(iGeoFactor+1);
                titles.push_back( title.toStdString() );
                geologicalFactors.push_back( std::move( geologicalFactor ) );
            }
        }

        //Display the geological factors
        displayGrids( geologicalFactors, titles );

        //Compute the grid derived form the geological factors (ideally it must match the input grid)
        spectral::array derivedGrid( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
        std::vector< spectral::array >::iterator it = geologicalFactors.begin();
        for( ; it != geologicalFactors.end(); ++it ){
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
            //Update the derived grid.
            derivedGrid += rfftResult;
        }

        //Display the derived grid and its difference with respect to the original grid.
        {
            spectral::array *gridData = grid->createSpectralArray( variable->getIndexInParentGrid() );
            spectral::array diff = *gridData - derivedGrid;
            displayGrids( {*gridData, derivedGrid, diff}, {"Original grid", "Derived grid", "difference"}, false );
            delete gridData;
        }

        break;
    } //----------------------END OF PARAMETER OPTIMIZATION LOOP-----------------
}

void VariographicDecompositionDialog::displayGrids(const std::vector<spectral::array> &grids,
                                                   const std::vector<std::__cxx11::string> &titles,
                                                   bool doShiftByHalf)
{
    //Create the structure to store the geological factors
    SVDFactorTree * factorTree = new SVDFactorTree( 0.0 ); //the split factor of 0.0 has no special meaning here
    //Populate the factor container with the geological factors.
    std::vector< spectral::array >::const_iterator it = grids.begin();
    std::vector< std::string >::const_iterator itTitles = titles.begin();
    for(int i = 1; it != grids.end(); ++it, ++i, ++itTitles){
        //make a local copy of the geological factor data
        spectral::array geoFactorDataCopy;
        if( doShiftByHalf )
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
    emit saveArray( gridData );
}
