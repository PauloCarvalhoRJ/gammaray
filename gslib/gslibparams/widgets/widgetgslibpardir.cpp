#include "widgetgslibpardir.h"
#include "ui_widgetgslibpardir.h"

#include <QFileDialog>
#include "../gslibpardir.h"
#include "domain/application.h"
#include "domain/project.h"


WidgetGSLibParDir::WidgetGSLibParDir(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParDir)
{
    ui->setupUi(this);

    //this signal-slot connection is defined in the .ui file (done in Qt Designer)
//     connect(ui->btnFile, SIGNAL(clicked(bool)), this, SLOT(onBtnFileClicked(bool)));
}

WidgetGSLibParDir::~WidgetGSLibParDir()
{
    delete ui;
}

void WidgetGSLibParDir::fillFields(GSLibParDir *param)
{
    ui->lblDesc->setText( param->getDescription() );
    ui->txtPath->setText( param->_path );
}

void WidgetGSLibParDir::updateValue(GSLibParDir *param)
{
    if( ui->txtPath->text().indexOf('-') >=0  ){
        Application::instance()->logError("WidgetGSLibParDir::updateValue(): File paths containing hyphens conflict"
                                          " with GSLib parameter file syntax.");
        Application::instance()->logError("     Parameter " + param->getName() + " value reset to empty.");
        param->_path = "";
        ui->txtPath->setText("");
    } else {
        param->_path = ui->txtPath->text();
    }
}

void WidgetGSLibParDir::onBtnFileClicked(bool)
{
    QString file = QFileDialog::getExistingDirectory(this, "select directory",
                                                     Application::instance()->getProject()->getPath());
    if( ! file.isEmpty() ){
        if( file.indexOf('-') >=0  ){
            Application::instance()->logError("WidgetGSLibParDir::onBtnFileClicked(): file system aths containing "
                                              "hyphens conflict with GSLib parameter file syntax.");
            Application::instance()->logError("     Field value reset to empty.");
            ui->txtPath->setText("");
        } else {
            ui->txtPath->setText( file );
        }
    }
}
