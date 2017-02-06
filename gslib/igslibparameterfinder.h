#ifndef IGSLIBPARAMETERFINDER_H
#define IGSLIBPARAMETERFINDER_H

#include <QString>

class GSLibParType;

/**
 * @brief This is an interface that GSLib parameters that are themselves parameter collections
 * can return via the GSLibParType::getFinder() to provide search services.
 */
class IGSLibParameterFinder
{
public:
    IGSLibParameterFinder();
    virtual ~IGSLibParameterFinder(){}

    /**
     *  Implementors should return a null pointer of the search fails.
     */
    virtual GSLibParType* findParameterByName( const QString name ) = 0;

};

#endif // IGSLIBPARAMETERFINDER_H
