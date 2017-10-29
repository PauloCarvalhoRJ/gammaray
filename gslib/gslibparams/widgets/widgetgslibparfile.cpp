#include "widgetgslibparfile.h"
#include "ui_widgetgslibparfile.h"
#include <QFileDialog>
#include "../gslibparfile.h"
#include "domain/application.h"
#include "domain/project.h"

WidgetGSLibParFile::WidgetGSLibParFile(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParFile)
{
    ui->setupUi(this);

    connect(ui->btnFile, SIGNAL(clicked(bool)), this, SLOT(onBtnFileClicked(bool)));
}

WidgetGSLibParFile::~WidgetGSLibParFile()
{
    delete ui;
}

void WidgetGSLibParFile::fillFields(GSLibParFile *param)
{
    ui->lblDesc->setText( param->getDescription() );
    ui->txtPath->setText( param->_path );
}

void WidgetGSLibParFile::updateValue(GSLibParFile *param)
{
    if( ui->txtPath->text().indexOf('-') >=0  ){
        Application::instance()->logError("WidgetGSLibParFile::updateValue(): File paths containing hyphens conflict with GSLib parameter file syntax.");
        Application::instance()->logError("     Parameter " + param->getName() + " value reset to empty.");
        param->_path = "";
        ui->txtPath->setText("");
    } else {
        param->_path = ui->txtPath->text();
    }
}

void WidgetGSLibParFile::onBtnFileClicked(bool)
{
    QString file = QFileDialog::getOpenFileName(this, "select file", Application::instance()->getProject()->getPath());
    if( ! file.isEmpty() ){
        if( file.indexOf('-') >=0  ){
            Application::instance()->logError("WidgetGSLibParFile::onBtnFileClicked(): File paths containing hyphens conflict with GSLib parameter file syntax.");
            Application::instance()->logError("     Field value reset to empty.");
            ui->txtPath->setText("");
        } else {
            ui->txtPath->setText( file );
        }
    }
}
