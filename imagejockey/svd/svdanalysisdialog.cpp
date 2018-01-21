#include "svdanalysisdialog.h"
#include "ui_svdanalysisdialog.h"
#include "domain/application.h"

SVDAnalysisDialog::SVDAnalysisDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SVDAnalysisDialog)
{
    ui->setupUi(this);

    setWindowTitle( "SVD Analysis" );

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);}

SVDAnalysisDialog::~SVDAnalysisDialog()
{
    Application::instance()->logInfo("SVDAnalysisDialog destroyed.");
    delete ui;
}

void SVDAnalysisDialog::setTree(SVDFactorTree &&tree)
{
    m_tree = std::move( tree );
}
