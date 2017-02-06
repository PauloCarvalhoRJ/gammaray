#include "gslibparmultivaluedfixed.h"
#include <QTextStream>
#include "widgets/widgetgslibparmultivaluedfixed.h"
#include "../../domain/application.h"
#include "../igslibparameterfinder.h"

/**
 * Implementation of the IGSLibParameterFinder interface.
 */
class GSLibParMultiValuedFixedFinder : public IGSLibParameterFinder
{
public:
    GSLibParMultiValuedFixedFinder(  GSLibParMultiValuedFixed* parameter ) : IGSLibParameterFinder(){
        m_parameter = parameter;
    }
//interface methods
    GSLibParType* findParameterByName(const QString name){
        for( int i = 0; i < m_parameter->_parameters.size(); ++i )
            if( m_parameter->_parameters.at( i )->isNamed( name ) )
                return m_parameter->_parameters.at( i );
        Application::instance()->logWarn( QString("GSLibParMultiValuedFixedFinder::findParameterByName(): parameter [").append(name).append("] not found in this collection.  It may be in another GSLibParMultiValuedFixed object.") );
        return nullptr;
    }
private:
    GSLibParMultiValuedFixed* m_parameter;
};

GSLibParMultiValuedFixed::GSLibParMultiValuedFixed(const QString name, const QString label, const QString description) :
    GSLibParType(name, label, description),
    m_finder( nullptr )
{

}

GSLibParMultiValuedFixed::~GSLibParMultiValuedFixed()
{
    //TODO: may also require deletion of child objects.
    delete m_finder;
}

void GSLibParMultiValuedFixed::save(QTextStream *out)
{
    QString buffer;
    //use a stream operating on a string to get and modify the values (e.g. remove line breaks)
    //of the component parameters before commiting them to the file stream.
    QTextStream local_str_stream( &buffer );
    for(QList<GSLibParType*>::iterator it = _parameters.begin(); it != _parameters.end(); ++it) {
        GSLibParType* parameter = (*it);
        parameter->save( &local_str_stream );
    }
    //replace end line characters with spaces
    buffer.replace('\n', ' ');
    buffer.replace('\r', ' ');
    //commit the values list to the file as single line of text
    *out << buffer << '\n';
}

QWidget *GSLibParMultiValuedFixed::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParMultiValuedFixed();
    ((WidgetGSLibParMultiValuedFixed*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParMultiValuedFixed::update()
{
    ((WidgetGSLibParMultiValuedFixed*)this->_widget)->updateValue( this );
    return true;
}

GSLibParMultiValuedFixed *GSLibParMultiValuedFixed::clone()
{
    GSLibParMultiValuedFixed* new_par = new GSLibParMultiValuedFixed("", "", _description);
    for( QList<GSLibParType*>::iterator it = _parameters.begin(); it != _parameters.end(); ++it ){
        GSLibParType* sub_par = (*it)->clone();
        if( sub_par ){
            new_par->_parameters.append( sub_par );
        }else{
            Application::instance()->logError( QString("GSLibParMultiValuedFixed::clone(): parameters of type\"").append((*it)->getTypeName()).append("\" do not support cloning.") );
        }
    }
    return new_par;
}

IGSLibParameterFinder *GSLibParMultiValuedFixed::getFinder()
{
    if( ! m_finder )
        m_finder = new GSLibParMultiValuedFixedFinder( this );
    return m_finder;
}

