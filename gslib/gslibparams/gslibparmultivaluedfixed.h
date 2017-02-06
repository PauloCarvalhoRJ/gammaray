#ifndef GSLIBPARMULTIVALUEDFIXED_H
#define GSLIBPARMULTIVALUEDFIXED_H

#include "gslibpartype.h"
#include <QList>

/**
 * @brief The GSLibParMultiValuedFixed represents a fixed list of parameters (e.g. <double> <double> <string>).
 */
class GSLibParMultiValuedFixed : public GSLibParType
{
public:
    GSLibParMultiValuedFixed(const QString name, const QString label, const QString description);
    ~GSLibParMultiValuedFixed();

    QList<GSLibParType*> _parameters;

    //TODO: create destructor and delete objects in _parameters there.

    /**
     * Returns an atomic parameter object of the given parameter pointer type.
     * @param i order in the parameter list.
     */
    template<typename T>
    T getParameter(int i);

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "multivaluedfixed";}
    QWidget* getWidget();
    bool update();
    GSLibParMultiValuedFixed* clone();
    bool isCollection() { return true; }
    IGSLibParameterFinder* getFinder();
private:
    IGSLibParameterFinder* m_finder;
};


template<typename T>
T GSLibParMultiValuedFixed::getParameter(int i)
{
    //TODO: this code needs more robustness.  It assumes it returns a valid pointer.
    return (T)(this->_parameters.at( i ));
}

#endif // GSLIBPARMULTIVALUEDFIXED_H
