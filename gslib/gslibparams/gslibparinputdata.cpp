#include "gslibparinputdata.h"
#include <QListIterator>
#include <QTextStream>
#include <cmath>
#include "widgets/widgetgslibparinputdata.h"
#include "../../domain/attribute.h"
#include "../../domain/datafile.h"
#include "../gslibparams/gslibparvarweight.h"

GSLibParInputData::GSLibParInputData()
    : GSLibParType( "InputData", "Input data", "Input data" ),
      _file_with_data("DataFile", "Data file", "File with data"),
      _trimming_limits("TrimLimits", "Trimming limits", "Trimming limits")
{
}

GSLibParInputData::~GSLibParInputData()
{
    //TODO: deallocate objects in _var_wgt_pairs?
}

void GSLibParInputData::set( Attribute *at )
{
    DataFile* data_file = dynamic_cast<DataFile*>(at->getContainingFile());
    data_file->loadData();
    uint var_index = data_file->getFieldGEOEASIndex( at->getName() );
    double data_min = data_file->min( var_index-1 );
    double data_max = data_file->max( var_index-1 );
    _file_with_data._path = data_file->getPath();
    _var_wgt_pairs.first()->_var_index = var_index;
    _trimming_limits._min = data_min - fabs( data_min/100.0 );
    _trimming_limits._max = data_max + fabs( data_max/100.0 );
}

double GSLibParInputData::getLowerTrimmingLimit()
{
    return _trimming_limits._min;
}

double GSLibParInputData::getUpperTrimmingLimit()
{
    return  _trimming_limits._max;
}


void GSLibParInputData::save(QTextStream *out)
{
    _file_with_data.save( out );

    //first comes the variable indexes
    QListIterator<GSLibParVarWeight*> it(_var_wgt_pairs);
    while( it.hasNext() )
        *out << "  " << it.next()->_var_index;
    //then the weight indexes
    it = QListIterator<GSLibParVarWeight*>(_var_wgt_pairs);
    while( it.hasNext() )
        *out << "  " << it.next()->_wgt_index;
    *out << '\n';

    _trimming_limits.save( out );
}

QWidget *GSLibParInputData::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParInputData();
    ((WidgetGSLibParInputData*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParInputData::update()
{
    ((WidgetGSLibParInputData*)this->_widget)->updateValue( this );
    return true;
}
