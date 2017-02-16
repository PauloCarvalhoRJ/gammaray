#include "intervalandcategorywidget.h"
#include "ui_intervalandcategorywidget.h"
#include "widgets/categoryselector.h"

IntervalAndCategoryWidget::IntervalAndCategoryWidget(CategoryDefinition *cd, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IntervalAndCategoryWidget)
{
    ui->setupUi(this);

    ui->frmCategorySelectorPlaceholder->layout()->addWidget( new CategorySelector( cd, this ) );
}

IntervalAndCategoryWidget::~IntervalAndCategoryWidget()
{
    delete ui;
}
