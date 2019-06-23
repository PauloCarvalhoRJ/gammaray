#include "automaticvarfitdialog.h"
#include "ui_automaticvarfitdialog.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/cartesiangrid.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/svd/svdfactor.h"
#include "spectral/spectral.h"

#include <cassert>

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

    File* dataFile = at->getContainingFile();
    ui->lblFileAttribute->setText( dataFile->getName() + "/" + at->getName() );

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
