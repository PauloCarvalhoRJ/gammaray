#include "automaticvarfitdialog.h"
#include "ui_automaticvarfitdialog.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/cartesiangrid.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/svd/svdfactor.h"
#include "spectral/spectral.h"

#include <cassert>
#include <thread>

AutomaticVarFitDialog::AutomaticVarFitDialog(Attribute *at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutomaticVarFitDialog),
    m_at( at )
{
    assert( at && "AutomaticVarFitDialog::AutomaticVarFitDialog(): attribute cannot be null.");

    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Automatic variogram model fitting" );

    //display the selected input data names
    File* dataFile = at->getContainingFile();
    ui->lblFileAttribute->setText( dataFile->getName() + "/" + at->getName() );

    //get the number of threads from logical CPUs
    ui->spinNumberOfThreads->setValue( (int)std::thread::hardware_concurrency() );

    // these pointers will be managed by Qt
    m_gridViewerInput = new IJGridViewerWidget( true, false, false );
    m_gridViewerVarmap = new IJGridViewerWidget( true, false, false );
    ui->tabInputAndVarmap->layout()->addWidget( m_gridViewerInput );
    ui->tabInputAndVarmap->layout()->addWidget( m_gridViewerVarmap );

    // display input variable
    {
        CartesianGrid* cg = dynamic_cast<CartesianGrid*>( dataFile );
        assert( cg && "AutomaticVarFitDialog::AutomaticVarFitDialog(): only attributes from CartesianGrids can be used.");
        //Get input data as a raw data array
        spectral::arrayPtr inputData( cg->createSpectralArray( m_at->getAttributeGEOEASgivenIndex()-1 ) );
        //make a local copy (will be moved to inside of a SVDFacor object)
        spectral::array temp( *inputData );
        //make a SVDFactor object so we can reuse IJGridViewerWidget to display gridded data
        SVDFactor* gridData = new SVDFactor( std::move(temp), 1, 0.42,
                                         cg->getOriginX(),
                                         cg->getOriginY(),
                                         cg->getOriginZ(),
                                         cg->getCellSizeI(),
                                         cg->getCellSizeJ(),
                                         cg->getCellSizeK(),
                                         0.0 );
        //set color scale label
        gridData->setCustomName( m_at->getName() );
        //display data
        m_gridViewerInput->setFactor( gridData );
    }

    // display input variable's varmap
    {
        CartesianGrid* cg = dynamic_cast<CartesianGrid*>( dataFile );
        assert( cg && "AutomaticVarFitDialog::AutomaticVarFitDialog(): only attributes from CartesianGrids can be used.");
        //Get input data as a raw data array
        spectral::arrayPtr inputData( cg->createSpectralArray( m_at->getAttributeGEOEASgivenIndex()-1 ) ) ;
        //make a local copy (will be moved to inside of a SVDFacor object)
        spectral::array temp( *inputData );
        //compute varmap (output will go to temp)
        spectral::autocovariance( temp , *inputData, true );
        //put covariance at h=0 in the center of the grid for ease of interpretation
        temp = spectral::shiftByHalf( temp );
        //clips the varmap so the grid matches the input's
        temp = spectral::project( temp, cg->getNI(), cg->getNJ(), cg->getNK() );
        //invert result so the value increases radially from the center at h=0
        temp = temp.max() - temp;
        //make a SVDFactor object so we can reuse IJGridViewerWidget to display gridded data
        SVDFactor* gridData = new SVDFactor( std::move(temp), 1, 0.42,
                                         -cg->getNI() / 2 * cg->getCellSizeI(),
                                         -cg->getNJ() / 2 * cg->getCellSizeJ(),
                                         -cg->getNK() / 2 * cg->getCellSizeK(),
                                         cg->getCellSizeI(),
                                         cg->getCellSizeJ(),
                                         cg->getCellSizeK(),
                                         0.0 );
        //set color scale label
        gridData->setCustomName( "Varmap" );
        //display data
        m_gridViewerVarmap->setFactor( gridData );
    }
}

AutomaticVarFitDialog::~AutomaticVarFitDialog()
{
    delete ui;
}

void AutomaticVarFitDialog::onDoWithSAandGD()
{
    /////-----------------GET USER CONFIGURATIONS-----------------------------------------------------------
    //.......................Global Parameters................................................
    // Number of parallel execution threads
    unsigned int nThreads = ui->spinNumberOfThreads->value();
    // Number of variogram structures to fit.
    int m = ui->spinNumberOfGeologicalFactors->value();
    // Intialize the random number generator with the same seed
    std::srand ((unsigned)ui->spinSeed->value());
    //...................................Annealing Parameters.................................
    // Intial temperature.
    double f_Tinitial = ui->spinInitialTemperature->value();
    // Final temperature.
    double f_Tfinal = ui->spinFinalTemperature->value();
    // Max number of SA steps.
    int i_kmax = ui->spinMaxStepsSA->value();
    /*Factor used to control the size of the random state “hop”.  For example, if the maximum “hop” must be
     10% of domain size, set 0.1.  Small values (e.g. 0.001) result in slow, but more accurate convergence.
     Large values (e.g. 100.0) covers more space faster, but falls outside the domain are more frequent,
     resulting in more re-searches due to more invalid parameter value penalties. */
    double f_factorSearch = ui->spinMaxHopFactor->value();
    //........................Gradient Descent Parameters.....................................
    // Max number of GD steps
    int maxNumberOfOptimizationSteps = ui->spinMaxSteps->value();
    // User-given epsilon (useful for numerical calculus).
    double epsilon = std::pow(10, ui->spinLogEpsilon->value() );
    // Alpha is the factor by which the gradient value is multiplied
    // a small value prevents overshooting.
    double initialAlpha = ui->spinInitialAlpha->value();
    // Alpha is reduced iteratively until a descent is detected (no overshoot)
    double maxNumberOfAlphaReductionSteps = ui->spinMaxStepsAlphaReduction->value();
    // GD stops after two consecutive steps yield two objective function values whose
    // difference is less than this value.
    double convergenceCriterion = std::pow(10, ui->spinConvergenceCriterion->value() );
    /////----------------END OF USER CONFIGURATIONS---------------------------------------------------------

}

void AutomaticVarFitDialog::onDoWithLSRS()
{

}

void AutomaticVarFitDialog::onDoWithPSO()
{

}

void AutomaticVarFitDialog::onDoWithGenetic()
{

}
