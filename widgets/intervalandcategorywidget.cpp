#include "intervalandcategorywidget.h"
#include "ui_intervalandcategorywidget.h"
#include "widgets/categoryselector.h"

IntervalAndCategoryWidget::IntervalAndCategoryWidget(CategoryDefinition *cd, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IntervalAndCategoryWidget),
    m_categorySelector(new CategorySelector( cd, this ) )
{
    ui->setupUi(this);

    ui->frmCategorySelectorPlaceholder->layout()->addWidget( m_categorySelector );
}

IntervalAndCategoryWidget::~IntervalAndCategoryWidget()
{
    delete ui;
}

double IntervalAndCategoryWidget::getIntervalLow()
{
    return ui->txtIntervalStart->text().toDouble();
}

double IntervalAndCategoryWidget::getIntervalHigh()
{
    return ui->txtIntervalEnd->text().toDouble();
}

uint IntervalAndCategoryWidget::getCategoryCode()
{
    return m_categorySelector->getSelectedCategoryCode();
}

void IntervalAndCategoryWidget::setIntervalLow(double value)
{
    ui->txtIntervalStart->setText( QString::number( value ) );
}

void IntervalAndCategoryWidget::setIntervalHigh(double value)
{
    ui->txtIntervalEnd->setText( QString::number( value ) );
}

void IntervalAndCategoryWidget::setCategoryCode(uint value)
{
    m_categorySelector->setSelectedCategoryCode( value );
}
