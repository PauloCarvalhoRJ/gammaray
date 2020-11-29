#include "sectiondialog.h"
#include "ui_sectiondialog.h"

#include <QFileDialog>
#include <QMessageBox>

#include "util.h"
#include "domain/section.h"
#include "domain/application.h"
#include "domain/project.h"
#include "domain/pointset.h"
#include "domain/cartesiangrid.h"

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
    QString name = ui->txtName->text();

    //sanity checks
    {
        if( name.isEmpty() ){
            QMessageBox::critical( this, "Error",
                                   QString("Name cannot be empty."));
            return;
        }
        QFile file( ui->txtPathToPointSet->text() );
        if( ! file.exists() ){
            QMessageBox::critical( this, "Error",
                                   QString("Point set file with the section path not found or not set."));
            return;
        }
        file.setFileName( ui->txtPathToCartesainGrid->text() );
        if( ! file.exists() ){
            QMessageBox::critical( this, "Error",
                                   QString("Cartesian grid file with the section data not found or not set."));
            return;
        }
    }

    //Create the section object.
    Section* section = new Section( Application::instance()->getProject()->getPath() + '/' +
                                    name );

    //work on the point set with the geologic section areal path.
    {
        //Copy the point set file to the project directory.
        QString ps_path = Util::copyFileToDir( ui->txtPathToPointSet->text(),
                                               Application::instance()->getProject()->getPath() );

        //Create the point set object.
        PointSet* ps = new PointSet( ps_path );
        ps->setInfo( 1, 2, 0, "" ); // this assumes the file format presented in the
                                    // dialog's UI and in Section class' documentation.

        //Adds the point set as a child of the Section object.
        section->setPointSet( ps );
    }

    //work on the Cartesian grid with the geologic section data.
    {
        //Copy the Cartesian grid file to the project directory.
        QString cg_path = Util::copyFileToDir( ui->txtPathToCartesainGrid->text(),
                                               Application::instance()->getProject()->getPath() );

        //Create the Cartesian grid object.
        CartesianGrid* cg = new CartesianGrid( cg_path );
        cg->setInfo( 0.0, 0.0, 0.0, //these parameters are not relevant for use in a section
                     1.0, 1.0, 1.0, //these parameters are not relevant for use in a section
                     ui->spinNI->value(), 1, ui->spinNK->value(), //nJ is always 1
                     0.0, 1,        //these parameters are not relevant for use in a section
                     "",
                     QMap<uint, QPair<uint, QString> >(),
                     QList<QPair<uint, QString> >() );

        //Adds the Cartesian grid as a child of the Section object.
        section->setCartesianGrid( cg );
    }

    //Adds the section to the project tree.
    Application::instance()->getProject()->addSection( section );
    Application::instance()->refreshProjectTree();

    //Close the dialog.
    reject();
}
