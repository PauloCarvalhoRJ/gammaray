#include "lvadatasetdialog.h"
#include "ui_lvadatasetdialog.h"

#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/cartesiangrid.h"
#include "domain/application.h"

LVADataSetDialog::LVADataSetDialog(Attribute* at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LVADataSetDialog),
    m_at( at )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle( "Locally Varying Anisotropy Dialog" );

    ui->lblCaption->setText( "Locally varying anisotropy for [" + m_at->getContainingFile()->getName() + " / " + m_at->getName() + "]" );

    ui->lblGridName->setText( m_at->getContainingFile()->getName() );
}

LVADataSetDialog::~LVADataSetDialog()
{
    delete ui;
}

void LVADataSetDialog::updateSummary()
{
    QString summary;
    CartesianGrid* cg = dynamic_cast<CartesianGrid*>( m_at->getContainingFile() );
    if( ! cg ){
        std::stringstream ss;
        ss << "Grid is " << cg->getNX() << " x " << cg->getNY() << " x " << cg->getNZ();
        summary = QString( ss.str().c_str() );
    }
    else
        summary = "Input file is not a Cartesian grid.";

    ui->txtEdWindowingSummary->clear();
    ui->txtEdWindowingSummary->appendPlainText( summary );
}

void LVADataSetDialog::computeLVA2D()
{
//    //Get the z-slices
//    std::vector< spectral::array > geologicalFactors;
//    {
//        for( int k = 0; k < nK; ++k){
//            spectral::array geologicalFactor( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
//            for( int iSVDFactor = 0; iSVDFactor < n; ++iSVDFactor){
//                geologicalFactor += fundamentalFactors[iSVDFactor] * va.d_[ iGeoFactor * m + iSVDFactor ];
//            }
//            geologicalFactors.push_back( std::move( geologicalFactor ) );
//        }
//    }

//    //Get the Fourier transform of the Z-slice
//    std::vector< spectral::complex_array > geologicalFactorsFTs;
//    {
//        std::vector< spectral::array >::iterator it = geologicalFactors.begin();
//        for( ; it != geologicalFactors.end(); ++it ){
//            spectral::array& geologicalFactor = *it;
//            spectral::complex_array tmp;
//            lck.lock();                                //
//            spectral::foward( tmp, geologicalFactor ); //fftw crashes when called simultaneously
//            lck.unlock();                              //
//            geologicalFactorsFTs.push_back( std::move( tmp ));
//        }
//    }

}

void LVADataSetDialog::onComputeLVA()
{
    if( ui->radio2D->isChecked() )
        return computeLVA2D();
    Application::instance()->logError("LVADataSetDialog::onComputeLVA(): unsupported LVA computation mode. Please, contact the program developers, this is likely a bug.");
}
