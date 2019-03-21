#include "dynamicfaciesrelationshipdiagramdialog.h"
#include "ui_dynamicfaciesrelationshipdiagramdialog.h"

DynamicFaciesRelationshipDiagramDialog::DynamicFaciesRelationshipDiagramDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DynamicFaciesRelationshipDiagramDialog)
{
    ui->setupUi(this);
}

DynamicFaciesRelationshipDiagramDialog::~DynamicFaciesRelationshipDiagramDialog()
{
    delete ui;
}
