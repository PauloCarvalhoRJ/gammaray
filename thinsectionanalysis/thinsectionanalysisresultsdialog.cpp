#include "thinsectionanalysisresultsdialog.h"
#include "ui_thinsectionanalysisresultsdialog.h"

ThinSectionAnalysisResultsDialog::ThinSectionAnalysisResultsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ThinSectionAnalysisResultsDialog)
{
    ui->setupUi(this);
}

ThinSectionAnalysisResultsDialog::~ThinSectionAnalysisResultsDialog()
{
    delete ui;
}
