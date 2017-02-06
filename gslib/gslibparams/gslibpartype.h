#ifndef GSLIBPARTYPE_H
#define GSLIBPARTYPE_H
#include <QString>

class QTextStream;
class QWidget;
class QLayout;
class IGSLibParameterFinder;

/**
 * @brief The GSLibParType class is the base class of all GSLib parameter types.
 */
class GSLibParType
{
public:
    GSLibParType(const QString name, const QString label, const QString description);
    virtual ~GSLibParType();
    /** Returns whether this parameter has the given name. */
    bool isNamed(const QString name);
    /** Saves internal state (normally a value) to the given stream (normally a file). */
    virtual void save( QTextStream* out) = 0;
    /** Returns the UI component used to manipulate the value of the parameter.
     * The type of widget depends on how the subclass instantiates it.
     */
    virtual QWidget* getWidget();
    /**
      * Returns the parameter type name.  E.g. string.
      */
    virtual QString getTypeName() = 0;
    /**
      * Returns the paramater description
      */
    virtual QString getDescription() { return _description; }
    /**
     * Updates the paramter velue according to the user input to the associated widget.
     * @see getWidget()
     * @return If false, the update() method was not implemented (read-only parameters).
     *  or the update failed for some unknown reason (invalid or absent widget, for example).
     *  Subclasses must return true for successful updates.
     */
    virtual bool update();
    /**
      * Informs whether the parameter is of <repeat> type.  Defaults to false.
      */
    virtual bool isRepeat();
    /**
      * Returns a new object of the same type and the same values.
      * It is recommended that sub-classes perform a deep copy.  Sub-classes that do not support
      * cloning must return a null pointer or simply do not override this method.
      */
    virtual GSLibParType* clone();
    /**
     * Returns the parameter name as declared in the paramater file template.
     * Returns an empty string if the parameter is anonymous (most GSLib parameters are anonymous).
     */
    QString getName() { return _name; }
    /**
     * Informs whether the parameter is itself a collection of parameters.
     * Subclasses of parameters mereley composed by more than one C++ type values (e.g. GSLibParLimitsDouble) are NOT parameter collections.
     * Parameter collections are those with more than one GSLibParType object within it.
     */
    virtual bool isCollection() = 0;
    /**
     * Subclasses should override this method if they want to support parameter search services.
     */
    virtual IGSLibParameterFinder* getFinder() { return nullptr; }

    /**
     * Performs the necessary operations so the WidgetGSLib* created bt getWidget()
     * is not automatically deleted by Qt.  Subclasses that are collections of other
     * GSLibPar* objects are encouraged to extend this method by calling detachFromGUI()
     * for each child, essentially creating a recursive detachment process.
     * This is necessary because QLayout::addWidget() sets QWidget::parent.
     * @see http://stackoverflow.com/questions/6035001/does-a-qwidget-remove-itself-from-the-gui-on-destruction
     * Without this detachment, widgets created by getWidget() are destroyed by Qt,
     * leaving the _widget pointer dangling, leading to crashes caused by access violations.
     * This method is normally called from a window or dialog destructor.
     * Calling this method while the GUI is visible will result in widgets disappearing from the screen.
     */
    virtual void detachFromGUI( QLayout* parentLayout );

protected:
    /** This is the internal parameter name. */
    QString _name;           //eg.: "VerticalScaling"
    /** This is a short descriptive text used, for instance, in GUI labels. */
    QString _label;           //eg.: "Vertical scaling"
    /** This is an extended text to explain details of the parameter. */
    QString _description;    //eg.: "Sets the scaling of the graph along the Y axis."
    /** The widget used to manipulate the parameter value.
     * It is up to the subclasses to create it.
     */
    QWidget* _widget;
};

#endif // GSLIBPARTYPE_H
