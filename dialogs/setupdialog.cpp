#include "setupdialog.h"
#include "ui_setupdialog.h"
#include <QFileDialog>
#include "domain/application.h"
#include "util.h"

SetupDialog::SetupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SetupDialog)
{
    ui->setupUi(this);
    //read settings from OS registry.
    ui->txtGSLibPath->setText( Application::instance()->getGSLibPathSetting() );
    ui->txtGSPath->setText( Application::instance()->getGhostscriptPathSetting() );
    adjustSize();
}

SetupDialog::~SetupDialog()
{
    delete ui;
}

void SetupDialog::showGSLibPathSearch()
{
    QString dir = QFileDialog::getExistingDirectory(this, "select GSLib executables location", Util::getProgramInstallDir());
    if( ! dir.isEmpty() )
        ui->txtGSLibPath->setText( dir );
}

void SetupDialog::showGSPathSearch()
{
    QString dir = QFileDialog::getExistingDirectory(this, "select Ghostscript installation directory", Util::getProgramInstallDir());
    if( ! dir.isEmpty() )
        ui->txtGSPath->setText( dir );
}

void SetupDialog::accept()
{
    //save settings to OS registry.
    Application::instance()->setGSLibPathSetting( ui->txtGSLibPath->text() );
    Application::instance()->setGhostscriptPathSetting( ui->txtGSPath->text() );
    //make dialog close.
    this->reject();
}
