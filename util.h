#ifndef UTIL_H
#define UTIL_H
#include <QStringList>
#include <QString>
#include <QList>

//macro used to do printf on QString for debugging purposes
//it is safe to delete this.
#define PRINT(x) printf("%s\n", x.toStdString().c_str())

class QWidget;
class QPlainTextEdit;
class CartesianGrid;
class Attribute;

/*! Display resolution classes used to select an adequate set of icons and maybe other
 *  GUI measures sensitive to display resolution. */
enum class DisplayResolution : uint {
    NORMAL_DPI = 0, /*!< For usual SVGA displays or high vertical resultion in physically large screens. */
    HIGH_DPI        /*!< For the so-called 4k displays, unless exceptionally large screens. */
};


/**
 * @brief The Util class organizes system-wide utilitary functions.
 */
class Util
{
public:
    Util();

    /** Returns the list of variable names available in the given
     * GSLib format data file.  GSLib files are in GEO-EAS format.
     * The list is in the same order as found in the file, so you
     * can use QStringList's index to find the variable column in
     * the file.
     */
    static QStringList getFieldNames( const QString gslib_data_file_path );

    /**
     * @brief Decomposes a parameter file template line into its tags and description.
     * Param types in parameter file templates follows the pattern "<type[specifiers]> <type2>  -description"
     * @return A pair composed by a string list with the tags found and a separate string with the description text.
     */
    static std::pair<QStringList, QString> parseTagsAndDescription( const QString gslib_param_file_template_line );

    /**
     * @brief Returns the number of leading spaces in the given template file line.
     * This value defines the scope of the special <repeat> tag.
     */
    static uint getIndentation( const QString gslib_param_file_template_line );

    /**
     * Returns the type name of the given tag.  For example: for the tag "<range [-1:L] [1:R]>" the
     * function returns "range".
     */
    static QString getNameFromTag( const QString tag );

    /**
     * Returns wether the passed tag has a plus (+) sign or not, denoting a variable
     * length multivalued parameter.
     */
    static bool hasPlusSign(const QString tag);

    /**
     * Returns the reference name of the given tag.  For example: for the tag "<string (title)>" the
     * function returns "title".  Returns an empty string if the parameter is anonymous.
     */
    static QString getRefNameFromTag( const QString tag );

    /**
     * Returns a list of pairs (option value, option description) that may be found inside a tag.
     * For example: for the tag "<range [-1:L] [1:R]>" the function returns the list of two pairs (-1, L),(1, R)
     */
    static std::vector< std::pair<QString, QString> > getTagOptions( const QString tag );

    /**
     * Returns a tag reference by name that may be found inside a tag.
     * For example: for the tag "<repeat [nvar]>" the function returns "nvar", which is a name given to
     * another tag.  Returns an empty string if there is no referece to another tag.
     */
    static QString getReferenceName( const QString tag );

    /**  Perfoms a reliable way to compare float values.
    * credit: Bruce Dawson
    * source: http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm
    */
    static bool almostEqual2sComplement(float A, float B, int maxUlps);

    /**
     * 64-bit version of almostEqual2sComplement by me.
     */
    static bool almostEqual2sComplement(double A, double B, int maxUlps);

    /**
      *  Remove all child widgets from the given widget.
      */
    static void clearChildWidgets( QWidget* widget );

    /**
     * Fills the given QPlainTextEdit with some lines of the given text file.
     */
    static void readFileSample( QPlainTextEdit* text_field, QString file_path );

    /**
     * Adds a triling hiphen to each parameter line of a .par text file.
     * Some GSLib programs fail when the .par file lacks those.
     */
    static void addTrailingHiphens(const QString par_file_path );

    /**
     * @brief Returns the first number in the given text file line.
     */
    static uint getFirstNumber( const QString line );

    /**
     * Returns the part of a GSLib parameter file line before the hiphen signaling the line comment.
     * Returns the entire line is no comment exists.
     * Example: for "10 5 2    -nx, ny, nz" returns "10 5 2"
     */
    static QString getValuesFromParFileLine( const QString line );

