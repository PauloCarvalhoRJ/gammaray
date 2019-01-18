#include "choosecategorydialog.h"
#include "ui_choosecategorydialog.h"
#include "widgets/categoryselector.h"

uint ChooseCategoryDialog::getSelectedCategoryCode()
{
    return m_cdSelector->getSelectedCategoryCode();
}

ChooseCategoryDialog::ChooseCategoryDialog(CategoryDefinition *cd, const QString title, const QString caption, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChooseCategoryDialog),
    m_cd(cd)
{
    ui->setupUi(this);
    m_cdSelector = new CategorySelector( m_cd );
    ui->layoutForCategorySelector->addWidget( m_cdSelector );
    ui->lblCaption->setText( caption );
    setWindowTitle( title );
}

ChooseCategoryDialog::~ChooseCategoryDialog()
{
    delete ui;
}
