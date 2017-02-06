#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    this->setWindowTitle(QString("About ").append(APP_NAME_VER));
    ui->lblAbout->setText( QString(APP_NAME_VER).\
         append(" is a graphical user interface to \
GSLib geostatistics software.\n\nGSLib by Centre for Computational Geostatistics of \
University of Alberta.\n\n").\
         append(APP_NAME).append(" uses GhostScript to parse GSLib PostScript plots. \
GhostScript by Artifex Software Inc.\n\n").\
         append(APP_NAME).append(" by Paulo R. M. Carvalho (paulo.r.m.carvalho@gmail.com) and the code contributors (see LICENSE.md file)\n\n").\
         append(APP_NAME).append(" is licensed under the Creative Commons BY-SA license v3.0."));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
