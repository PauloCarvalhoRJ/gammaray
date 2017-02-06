#ifndef GSLIBPARREPEAT_H
#define GSLIBPARREPEAT_H
#include "gslibpartype.h"
#include "../../domain/application.h"
#include <QList>

/**
 * @brief The GSLibParRepeat class is a special parameter token that does not represent an
 * actual parameter.  Instead, it causes the following tags to repeat by the number set in this tag.
 * Please refer to the template syntax explanation text (template_syntax.txt).
 */
class GSLibParRepeat : public GSLibParType
{
public:
    GSLibParRepeat();
    QString _ref_par_name; //the name of another parameter whose value is used
                           //to repeat the original parameters
    QList<GSLibParType*> _original_parameters; //the parameters originally declares in the template file
    QList<GSLibParType*> _repeated_parameters; //possible parameters replicates

    /** Sets the repetition count.
     * It causes the objects in _original_parameters colletion to be cloned by the given number in
     * _repeated_parameters collection.
      */
    void setCount(uint count);

    /** Returns the number of times the parameters in _original_parameters colletion were CLONED.
     * So it returns ZERO if there are only the original parameters.
     * @note Do not confuse this number with the parameter count.  One should call the collections'
     *       size() method to get the parameter count.
     */
    uint getReplicateCount();

    /**
     * Returns the number of copies of the parameters in _original_parameters.
     * The lowest value is 1 (only the original parameters as delcared in the template).
     * In practical termos, it returns getReplicateCount() + 1.
     */
    uint getCount();

    /**
     * Returns the total count of parameters in this object.  In practical termos, it returns
     * _original_parameters.size() + _repeated_parameters.size().
     */
    uint getTotalParameterCount();

    /**
     * Returns an child parameter object of the given parameter pointer type.
     * @param r repetition count (0 for the original parameter set, 1+ for their replicates).
     * @param i parameter index in the original or in a replicate list.
     */
    template<typename T>
    T getParameter(uint r, uint i);

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "repeat";}
    QWidget* getWidget();
    GSLibParRepeat* clone();
    bool update();
    bool isRepeat() {return true;}
    bool isCollection() { return true; }
};

template<typename T>
T GSLibParRepeat::getParameter(uint r, uint i)
{
    //TODO: this code needs more robustness.  It assumes it returns a valid pointer.
    if( r == 0 )
        return (T)(this->_original_parameters.at( i ));
    else{
        //conpute first parameter of given repetition number.
        int offset = (r-1)*_original_parameters.size();
        return (T)(this->_repeated_parameters.at( offset + i ));
    }
}

#endif // GSLIBPARREPEAT_H
