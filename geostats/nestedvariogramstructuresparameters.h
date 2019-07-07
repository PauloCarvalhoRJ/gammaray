#ifndef NESTEDVARIOGRAMSTRUCTURESPARAMETERS_H
#define NESTEDVARIOGRAMSTRUCTURESPARAMETERS_H

#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include <memory>

class NestedVariogramStructuresParameters : public GSLibParameterFile
{
public:
    NestedVariogramStructuresParameters();

    void setNumberOfNestedStructures( int number );

    /**
     * Returs a QWidget that can be used to edit the values of this parameter set.
     */
    QWidget* getEditorWidget();

    double getSemiMajorAxis( int nNestedStructure );
    double getSemiMinorSemiMajorAxesRatio( int nNestedStructure );
    double getAzimuthAsRadians( int nNestedStructure );
    double getContribution( int nNestedStructure );

private:

    /** The pointer to the QWidget returned by getEditorWidget() in the last call. */
    QWidget* m_lastReturnedEditorWidget;

    /** Updates the parameter values withe the user-entered values.
     * This method should be called in all methods that returns the values
     * to ensure the parameters are updated.
     */
    void updateValuesFromEditorWidget();
};

typedef std::shared_ptr<NestedVariogramStructuresParameters> NestedVariogramStructuresParametersPtr;

#endif // NESTEDVARIOGRAMSTRUCTURESPARAMETERS_H
