#include "realizationselectiondialog.h"
#include "ui_realizationselectiondialog.h"

RealizationSelectionDialog::RealizationSelectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RealizationSelectionDialog)
{
    ui->setupUi(this);
}

RealizationSelectionDialog::~RealizationSelectionDialog()
{
    delete ui;
}

void RealizationSelectionDialog::setNReals(int count)
{
    for( int i = 1; i <= count; ++i){
        ui->listReals->addItem( QString::number(i) );
    }
    ui->listReals->selectAll();
}

std::vector<int> RealizationSelectionDialog::getSelectedRealizations()
{
    std::vector<int> result;

    QList<QListWidgetItem*> selectedItems = ui->listReals->selectedItems();
    QList<QListWidgetItem*>::iterator itSel = selectedItems.begin();
    for( ; itSel != selectedItems.end(); ++itSel){
        result.push_back( (*itSel)->text().toInt() );
    }

    return result;
}
