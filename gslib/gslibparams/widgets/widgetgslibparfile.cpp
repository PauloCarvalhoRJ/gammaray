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
    param->_path = ui->txtPath->text();
}

void WidgetGSLibParFile::onBtnFileClicked(bool)
{
    QString file = QFileDialog::getOpenFileName(this, "select file", Application::instance()->getProject()->getPath());
    if( ! file.isEmpty() ){
        ui->txtPath->setText( file );
    }
}
