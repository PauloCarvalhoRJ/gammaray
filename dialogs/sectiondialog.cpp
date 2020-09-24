#include "sectiondialog.h"
#include "ui_sectiondialog.h"

#include <QFileDialog>

#include "util.h"

SectionDialog::SectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SectionDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Define a geologic section." );
}

SectionDialog::~SectionDialog()
{
    delete ui;
}

void SectionDialog::onChoosePointSet()
{
    QString path = QFileDialog::getOpenFileName(this, "Choose point set file:",
                                                Util::getLastBrowsedDirectory());
    if( path.isEmpty() )
        return;

    ui->txtPathToPointSet->setText( path );


    Util::saveLastBrowsedDirectoryOfFile( path );
}

void SectionDialog::onChooseCartesianGrid()
{
    QString path = QFileDialog::getOpenFileName(this, "Choose Cartesian grid file:",
                                                Util::getLastBrowsedDirectory());
    if( path.isEmpty() )
        return;

    ui->txtPathToCartesainGrid->setText( path );


    Util::saveLastBrowsedDirectoryOfFile( path );
}

void SectionDialog::onCreate()
{

}
