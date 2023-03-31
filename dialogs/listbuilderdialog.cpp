#include "listbuilderdialog.h"
#include "ui_listbuilderdialog.h"

#include "widgets/listbuilder.h"

ListBuilderDialog::ListBuilderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ListBuilderDialog)
{
    ui->setupUi(this);

    m_listBuilder = new ListBuilder(this); //this will be deallocated by Qt
    ui->frmForListBuilder->layout()->addWidget( m_listBuilder );
}

ListBuilderDialog::~ListBuilderDialog()
{
    delete ui;
}
