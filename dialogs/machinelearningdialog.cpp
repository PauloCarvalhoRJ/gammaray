#include "machinelearningdialog.h"
#include "ui_machinelearningdialog.h"
#include "widgets/fileselectorwidget.h"

MachineLearningDialog::MachineLearningDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MachineLearningDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    FileSelectorWidget* m_trainingFileSelector = new FileSelectorWidget( FileSelectorType::DataFiles );
    ui->frmTrainingFileSelectorPlaceholder->layout()->addWidget( m_trainingFileSelector );
}

MachineLearningDialog::~MachineLearningDialog()
{
    delete ui;
}
