#ifndef GSLIBPARMULTIVALUEDVARIABLE_H
#define GSLIBPARMULTIVALUEDVARIABLE_H

#include "gslibpartype.h"
#include <QList>


/**
 * @brief The GSLibParMultiValuedVariable represents a variable length list of
 *  parameters of a given type (e.g. <double+>).
 */
class GSLibParMultiValuedVariable : public GSLibParType
{
public:
    /**
     * Construct with at least one GSLibParType object.
     */
    GSLibParMultiValuedVariable( GSLibParType* original_parameter );

    /**
     * Returns an atomic parameter object of the given parameter pointer type.
     * @param i order in the parameter list.
     */
    template<typename T>
    T getParameter(int i);

    //TODO: encapsulate this
    QList<GSLibParType*> _parameters;

    /** Returns the name of the type of the parameters that can be added to this GSLib parameter collection. */
    QString getAllowedParameterTypeName();

    /** Assures the given number of child parameter objects. Nothing happens if the number is already greater
      than or equal to the given number. */
    void assure(uint n);

    /** Sets the number of elements in this parameter collection.*/
    void setSize(uint n);

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "multivaluedvariable";}
    QWidget* getWidget();
    bool isCollection() { return true; }
    GSLibParMultiValuedVariable* clone();
    bool update();

private:
    template<typename T>
    void assureLoop(uint n);

};

template<typename T>
T GSLibParMultiValuedVariable::getParameter(int i)
{
    //TODO: this code needs more robustness.  It assumes it returns a valid pointer.
    return (T)(this->_parameters.at( i ));
}

#endif // GSLIBPARMULTIVALUEDVARIABLE_H
