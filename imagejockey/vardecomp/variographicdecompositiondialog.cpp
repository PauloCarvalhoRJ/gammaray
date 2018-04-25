#include "variographicdecompositiondialog.h"
#include "ui_variographicdecompositiondialog.h"

VariographicDecompositionDialog::VariographicDecompositionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VariographicDecompositionDialog)
{
    ui->setupUi(this);

	//deletes dialog from memory upon user closing it
	this->setAttribute(Qt::WA_DeleteOnClose);

	setWindowTitle( "Variographic Decomposition" );
}

VariographicDecompositionDialog::~VariographicDecompositionDialog()
{
    delete ui;
}
