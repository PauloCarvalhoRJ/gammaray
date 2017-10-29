#include "gslibparrepeat.h"
#include "../../domain/application.h"
#include "widgets/widgetgslibparrepeat.h"

GSLibParRepeat::GSLibParRepeat() : GSLibParType( "", "", "" )
{
}

void GSLibParRepeat::setCount(uint count)
{
    if( _original_parameters.size() < 1 ){
        Application::instance()->logError( "No nested parameters found in <repeat> parameter.");
        Application::instance()->logError( "Check the template parameter file for misspelled parameter types or lack of due indentation for the <repeat> parameter." );
        return;
    }
    // the number of repeated parameters are always multiples of _original_parameters.size()
    int number_of_repeated_parameters = _repeated_parameters.size() / _original_parameters.size();
    //does nothing if the count does not change
    if( ((int)count-1) != number_of_repeated_parameters ){
        if( count == 0)
            Application::instance()->logError( "Cannot set repeat count to zero." );
        else if( count == 1 ){ //count == 1 means only the objects in _original_parameters collection
            //de-allocate all the parameter objects first.
            qDeleteAll( _repeated_parameters.begin(), _repeated_parameters.end() );
            //clear the list.
            _repeated_parameters.clear();
        // if the count is decreasing
        } else if ( (int)(count-1) < number_of_repeated_parameters ) {
            int number_of_deletions = (number_of_repeated_parameters - (count-1)) * _original_parameters.size();
            for(; number_of_deletions > 0; --number_of_deletions){
                GSLibParType * par_to_be_deleted = _repeated_parameters.takeLast();
                delete par_to_be_deleted;
            }
        // if the count is increasing
        } else {
            //adds only the increase in count.  For example, if number of repeated parameters is 2 and count is set
            // to 3, then only one clone operation for each original parameter is necessary.
            for(int i = number_of_repeated_parameters+1; i < (int)count; ++i){
                for( QList<GSLibParType*>::iterator it = _original_parameters.begin();
                     it != _original_parameters.end(); ++it){
                    GSLibParType* par = (*it)->clone();
                    if( par ){
                        _repeated_parameters.append( par );
                    }else{
                        Application::instance()->logError( QString("GSLibParRepeat::setCount(): parameters of type \"").append((*it)->getTypeName()).append("\" do not support cloning." ));
                    }
                }
            }
        }
    }
}

uint GSLibParRepeat::getReplicateCount()
{
    // the number of repeated parameters are always multiples of _original_parameters.size()
    return _repeated_parameters.size() / _original_parameters.size();
}

uint GSLibParRepeat::getCount()
{
    return getReplicateCount() + 1;
}

uint GSLibParRepeat::getTotalParameterCount()
{
    return _original_parameters.size() + _repeated_parameters.size();
}

void GSLibParRepeat::save(QTextStream *out)
{
    //this tag is not saved per se.
    //instead, it just calls its concrete child objects to save.
    for(QList<GSLibParType*>::iterator it = _original_parameters.begin(); it != _original_parameters.end(); ++it) {
        GSLibParType* parameter = (*it);
        parameter->save( out );
    }
    for(QList<GSLibParType*>::iterator it = _repeated_parameters.begin(); it != _repeated_parameters.end(); ++it) {
        GSLibParType* parameter = (*it);
        parameter->save( out );
    }
}

QWidget *GSLibParRepeat::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParRepeat();
    ((WidgetGSLibParRepeat*)this->_widget)->fillFields( this );
    return this->_widget;
}

GSLibParRepeat *GSLibParRepeat::clone()
{
    GSLibParRepeat *new_repeat = new GSLibParRepeat();
    new_repeat->_ref_par_name = _ref_par_name;

    QList<GSLibParType*>::iterator it = _original_parameters.begin();
    for(; it != _original_parameters.end(); ++it){
        GSLibParType* new_parameter = (*it)->clone();
        if( new_parameter )
            new_repeat->_original_parameters.append( new_parameter );
        else
            Application::instance()->logError( QString("GSLibParRepeat::clone(): parameters of type \"").append((*it)->getTypeName()).append("\" do not support cloning." ));
    }

    it = _repeated_parameters.begin();
    for(; it != _repeated_parameters.end(); ++it){
        GSLibParType* new_parameter = (*it)->clone();
        if( new_parameter )
            new_repeat->_repeated_parameters.append( new_parameter );
        else
            Application::instance()->logError( QString("GSLibParRepeat::clone(): parameters of type \"").append((*it)->getTypeName()).append("\" do not support cloning." ));
    }

    return new_repeat;
}

bool GSLibParRepeat::update()
{
    ((WidgetGSLibParRepeat*)this->_widget)->updateValue( this );
    return true;
}
