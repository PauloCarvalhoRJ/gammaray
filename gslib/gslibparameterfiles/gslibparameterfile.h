#ifndef GSLIBPARAMETERFILE_H
#define GSLIBPARAMETERFILE_H

#include <QList>
#include <QString>
#include <QStringList>
#include "../gslibparams/gslibpartype.h"

class QTextStream;
class GSLibParRepeat;
class VariogramModel;
class CartesianGrid;

/**
 * @brief The GSLibParameterFile class represents a parameter file for a GSLib program.
 * GSLibParameterFile can be regarded as an a priori empty object whose members are added during runtime
 * depending on the parameter file template for a GSLib program passed to its constructor.
 *
 * TODO: This class is a good candidate for a refactoring.
 *
 */
class GSLibParameterFile
{
public:
    /**
     * @brief constructs the object by loading the parameter file template for the given program.
     * @param program_name Name of the target GSLib program, e.g. histplt.
     */
    GSLibParameterFile( const QString program_name );

    /**
     * Constructs an undefined/empty parameter file object.
     * This is useful to open the plot display dialog just to display a saved plot or experimental variogram,
     * which doesn't require setting up GSLib parameters.
     */
    GSLibParameterFile();

    virtual ~GSLibParameterFile();

    /**
      *  Sets default values to the parameters.  The values obviously depend on the program name
      *  passed to the constructor as this object composition depends on the target GSLib program.
      *  Setting default values allows the caller code or the user to set less parameters depending on the task.
      */
    void setDefaultValues();

    /**
     * Sets the values reading from a saved parameter file.  The parameter file must be made for the GSLib program
     * whose name was passed to the object constructor, otherwise an error or abend will ensue.
     */
    void setValuesFromParFile( const QString path );

    /**
     * @brief Saves this object as a parameter file.
     */
    void save( const QString path );

    /** Returns the target GSLib program name for this parameter set. */
    QString getProgramName();

    /** Returns the parameter count. */
    int getParameterCount();

    /**
     * Returns a parameter object cast to the desired parameter class.
     * @param i order in the parameter file template.
     */
    template<typename T>
    T getParameter(int i);

    /**
     * Returns a parameter object cast to the desired parameter class.
     * @param name Name given to the parameters (works only on named parameters).  Returns nullptr if no parameter
     *             with the given name is not found.
     */
    template<typename T>
    T getParameterByName(QString name);

    /**
     * Performs a deep search in the main parameter collection as well as collections within it to find the
     * parameter with the given name.  It is useful to find named parameters that are not directly children
     * of the _params collection, a limitation of the getParameterIndexByName() method.  Returns a null pointer
     * if the serach fails.
     */
    GSLibParType* findParameterByName( const QString par_name );

    /**
     * Returns whether this parameter file object contains parameters.
     */
    bool isEmpty();

    /** Returns the same as isEmpty(). */
    bool isUndefined();

    /** Sets the variogram model parameters using the values from
     * the given variogram model object.
     * This method has no effect if this parameter set does not have a variogram model.
     */
    void setVariogramModel(VariogramModel *vm );

    /**
     * Saves the variogram model parameters as a vmodel program parameter file given a full path to it.
     * This method has no effect if this parameter set does not have a variogram model.
     */
    void saveVariogramModel( const QString vmodel_par_path );

    /**
     * Copies the variogram model of the given parameter file object to this one.
     * This method has no effect if either this or the source parameter set does not have a variogram model.
     */
    void copyVariogramModel( GSLibParameterFile* gpf_from );

    /**
     * Sets the grid parameters according to the given CartesianGrid grid parameters.
     * This method has no effect if this parameter set does not have a parameter of type GSLibParGrid.
     * NOTE: all parameters of type GSLibParGrid found will be set.
     */
    void setGridParameters( CartesianGrid* cg );

    /** Adds a GSLib parameter object (any subclass of GSLibParType) to the collection of parameters.
     * This functions is useful in applications that require an ad-hoc parameter setting instead of
     * reading them from a parameter template file.
     */
    void addParameter( GSLibParType* param );

