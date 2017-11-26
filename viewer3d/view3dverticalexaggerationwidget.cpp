#include "view3dverticalexaggerationwidget.h"
#include "ui_view3dverticalexaggerationwidget.h"
#include "widgets/focuswatcher.h"

View3DVerticalExaggerationWidget::View3DVerticalExaggerationWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::View3DVerticalExaggerationWidget)
{
    ui->setupUi(this);

    //monitor focus events without subclassing the widget
    connect(new FocusWatcher(ui->dblSpinVerticalExaggeration), &FocusWatcher::focusChanged,
            this, &View3DVerticalExaggerationWidget::focusChanged);

    //signal-signal connection
    connect( ui->dblSpinVerticalExaggeration, SIGNAL(valueChanged(double)),
             this, SIGNAL(valueChanged(double)));
}

View3DVerticalExaggerationWidget::~View3DVerticalExaggerationWidget()
{
    delete ui;
}

double View3DVerticalExaggerationWidget::getVerticalExaggeration()
{
    return ui->dblSpinVerticalExaggeration->value();
}

void View3DVerticalExaggerationWidget::setFocus()
{
    QWidget::setFocus();
    ui->dblSpinVerticalExaggeration->setFocus();
}

void View3DVerticalExaggerationWidget::focusChanged(bool in)
{
    if( ! in )
        this->hide();
}
