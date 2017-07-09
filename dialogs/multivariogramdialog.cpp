#include "multivariogramdialog.h"
#include "ui_multivariogramdialog.h"

#include "domain/application.h"
#include "domain/attribute.h"
#include "domain/file.h"

MultiVariogramDialog::MultiVariogramDialog(const std::vector<Attribute *> attributes,
                                           QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MultiVariogramDialog),
    _attributes( attributes )
{
    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);

    setWindowTitle("Multiple variogram plot");

    //formats a check list for the user
    QString html = "<html><head/><body><p><span style=\" color:#0000ff;\"><table>";
    html += "<tr><td><b>File:</b></td><td>&nbsp;&nbsp;&nbsp;&nbsp;</td><td><b>Variable:</b></td></tr>";
    std::vector<Attribute *>::iterator it = _attributes.begin();
    for(; it != _attributes.end(); ++it){
        Attribute *at = *it;
        File *file = at->getContainingFile();
        html += "<tr><td>" + file->getName() + "</td><td>&nbsp;&nbsp;&nbsp;&nbsp;</td><td>"
                + at->getName() + "</td></tr>";
    }
    html += "</table></span></p></body></html>";

    ui->lblCheckList->setText( html );

}

MultiVariogramDialog::~MultiVariogramDialog()
{
    Application::instance()->logInfo("MultiVariogramDialog destroyed.");
    delete ui;
}

void MultiVariogramDialog::onGam()
{

}
