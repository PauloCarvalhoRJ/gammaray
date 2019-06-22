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
    m_gridViewerInput = new IJGridViewerWidget( false, false, false );
    m_gridViewerVarmap = new IJGridViewerWidget( false, false, false );
    ui->tabInputAndVarmap->layout()->addWidget( m_gridViewerInput );
    ui->tabInputAndVarmap->layout()->addWidget( m_gridViewerVarmap );

    // display input variable
    {
        CartesianGrid* cg = dynamic_cast<CartesianGrid*>( dataFile );
        assert( cg && "AutomaticVarFitDialog::AutomaticVarFitDialog(): only attributes from CartesianGrids can be used.");

        spectral::arrayPtr inputData( cg->createSpectralArray( m_at->getAttributeGEOEASgivenIndex()-1 ) ) ;
        SVDFactor* gridData = new SVDFactor( std::move(*inputData), 1, 0.42,
                                         cg->getOriginX(),
                                         cg->getOriginY(),
                                         cg->getOriginZ(),
                                         cg->getCellSizeI(),
                                         cg->getCellSizeJ(),
                                         cg->getCellSizeK(),
                                         0.0 );
        m_gridViewerInput->setFactor( gridData );
    }
}

AutomaticVarFitDialog::~AutomaticVarFitDialog()
{
    delete ui;
}
