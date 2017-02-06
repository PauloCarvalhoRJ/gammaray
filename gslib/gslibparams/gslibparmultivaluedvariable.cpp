#include "gslibparmultivaluedvariable.h"
#include <QTextStream>
#include "../../domain/application.h"
#include "widgets/widgetgslibparmultivaluedvariable.h"
#include "gslibpardouble.h"
#include "gslibparuint.h"

GSLibParMultiValuedVariable::GSLibParMultiValuedVariable(GSLibParType *original_parameter)
    :    GSLibParType("", "", "")
{
    _parameters.append( original_parameter );
}

QString GSLibParMultiValuedVariable::getAllowedParameterTypeName()
{
    if( this->_parameters.size() < 1 ){
        Application::instance()->logError("GSLibParMultiValuedVariable::getAllowedParameterTypeName(): at least one GSLipParType object must exist.");
        return "";
    }
    return _parameters.first()->getTypeName();
}

template<typename T>
void GSLibParMultiValuedVariable::assureLoop(uint n)
{
    for(int i = _parameters.size(); i < n; ++i){
        _parameters.append( new T("","","") );
    }
}

void GSLibParMultiValuedVariable::assure(uint n)
{
    if( _parameters.size() >= n)
        return;
    if( getAllowedParameterTypeName() == "double" ){
        assureLoop<GSLibParDouble>( n );
    } else if( getAllowedParameterTypeName() == "uint" ){
        assureLoop<GSLibParUInt>( n );
    } else {
        Application::instance()->logError("GSLibParMultiValuedVariable::assure(): parameter type not supported:" + getAllowedParameterTypeName());
    }
}

void GSLibParMultiValuedVariable::save(QTextStream *out)
{
    QString buffer;
    //use a stream operating on a string to get and modify the values (e.g. remove line breaks)
    //of the component parameters before commiting them to the file stream.
    QTextStream local_str_stream( &buffer );
    for(QList<GSLibParType*>::iterator it = _parameters.begin(); it != _parameters.end(); ++it) {
        GSLibParType *parameter = (*it);
        parameter->save( &local_str_stream );
    }
    //replace end line characters with spaces
    buffer.replace('\n', ' ');
    buffer.replace('\r', ' ');
    //commit the values list to the file as single line of text
    *out << buffer << '\n';
}

QWidget *GSLibParMultiValuedVariable::getWidget()
{
    //the GSLibPar* type in this collection is determined by the original parameter (first object in _parameters)
    if( this->_parameters.size() == 0 )
        Application::instance()->logError("GSLibParMultiValuedVariable::getWidget(): _parameters must have at least one object.");
    if( ! this->_widget  )
       this->_widget = new WidgetGSLibParMultiValuedVariable( this->_parameters.at(0)->getTypeName() );
    ((WidgetGSLibParMultiValuedVariable*)this->_widget)->fillFields( this );
    return this->_widget;
}

GSLibParMultiValuedVariable *GSLibParMultiValuedVariable::clone()
{
    GSLibParMultiValuedVariable* clone = nullptr;
    //copy the original parameter
    GSLibParType* orig_par = _parameters.at(0)->clone();
    if( orig_par ){
        //copy the object itself
        clone = new GSLibParMultiValuedVariable( orig_par );
        //copy the replicates, if any.
        QList<GSLibParType*>::iterator it = _parameters.begin();
        ++it;
        for(; it != _parameters.end(); ++it){
            GSLibParType* replicate_param = (*it)->clone();
            if( replicate_param )
                clone->_parameters.append( replicate_param );
            else
                Application::instance()->logError("GSLibParMultiValuedVariable::clone() 2: parameters of type \"" + _parameters.at(0)->getTypeName() + "\" do not support cloning.");
        }
    } else {
        Application::instance()->logError("GSLibParMultiValuedVariable::clone() 1: parameters of type \"" + _parameters.at(0)->getTypeName() + "\" do not support cloning.");
    }
    return clone;
}


