#include "widgetgslibparinputdata.h"
#include "ui_widgetgslibparinputdata.h"
#include "gslibparamwidgets.h"
#include "../gslibparinputdata.h"
#include <QPainter>

WidgetGSLibParInputData::WidgetGSLibParInputData(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParInputData),
    _wfile( nullptr ),
    _wmvv( nullptr ),
    _wldbl( nullptr )
{
    ui->setupUi(this);
}

WidgetGSLibParInputData::~WidgetGSLibParInputData()
{
    delete ui;
    //we delete _wmvv because it is not managed by a GSLibPar*, since
    //GSLibParLimitsDouble::_var_wgt_pairs is a QList
    delete _wmvv;
}

void WidgetGSLibParInputData::fillFields(GSLibParInputData *param)
{
    ui->lblDesc->setText( param->getDescription() );

    if(!_wfile){
        _wfile = (WidgetGSLibParFile*)param->_file_with_data.getWidget();
        ui->frmFilePart->layout()->addWidget( _wfile );
    }
    if(!_wmvv){
        // param->_var_wgt_pairs is not a GSLibPar*, it is a QList, thus we create a new WidgetGSLib*
        _wmvv = new WidgetGSLibParMultiValuedVariable("int");
        ui->frmVarWgtPairsPart->layout()->addWidget( _wmvv );
    }
    if(!_wldbl){
        _wldbl = (WidgetGSLibParLimitsDouble*)param->_trimming_limits.getWidget();
        ui->frmTrimmingLimitsPart->layout()->addWidget( _wldbl );
    }

    _wfile->fillFields( &(param->_file_with_data) );
    _wmvv->fillFields( &(param->_var_wgt_pairs) );
    _wldbl->fillFields( &(param->_trimming_limits) );
}

void WidgetGSLibParInputData::updateValue(GSLibParInputData *param)
{
    _wfile->updateValue( &(param->_file_with_data) );
    _wmvv->updateValue( &(param->_var_wgt_pairs) );
    _wldbl->updateValue( &(param->_trimming_limits) );
}

void WidgetGSLibParInputData::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