	/**
	 * Populates this parameter set to work with Factorial Kriging.  This is not a GSLib program.
	 * This is not saved nor populated from a GSLib parameter file.  Instead, a parameter set
	 * populated for FK is a convenient way to create a parameter dialog for the FK internal
	 * implementation.  This reuses code insead if creating a new dialog in Qt Designer full of
	 * parameters from scratch along with all the associated set/get glue code.
	 */
	void makeParamatersForFactorialKriging();

public: //-------static functions---------------
    /**
      *  Generates all parameter file templates that may be missing in the given directory.
      *  If the directory is empty or no template is found, all templates are generated.
      */
    static void generateParameterFileTemplates( const QString directory_path );

private:
    /**
      * the header of the parameter file
      */
    QString _header;
    /**
     * @brief the collection of parameters.
     */
    QList<GSLibParType*> _params;
    /**
      * Stores the program name used to call the constructor.
      */
    QString _program_name;
    /**
     * stores the last <repeat> parameter object so subordinate parameters are added to it accordingly
     */
    GSLibParRepeat* _last_repeat;
    /**
      * Parses the given line from a paramter file template passed to the constructor.
      * If parsing is successful, an object of type according to the line is created and added to
      * the structure of this object.
      */
    void parseTemplateLine( const QString line );
    /**
     * Internal function for code reuse.
     * @param line_indentation The text line indentation found in file.  This value is used to define the scope of a <repeat> tag.
     * @param tag The tag to be parsed.  E.g. <double>
     * @param params Pointer to a list of GSLibParType* that will receive a new GSLibParType constructed with the tag contents.
     * @param tag_description An optional tag description. This is a helper text for the user.
     * @return True of the given type was regonized.
     */
    bool parseType( uint line_indentation, QString tag, QList<GSLibParType*>* params, QString tag_description );
    /**
     * Parses the parameter file text line so the given parameter object is filled in.
     */
    void parseParFileLine( const QString line, GSLibParType* par );
    /**
      * Called by parseParFileLine() to process parameter types that are themselves
      * collections of parameters.
      */
    void parseParFileLineWithMultipleValues( const QString line, GSLibParType* par );
    /**
     * Parses the given parameter file text lines so the given repeart parameter object's
     * inner parameters' values are set.
     */
    void parseLinesForRepeatObject( QStringList &lines, GSLibParRepeat* repeat_par );
    /**
      * Called by setDefaultValues if _program_name is "histplt".
      */
    void setDefaultValuesForHistplt();
    /**
      * Called by setDefaultValues if _program_name is "locmap".
      */
    void setDefaultValuesForLocmap();
    /**
      * Called by setDefaultValues if _program_name is "scatplt".
      */
    void setDefaultValuesForScatplt();
    /**
      * Called by setDefaultValues if _program_name is "pixelplt".
      */
    void setDefaultValuesForPixelplt();
    /**
      * Called by setDefaultValues if _program_name is "gamv".
      */
    void setDefaultValuesForGamv();
    /**
      * Called by setDefaultValues if _program_name is "vargplt".
      */
    void setDefaultValuesForVargplt();
    /**
      * Called by setDefaultValues if _program_name is "varmap".
      */
    void setDefaultValuesForVarmap();
    /**
      * Called by setDefaultValues if _program_name is "gam".
      */
    void setDefaultValuesForGam();
    /**
      * Called by setDefaultValues if _program_name is "declus".
      */
    void setDefaultValuesForDeclus();
    /**
      * Called by setDefaultValues if _program_name is "getpoints".
      */
    void setDefaultValuesForGetPoints();
    /**
      * Called by setDefaultValues if _program_name is "nscoremv".
      */
    void setDefaultValuesForNScoreMV();
    /**
      * Called by setDefaultValues if _program_name is "vmodel".
      */
    void setDefaultValuesForVmodel();
    /**
      * Called by setDefaultValues if _program_name is "histsmth".
      */
    void setDefaultValuesForHistsmth();
    /**
      * Called by setDefaultValues if _program_name is "scatsmth".
      */
    void setDefaultValuesForScatsmth();
    /**
      * Called by setDefaultValues if _program_name is "bivplt".
      */
    void setDefaultValuesForBivplt();
    /**
      * Called by setDefaultValues if _program_name is "probplt".
      */
    void setDefaultValuesForProbplt();
    /**
      * Called by setDefaultValues if _program_name is "qpplt".
      */
    void setDefaultValuesForQpplt();
    /**
      * Called by setDefaultValues if _program_name is "kt3d".
      */
    void setDefaultValuesForKt3d();
    /**
      * Called by setDefaultValues if _program_name is "ik3d".
      */
    void setDefaultValuesForIk3d();
    /**
      * Called by setDefaultValues() if _program_name is "postik".
      */
    void setDefaultValuesForPostik();
    /**
      * Called by setDefaultValues() if _program_name is "cokb3d".
      */
    void setDefaultValuesForCokb3d();
    /**
      * Called by setDefaultValues if _program_name is "histpltsim".
      */
    void setDefaultValuesForHistpltsim();
    /**
      * Called by setDefaultValues if _program_name is "sgsim".
      */
    void setDefaultValuesForSgsim();
    /**
      * Called by setDefaultValues if _program_name is "postsim".
      */
    void setDefaultValuesForPostsim();
    /**
      * Called by setDefaultValues if _program_name is "newcokb3d".
      */
    void setDefaultValuesForNewcokb3d();
    /**
      * Called by setDefaultValues if _program_name is "sisim", "sism_gs" or "sisim_lm".
      */
    void setDefaultValuesForSisim( QString sisimProgramName );
	/**
	  * Called by setDefaultValues if _program_name is "bicalib".
	  */
	void setDefaultValuesForBicalib( );

    /**
     * Wraps the given GSLib parameter type in a GSLibParMultivaluedVariable container object,
     * instead of directly adding it to the given parameter list.
     */
    void addAsMultiValued(QList<GSLibParType*>* params, GSLibParType *parameter );

    /**
     * Called in generateParameterFileTemplates() to generate the parts in common with the
     * three SISIM programs (sisim, sisim_gs and sisim_lm).
     * @param programName Either "sisim", "sisim_gs" or "sisim_lm".
     */
    static void generateParameterFileTemplatesSISIMCommons( QTextStream& out,
                                                            QString programName );
};


template<typename T>
T GSLibParameterFile::getParameter(int i)
{
    //TODO: this code needs more robustness.  It assumes it returns a valid pointer.
    return (T)(this->_params.at( i ));
}

template<typename T>
T GSLibParameterFile::getParameterByName(QString name)
{
    //TODO: this code needs more robustness.  It assumes it returns a valid pointer.
    QList<GSLibParType*>::iterator it = _params.begin();
    for(; it != _params.end(); ++it){
        if( (*it)->getName() == name )
            return (T)*it;
    }
    return nullptr;
}

#endif // GSLIBPARAMETERFILE_H