    /**
     *  Replaces asterisks created by a bug in varmap with a no-data value.
     */
    static void fixVarmapBug( const QString varmap_grid_file_path );

    /**
     * Renames one variable name in a GEO-EAS file.
     * @note This function only works on the physical file only.  Metadata kept by client code
     *       must update its internally kept information from the changed file.
     * @note This function also trims heading and trailing spaces from the variable names.
     */
    static void renameGEOEASvariable( const QString file_path,
                                      const QString old_name,
                                      const QString new_name );
    /**
     * Copies a physical file.  If there is a file in the destination path, overwrites.
     */
    static void copyFile( const QString from_path, const QString to_path );

    /**
     * Copies a physical file to the specified directory.
     * If there is a file in the destination path, overwrites.
     */
    static void copyFileToDir( const QString from_path, const QString path_to_directory );

    /**
     * Creates a GEO-EAS regular grid file using the grid specs from the
     * passed CartesianGrid object.  The resulting file contains a single binary variable
     * that results in a checkerboard battern in the plots.
     */
    static void createGEOEAScheckerboardGrid( CartesianGrid* cg, QString path );

    /**
     * Runs the GSLib program pixelplt and opens the plot dialog to view a
     * variable in a regular grid.
     * @param parent Parent QWidget for the plot dialog.
     */
    static void viewGrid(Attribute* variable , QWidget *parent);

    /**
     * Runs the GSLib program scatplt and opens the plot dialog to view a
     * a crossplot between two or three variable.
     * @param parent Parent QWidget for the plot dialog.
     * @note This method assumes all variables belong to the same parent file.
     */
    static void viewXPlot( Attribute* xVariable, Attribute* yVariable, QWidget *parent, Attribute* zVariable = nullptr );

    /** Returns the total size, in bytes, of the contents of the given directory.
     *  It is a recursive function, so it may take a while for large directories.
     */
    static qint64 getDirectorySize( const QString path );

    /** Deletes all files in the given directory. */
    static void clearDirectory( const QString path );

    /** Renames the file given by path.
     *  @param newName The new file name, e.g. "foo.txt"
     *  @return The complete path to the file after renaming.
     */
    static QString renameFile( const QString path, const QString newName );

    /**
     * The value of no-data value generated by the varmap GSLib program.
     * See util.cpp for the constant definition.
     */
    static const QString VARMAP_NDV;

    /**
    *  Imports a univariate distribution file to the project as a new UnivariateDistribution object.
    *  @param at The attribute that originated the distribution.
    *  @param path_from The complete path to the file.
    *  @param dialogs_owner Widget used to anchor the modal dialogs to.
    */
    static void importUnivariateDistribution(Attribute *at, const QString path_from , QWidget *dialogs_owner);

    /**
     * Populates the passed list with QColor objects containing colors according to GSLib convention.
     * The indexes in the list + 1 are the GSLib color codes.  For instance, red is the first color,
     * list index == 0, thus the color code is 1.
     */
    static void makeGSLibColorsList( QList<QColor> &colors );

    /**
     * Creates a 16x16 pixel QIcon filled with a GSLib color given its code.
     */
    static QIcon makeGSLibColorIcon( uint color_code );
    
    /**
     * Imports the registry/user home seetings from a previus version of GammaRay.
     * The import happens only if there are no seetings for this version.
     * Nothing happens if there no previous settings are found.
     */
    static void importSettingsFromPreviousVersion();

    /**
     * Returns an ID to classify display resolutions.
     */
    static DisplayResolution getDisplayResolutionClass();

    /**
     * Return the last browsed directory in a file dialog.
     */
    static QString getLastBrowsedDirectory();

    /**
     * Saves the last browsed directory in a file dialog to registry/home settings.
     * The parameter must be a directory path.
     */
    static void saveLastBrowsedDirectory(QString dir_path);

    /**
     * Saves the last browsed directory in a file dialog to registry/home settings.
     * The parameter must be a file path.
     */
    static void saveLastBrowsedDirectoryOfFile(QString file_path);

    /**
     * Returns the PROGRAMFILES environmental variable in Windows or /usr otherwise.
     */
    static QString getProgramInstallDir();
};

#endif // UTIL_H
