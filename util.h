﻿#ifndef UTIL_H
#define UTIL_H
#include <QColor>
#include <QIcon>
#include <QList>
#include <QString>
#include <QStringList>
#include <cassert>
#include <complex>
#include "geometry/face3d.h"
#include "viewer3d/view3dcolortables.h"
#include "domain/faciestransitionmatrix.h"

// macro used to do printf on QString for debugging purposes
// it is safe to delete this.
#define PRINT(x) printf("%s\n", x.toStdString().c_str())

// The usual value of 10.0 for scaling in decibel (dB).
// The value of 20.0 (2*10) is usually for power measurements (square law).
#define DECIBEL_SCALE_FACTOR 10.0

class QWidget;
class QPlainTextEdit;
class QFrame;
class CartesianGrid;
class Attribute;
class CategoryDefinition;
class VariogramModel;
class SpatialLocation;
class GridFile;
namespace spectral{
    struct array;
}

/*! Display resolution classes used to select an adequate set of icons and maybe other
 *  GUI measures sensitive to display resolution. */
enum class DisplayResolution : uint {
    NORMAL_DPI
    = 0,     /*!< For usual SVGA displays or high vertical resultion in physically large
                screens. */
    HIGH_DPI /*!< For the so-called 4k displays, unless exceptionally large screens. */
};

/*! FFT computation mode for fft1d() and fft2d(). */
enum class FFTComputationMode : int {
    DIRECT = 0, /*!< Functions fft*() take an input in real space and result is in
                   frequency space. */
    REVERSE /*!< Functions fft*() take an input in frequency space and result is in real
               space. */
};

/*! FFT image type for functions fft*(). */
enum class FFTImageType : int {
    RECTANGULAR_FORM = 0, /*!< Functions fft*() complex numbers are directly their
                             real and imaginary components. */
    POLAR_FORM            /*!< Functions fft*() complex numbers are in their polar form
                             (magnitude == real part, angle == imaginary part). */
};

/*! Computation direction for fft1d() when taking a 3D array. */
enum class FFT1DDirection : int {
    DIR_I = 0, /*!< Computes along I (inlines). */
    DIR_J,     /*!< Computes along J (crosslines). */
    DIR_K      /*!< Computes along K (traces). */
};

enum class ColorScaling : uint { ARITHMETIC = 0, LOG };

enum class ValueScaling : uint { DIRECT = 0, ABS };

enum class SortingOrder : int {
    ASCENDING,
    DESCENDING
};

/** Enum that can be used to avoid the slow File::getFileType() in performance-critical code. */
enum class DataSetType : int {
    UNDEFINED,
    POINTSET,
    CARTESIANGRID,
    GEOGRID,
    SEGMENTSET
};


//one Facies Transition Matrix per h.
typedef double Separation;
typedef std::pair<Separation, FaciesTransitionMatrix> hFTM;

/**
 * @brief The Util class organizes system-wide utilitary functions.
 */
class Util
{
public:
    Util();

    /** The math constant PI. */
    static const long double PI;

    /** Constant used to convert degrees to radians. */
    static const long double PI_OVER_180;

    /** Returns the list of variable names available in the given
     * GSLib format data file.  GSLib files are in GEO-EAS format.
     * The list is in the same order as found in the file, so you
     * can use QStringList's index to find the variable column in
     * the file.
     */
    static QStringList getFieldNames(const QString gslib_data_file_path);

    /**
     * @brief Decomposes a parameter file template line into its tags and description.
     * Param types in parameter file templates follows the pattern "<type[specifiers]>
     * <type2>  -description"
     * @return A pair composed by a string list with the tags found and a separate string
     * with the description text.
     */
    static std::pair<QStringList, QString>
    parseTagsAndDescription(const QString gslib_param_file_template_line);

    /**
     * @brief Returns the number of leading spaces in the given template file line.
     * This value defines the scope of the special <repeat> tag.
     */
    static uint getIndentation(const QString gslib_param_file_template_line);

    /**
     * Returns the type name of the given tag.  For example: for the tag "<range [-1:L]
     * [1:R]>" the
     * function returns "range".
     */
    static QString getNameFromTag(const QString tag);

    /**
     * Returns wether the passed tag has a plus (+) sign or not, denoting a variable
     * length multivalued parameter.
     */
    static bool hasPlusSign(const QString tag);

    /**
     * Returns the reference name of the given tag.  For example: for the tag "<string
     * (title)>" the
     * function returns "title".  Returns an empty string if the parameter is anonymous.
     */
    static QString getRefNameFromTag(const QString tag);

    /**
     * Returns a list of pairs (option value, option description) that may be found inside
     * a tag.
     * For example: for the tag "<range [-1:L] [1:R]>" the function returns the list of
     * two pairs (-1, L),(1, R)
     */
    static std::vector<std::pair<QString, QString>> getTagOptions(const QString tag);

    /**
     * Returns a tag reference by name that may be found inside a tag.
     * For example: for the tag "<repeat [nvar]>" the function returns "nvar", which is a
     * name given to
     * another tag.  Returns an empty string if there is no referece to another tag.
     */
    static QString getReferenceName(const QString tag);

    /**  Perfoms a reliable way to compare float values.
    * credit: Bruce Dawson
    * source: http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm
    */
    static bool almostEqual2sComplement(float A, float B, int maxUlps);

    /**
     * 64-bit version of almostEqual2sComplement by me.
     */
    inline static bool almostEqual2sComplement(double A, double B, int maxUlps)
    {
        //TODO: replace the reinterpret_cast with an union:
        //typedef union
        // {
        //    int64_t i64;
        //    double dbl;
        //  } U64;
        //
        //  U64 in;
        //  in.dbl = ...;
        //  int64_t v = in.i64; //convert double's raw bytes into an integer value.

        // Make sure maxUlps is non-negative and small enough that the
        // default NAN won't compare as equal to anything.
        //<cassert>'s assert doesn't accept longs
        // assert(maxUlps > 0 && maxUlps < 2 * 1024 * 1024 * 1024 * 1024 * 1024);
        assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
        int64_t aLong = *reinterpret_cast<int64_t *>(&A); // use the raw bytes from the
                                                          // double to make a long int
                                                          // value (type punning)
        // Make aLong lexicographically ordered as a twos-complement long
        if (aLong < 0)
            aLong = 0x8000000000000000 - aLong;
        // Make bLong lexicographically ordered as a twos-complement long
        int64_t bLong = *reinterpret_cast<int64_t *>(&B); // use the raw bytes from the
                                                          // double to make a long int
                                                          // value (type punning)
        if (bLong < 0)
            bLong = 0x8000000000000000 - bLong;
        int64_t longDiff = (aLong - bLong) & 0x7FFFFFFFFFFFFFFF;
        if (longDiff <= maxUlps)
            return true;
        return false;
    }

    /**
      *  Remove all child widgets from the given widget.
      */
    static void clearChildWidgets(QWidget *widget);

    /**
     * Fills the given QPlainTextEdit with some lines of the given text file.
     */
    static void readFileSample(QPlainTextEdit *text_field, QString file_path);

    /**
     * Adds a triling hiphen to each parameter line of a .par text file.
     * Some GSLib programs fail when the .par file lacks those.
     */
    static void addTrailingHiphens(const QString par_file_path);

    /**
     * @brief Returns the first number in the given text file line.
     */
    static uint getFirstNumber(const QString line);

    /**
     * Returns the part of a GSLib parameter file line before the hiphen signaling the
     * line comment.
     * Returns the entire line is no comment exists.
     * Example: for "10 5 2    -nx, ny, nz" returns "10 5 2"
     */
    static QString getValuesFromParFileLine(const QString line);

    /**
     *  Replaces asterisks created by a bug in varmap with a no-data value.
     */
    static void fixVarmapBug(const QString varmap_grid_file_path);

    /**
     * Renames one variable name in a GEO-EAS file.
     * @note This function only works on the physical file only.  Metadata kept by client
     * code
     *       must update its internally kept information from the changed file.
     * @note This function also trims heading and trailing spaces from the variable names.
     */
    static void renameGEOEASvariable(const QString file_path, const QString old_name,
                                     const QString new_name);
    /**
     * Copies a physical file.  If there is a file in the destination path, overwrites.
     */
    static void copyFile(const QString from_path, const QString to_path);

    /**
     * Copies a physical file to the specified directory.
     * If there is a file in the destination path, overwrites.
     * @return The complete path after the copy operation.
     */
    static QString copyFileToDir(const QString from_path,
                                 const QString path_to_directory);

    /**
     * Creates a GEO-EAS regular grid file using the grid specs from the
	 * passed grid object.  The resulting file contains a single binary variable
     * that results in a checkerboard battern in the plots.
     */
	static void createGEOEAScheckerboardGrid(GridFile *gf, QString path);

    /**
     * Creates a GEO-EAS regular grid file using the given values
     * passed in the unidimensional vector of complex values.
     * If you omit a name for a field/column, the corresponding values will not be
     * written to the file.  If you omit (empty string) both names, no file will be
     * generated.
     * @note the array elements are expected to follow the GEO-EAS grid scan protocol for
     * the elemental
     * three indexes (i, j and k): array[ i + j*nI + k*nJ*nI ]
     */
    static void createGEOEASGrid(const QString columnNameForRealPart,
                                 const QString columnNameForImaginaryPart,
                                 std::vector<std::complex<double>> &array, QString path);

    /**
     * Creates a GEO-EAS regular grid file using the given values
     * passed in the unidimensional vector of doubles.
     * @note the array elements are expected to follow the GEO-EAS grid scan protocol for
     * the elemental
     * three indexes (i, j and k): array[ i + j*nI + k*nJ*nI ]
     */
    static void createGEOEASGrid(const QString columnName, std::vector<double> &values,
                                 QString path);

    /**
     * Creates a GEO-EAS regular grid file using the given values
     * passed as a spectra::array object.
     * @param silent If true, no user feedback (e.g. progress bar) is given.  This is normally set to false,
     *        but calling this from multiple threads may cause Qt to crash.  Hence, for such application it is
     *        necessary to enable this flag.
     * @param cg_to_print_definitions_from If this pointer is provided, the grid definitions are printed in
     *        the first line of the GEO-EAS file (comment).
     */
    static void createGEOEASGrid(const QString columnName,
                                 const spectral::array &values,
                                 QString path,
                                 bool silent = false,
                                 const CartesianGrid* cg_to_print_definitions_from = nullptr );

    /**
     * Creates a GEO-EAS regular grid file using the given values
     * passed in the bidimensional vector of values.
     * @note the array elements are expected to follow the GEO-EAS grid scan protocol for
     * the elemental
     * three indexes (i, j and k): array[ i + j*nI + k*nJ*nI ]
     * @param gridDescription A descriptive text for the new grid.  It is the first line
     * of the GEO-EAS grid file.
     */
    static void createGEOEASGridFile(const QString gridDescription,
                                     std::vector<QString> columnNames,
                                     std::vector<std::vector<double>> &array,
                                     QString path);

    /**
     * Runs the GSLib program pixelplt and opens the plot dialog to view a
     * variable in a regular grid.
     * @param parent Parent QWidget for the plot dialog.
     * @param modal If true, the method returns only when the user closes the Plot Dialog.
     * @param cd If informed, the grid is renderd as a categorical variable.
     * @return True if modal == true and if the user did not cancel the Plot Dialog; false
     * otherwise.
     */
    static bool viewGrid(Attribute *variable, QWidget *parent, bool modal = false,
                         CategoryDefinition *cd = nullptr);

    /**
     * Runs the GSLib program locmap and opens the plot dialog to view a
     * variable in a point set file.
     * @param parent Parent QWidget for the plot dialog.
     * @param modal If true, the method returns only when the user closes the Plot Dialog.
     */
    static bool viewPointSet(Attribute *variable, QWidget *parent, bool modal = false);

    /**
     * Runs the GSLib program scatplt and opens the plot dialog to view a
     * a crossplot between two or three variable.
     * @param parent Parent QWidget for the plot dialog.
     * @note This method assumes all variables belong to the same parent file.
     */
    static void viewXPlot(Attribute *xVariable, Attribute *yVariable, QWidget *parent,
                          Attribute *zVariable = nullptr);

    /** Returns the total size, in bytes, of the contents of the given directory.
     *  It is a recursive function, so it may take a while for large directories.
     */
    static qint64 getDirectorySize(const QString path);

    /** Deletes all files in the given directory. */
    static void clearDirectory(const QString path);

    /** Renames the file given by path.
     *  @param newName The new file name, e.g. "foo.txt"
     *  @return The complete path to the file after renaming.
     */
    static QString renameFile(const QString path, const QString newName);

    /**
     * The value of no-data value generated by the varmap GSLib program.
     * See util.cpp for the constant definition.
     */
    static const QString VARMAP_NDV;

    /**
    *  Imports a univariate distribution file to the project as a new
    * UnivariateDistribution object.
    *  @param at The attribute that originated the distribution.
    *  @param path_from The complete path to the file.
    *  @param dialogs_owner Widget used to anchor the modal dialogs to.
    */
    static void importUnivariateDistribution(Attribute *at, const QString path_from,
                                             QWidget *dialogs_owner);

    /**
     * Populates the passed list with QColor objects containing colors according to GSLib
     * convention.
     * The indexes in the list + 1 are the GSLib color codes.  For instance, red is the
     * first color,
     * list index == 0, thus the color code is 1.
     */
    static void makeGSLibColorsList(QList<QColor> &colors);

    /**
     * Creates a 16x16 pixel QIcon filled with the given color.
     */
    static QIcon makeColorIcon( const QColor& color );

    /**
     * Creates a 16x16 pixel QIcon filled with a GSLib color given its code.
     */
    static QIcon makeGSLibColorIcon(uint color_code);

    /**
      * Returns a QColor given a GSLib color code.
      */
    static QColor getGSLibColor(uint color_code);

    /** Returns the highest GSLib color code.
     * Lowest is 1 (red).
     */
    static uint getMaxGSLibColorCode();

    /**
      * Returns the name of a GSLib color given its code.
      */
    static QString getGSLibColorName(uint color_code);

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

    /**
     * Returns the number of file lines that make up the header of the given GEO-EAS file.
     */
    static uint getHeaderLineCount(QString file_path);

    /**
      * Returns the first line of a GEO-EAS file (the file description).
      */
    static QString getGEOEAScomment(QString file_path);

    /**
     * Creates a widget with the appearance of a horizontal line, normally used as a
     * separator.
     */
    static QFrame *createHorizontalLine();

    /**
     * Creates a widget with the appearance of a vartical line, normally used as a
     * separator.
     */
    static QFrame *createVerticalLine();

    /** Returns whether the given set of variograms form a Linear Model of
     * Coregionalization.
     *  Any problems encountered are reported to the main window's message panel as error
     * messages
     *  so the user can take corrective measures.
     *  @param vmVar1 Autovariogram of 1st variable.
     *  @param vmVar2 Autovariogram of 2nd variable.
     *  @param crossVariogram Cross variogram between the variables (no lag effect
     * assumed).
     */
    static bool isLMC(VariogramModel *vmVar1, VariogramModel *vmVar2,
                      VariogramModel *crossVariogram);

    /** Saves the given list of strings as lines in the given text file. */
    static void saveText(const QString filePath, const QStringList lines);

    /** Computes FFT (forward or reverse) for a vector of values.  The result will be
     * stored in the input array.
     *  This is a C++ port from the original Fortran implementation by Jon Claerbout
     * (1985).
     *  @note The array elements are OVERWRITTEN during computation.
     *  @note This is a WIP.  Not currently working accurately, probably by something
     * misinterpreted from the
     *        Fortran code.  Help with this will be much appreciated.  Original Fortran
     * code is in the .cpp.
     *        function body as a series of comments.
     *  @param lx Number of elements in values array.
     *  @param cx Input/output vector of values (complex numbers).
     *  @param startingElement Position in cx considered as 1st element (pass zero if the
     * array is unidimensional).
     *  @param isig 0 or 1 to transform or back-transform respectively.
     */
    static void fft1D(int lx, std::vector<std::complex<double>> &cx, int startingElement,
                      FFTComputationMode isig);

    /**
     *  Slight modification to use the more portable STL's complex type and be called from
     * an FFT 2D routine
     *  from the modification of Paul Bourkes FFT code by Peter Cusack
     *  to utilise the Microsoft complex type.
     *
     *  This computes an in-place complex-to-complex FFT
     *  dir =  1 gives forward transform
     *  dir = -1 gives reverse transform
     *
     *  @param m log2(number of cells). Number of cells should be 4, 16, 64, etc...
     *  @note WIP: This function is not compatible with fft2D().
     */
    static void fft1DPPP(int dir, long m, std::vector<std::complex<double>> &x,
                         long startingElement);

    /** Computes 2D FFT (forward or reverse) for an array of values.  The result will be
     * stored in the input array.
     *  This is a C++ port from the original Fortran implementation by M.Pirttijärvi
     * (2003).
     *  @note The array elements are OVERWRITTEN during computation.
     *  @note The array should be created by making a[nI*nJ*nK] and not a[nI][nJ][nK] to
     * preserve memory locality (maximize cache hits)
     *  @param n1 Number of elements in X/I direction.
     *  @param n2 Number of elements in Y/J direction.
     *  @param cx Input/output array of values (complex numbers).
     *  @param isig 0 or 1 to transform or back-transform respectively.
     */
    static void fft2D(int n1, int n2, std::vector<std::complex<double>> &cp,
                      FFTComputationMode isig);

    /** Split function specialized to tokenize data lines of GEO-EAS files.
     *  @note This is not a generic tokenizer, so do not use for other applications.
     *        Use tokenizeWithQuotes() for generic tokenization (slower).
     */
	static void fastSplit(const QString lineGEOEAS, QStringList& list);

    /**
     * Tokenizes a line of text using blank spaces or tabulation characters as separator.
     * Text enclosed in double quotes are kept as one token.
     * @param includeDoubleQuotes If true, tokens delimited by double quotes are kept with them.
     */
    static std::vector<std::string> tokenizeWithDoubleQuotes(const std::string& lineOfText, bool includeDoubleQuotes);

    /** Enclose in double quotes if the input text has whitespaces. */
    static QString putDoubleQuotesIfThereIsWhiteSpace( const QString& text );

    /** Computes 3D FFT (forward or reverse) for an array of values.  The result will be
     * stored in the input array.
     *  @note The array elements are OVERWRITTEN during computation.
     *  @note The array should be created by making a[nI*nJ*nK] and not a[nI][nJ][nK] to
     * preserve memory locality (maximize cache hits)
     *  @param nI Number of elements in X/I direction.
     *  @param nJ Number of elements in Y/J direction.
     *  @param nK Number of elements in Z/K direction.
     *  @param values Input/output array of values (complex numbers).
     *  @param isig 0 or 1 to transform or back-transform respectively.
     *  @param itype Image type.  Either direct real and imaginary components or the
     * complex numbers are in polar form (magnitude and angle).
     */
    static void fft3D(int nI, int nJ, int nK, std::vector<std::complex<double>> &values,
                      FFTComputationMode isig,
                      FFTImageType itype = FFTImageType::RECTANGULAR_FORM);

    /** Compute the dip angle corresponding to grid steps.
     * the d* parameters are the grid cell sizes.
     * The returned angle is in degrees and follow the GSLib convention.
     */
    static double getDip(double dx, double dy, double dz, int xstep, int ystep,
                         int zstep);

    /** Compute the azimuth angle corresponding to grid steps.
     * the d* parameters are the grid cell sizes.
     * The returned angle is in degrees and follow the GSLib convention.
     */
    static double getAzimuth(double dx, double dy, int xstep, int ystep);

    /** Returns decibels of an input value with respect to a reference value.
     * Examples.: dBm is defined as the decibel level of value in milliwatts with respect
     * to 1mW.
     *            dBi is defined as the gain in decibels of an antenna with respect to the
     * ideal isotropic antenna.
     * refLevel cannot be zero or a divison by zero error ensues.
     * epsilon The smallest allowed absolute value as to avoid large negative results or
     * even -inf (value = 0.0).
     */
    static double dB(double value, double refLevel, double epsilon);

    /** Returns a text containing a better human readable value.
     * E.g.: 25000000 results in "25M".  Client code can then append a basic unit: "25M" +
     * "Hz" to make "25MHz".
     * This function supports multiplier suffixes from pico (p) to Exa (E).  More can be
     * added with small changes
     * to its code.
     */
    static QString humanReadable(double value);

    /** Tests whether the diven 2D location lies within the given 2D bounding box. */
    static bool isWithinBBox(double x, double y, double minX, double minY, double maxX,
                             double maxY);

    /** Sets the given min/max values, unless the input values are equal.
     * In this case, a small window is set around the values.
     */
    static void assureNonZeroWindow(double &outMin, double &outMax, double inMin,
                                    double inMax,
                                    double minWindowPercent = 0.01); // 0.01 == 1%

    /**
     * Runs the GSLib program histplt and opens the plot dialog to view the histogram of
     * variable.
     * @param parent Parent QWidget for the plot dialog.
     * @param modal If true, the method returns only when the user closes the Plot Dialog.
     * @return True if modal == true and if the user did not cancel the Plot Dialog; false
     * otherwise.
     */
    static bool viewHistogram(Attribute *at, QWidget *parent = nullptr,
                              bool modal = false);

    /** Returns whether the program was launched with the given argument. */
    static bool programWasCalledWithCommandLineArgument(QString argument);

    /** Returns the ammount of physical RAM used by the program in bytes.
     * Returns a negative value if is not possible to get the value or if the current code
     * does not support the OS (see util.cpp).
     */
    static std::int64_t getPhysicalRAMusage();

    /** Returns the filename (e.g. drillholes.txt) from a path. */
    static QString getFileName(QString path);

    /**
      * Returns the name of a variographic structure given its code following GSLib convention.
      */
    static QString getGSLibVariogramStructureName(uint it);

	/** Thests whether the passed point (as Vertex3D) is inside the polyhedron (as a vector of Face3D).
     * This assumes the polyhedron is convex and the faces are all counter-clockwise as seen from outside.
     * source: https://stackoverflow.com/questions/8877872/determining-if-a-point-is-inside-a-polyhedron
	 */
	static bool isInside( const Vertex3D& p, const std::vector<Face3D>& fs );

    /**
     * Computes the varmap using the FFT method for fast result.
     * It is based on the the Fourier Integral Method (Pardo-Iguzquiza & Chica-Olmo, 1993).
     * Review results if the cells are not squares/cubes.
     */
    static spectral::array getVarmapFIM( const spectral::array& inputData );

    /**
     * Computes the varmap using the spectral::autocovariance() method for fast result.
     * Review results if the cells are not squares/cubes.
     */
    static spectral::array getVarmapSpectral( const spectral::array& inputData );

    /**
     * Computes the varmap with either getVarmapFFT() or getVarmapSpectral().
     * Calling this method will ask the user to choose.
     */
    static spectral::array getVarmap( const spectral::array& inputData );

    /** Converts an angle in degrees of azimuth (0 == north and increasing clockwise)
     * to radians in trigonometric convention (0 == right and increasing counter-clockwise).
     */
    static double azimuthToRadians( double azimuth );

    /** Converts an angle in radians (0 == right and increasing counter-clockwise by default)
     * to half azimuth degrees (0 == north and increasing clockwise).
     * Half-azimuth means that an azimuth greater than 180 is recomputed as az - 180.  E.g. N240E becomes N060E
     */
    static double radiansToHalfAzimuth( double trigonometricAngle, bool clockwiseRadians = false );

    /** Formats a double value as a string with a given number of decimal places. */
    static QString formatToDecimalPlaces( double value, int nDecimalPlaces );

    /** Divides a given range of values into consecutive sub-ranges as evenly as possible.
     * This is mostly useful to distribute a former for...loop's range of indexes amongst a
     * number of threads for parallelization.
     * Returns a vector of pairs.  The first member of each pair is the min and the second, the max.
     */
    static std::vector< std::pair< int, int > > generateSubRanges(int mainRangeMin, int mainRangeMax, int numberOfSubRanges);
    
    /**
     * Replaces every occurrences of facies names/symbols in the given file with the respective
     * facies numerical codes.
     * @param path The path to the input file.
     * @param cd The CategoryDefinition object to use.
     * @param useMainNames If true, the function searches for the texts saved in the main name field of the CategoricalDefinition.
     *                     Otherwise, it uses the alternate name field.
     * @param saveTo The path to the output file.
     * @return True if the function completes successfully.  False if something goes wrong (e.g. file does not exist).
     */
    static bool replaceFaciesNamesWithCodes( QString path,
                                             CategoryDefinition* cd,
                                             bool useMainNames,
                                             QString saveTo );

    /**
     * Tests wheter the string is one of those in the list.
     * This method is designed for comparison against a short list of constant strings.
     * For brevity, it is suggested to call this function like this:
     *    Util::isIn( droidName, {"C3PO", "R2D2", "BB8"} );
     */
    static bool isIn( const QString& stringToTest, const QStringList& listOfValues );

    /**
     * Returns the color mapped from the given value according to
     * the passed color table.
     */
    static QColor getColorFromValue(double value, ColorTable colorTableToUse, double min = 0.0, double max = 1.0);

    /**
     * Returns a string in the format "#RRGGBB" mapped from the given value according to
     * the passed color table.
     */
    static QString getHTMLColorFromValue(double value, ColorTable colorTableToUse, double min = 0.0, double max = 1.0);

    /** Returns the probability of a given value according to the Chi-Squared Distribution with the given degrees of freedom. */
    static double chiSquared( double x, int degreesOfFreedom );

    /** Returns the value of x of chiSquares() whose area under the chi-squared distribution (see chiSquared) to its right
     * corresponds to the value passed as the significanceLevel parameter.
     * The returned value is equivalent to the one that would be manually obtained with chi-square tables commonly used.
     * @param step Step size used to compute the area under the curve.
     */
    static double chiSquaredAreaToTheRight(double significanceLevel, int degreesOfFreedom, double step );

    /** Returns whether the given color is dark.
     * This function computes the luminance of the color according to the ITU-R recommendation BT.709,
     * then it judges whether it is dark accoring to a threshold per W3C Recommendations.
     */
    static bool isDark( const QColor& color );

    /**
     * Returns a color that is contrasting with respect the input color.
     * This is useful to set, for instance, bright letters over a dark barkground.
     */
    static QColor makeContrast( const QColor& color );

    /**
     * Makes a <font color='#nnnnnn'>text</font> HTML tag so that the text has constrasting letters against
     * the given background color.
     */
    static QString fontColorTag( const QString& text, const QColor& bgcolor );

    /**
     * Computes a series of Facies Transition Matrices for different separations (h) in space.
     * @param categoricalAttributes The list with categorical attributes to compute FTMs for.
     * @param hInitial The initial separation (e.g. 1m)
     * @param hFinal The final separation (e.g. 30m)
     * @param nSteps The number of separations between initial and final separations (e.g. 15).
     * @param toleranceCoefficient The tolerance to be used for spatial searches (useful for point sets or other
     *                             data sets with sparse small support).
     */
    static std::vector<hFTM> computeFaciesTransitionMatrices( std::vector<Attribute *> &categoricalAttributes,
                                                              double hInitial,
                                                              double hFinal,
                                                              int nSteps,
                                                              double toleranceCoefficient );

    /**
     *  Removes zero-only columns and rows from the passed Facies Transion Matrices.
     *  This action is done such that all matrices in the list have columns
     *  and rows refering to the same facies.
     */
    static void compressFaciesTransitionMatrices( std::vector<hFTM>& hFTMs );

    /**
     * Plots the Facies Relationship Diagram for the passed Facies Transition Matrix.
     * @param tmpPostscriptFilePath Output parameter: the path to the resulted Postscript temporary file.
     * @param cutoff Probability values below this value are not plotted.
     * @param makeLinesProportionalToProbabilities The thickness of the lines in the graph are proportional to the transition probabilities.
     * @param numberOfDecimalDigits The number of decimal places in the labels of the graph.
     * @param maxLineThickness The maximum line thickness to be used if makeLinesProportionalToProbabilities is true.
     */
    static void makeFaciesRelationShipDiagramPlot( const FaciesTransitionMatrix &faciesTransitionMatrix,
                                                   QString &tmpPostscriptFilePath,
                                                   double cutoff,
                                                   bool makeLinesProportionalToProbabilities ,
                                                   int numberOfDecimalDigits,
                                                   int maxLineThickness );

    /**
     * Sorts (in-place) the given data table by the given data column.  The outer vector are each row
     * and the inner vector are each column. This function assumes the existence of the n-th data column
     * in all rows, so passing an irregular data table may result in a crash.
     */
    static void sortDataFrame(std::vector< std::vector < double > >& df, uint dataColumn, SortingOrder sortingOrder );

    /**
     * Tells whether two lines, giving their head and tail XYZ coordinates, are connected somehow.
     * The function also returns true if the lines are conneced at both ends (coincident).
     */
    static bool areConnected( double line1HeadX, double line1HeadY, double line1HeadZ,
                              double line1TailX, double line1TailY, double line1TailZ,
                              double line2HeadX, double line2HeadY, double line2HeadZ,
                              double line2TailX, double line2TailY, double line2TailZ,
                              double tolerance = 0.000001 );

    /** Converts a vector of doubles as space-separated string. */
    static QString dumpDataLine( const std::vector<double>& dataLine );

    /** Performs a linear interpolation: y = y0 + ( y1 - y0 ) * ( x - x0 ) / ( x1 - x0 ); */
    inline static double linearInterpolation( double x, double x0, double x1, double y0, double y1 )
                   { return y0 + ( y1 - y0 ) * ( x - x0 ) / ( x1 - x0 ); }

    /** If z1 < z0, returns x1, y1, z1; x0, y0, z0 otherwise. */
    static void getBaseCoordinate( double x0, double y0, double z0,
                                   double x1, double y1, double z1,
                                   double& x, double& y, double& z);

    /** If z1 > z0, returns x1, y1, z1; x0, y0, z0 otherwise. */
    static void getTopCoordinate(  double x0, double y0, double z0,
                                   double x1, double y1, double z1,
                                   double& x, double& y, double& z);

    /** Format the given list of strings as single-line string with the given separator. **/
    static QString formatAsSingleLine( QStringList list, QString separator = " " );

    /** Rescale the values so they sum up to 1.0. Example: 70, 10, 20 -> 0.7, 0.1, 0.2.
     * This is an in-place operation, so if you need to keep the original values, then you need
     * to make a copy of the vector prior to calling this method.
     * @warning Does not work with negative values! For this, one needs a softmax function.
     */
    static void unitize( std::vector<double>& values );

    /** Returns the Unix-style timestamp that is the number of milliseconds
     * since 00:00 UTC, Jan 1st, 1970.
     */
    static long long getUnixTimeStamp();

    /** Returns whether the file exists, given its path. */
    static bool fileExists( QString path );

    /** Returns the parent directory of the given path. */
    static QString getParentDirectory( QString path );

    /** Returns 1 for a positive number, -1 for a negative one and
     * 0 for zero values (implements signum()).
     * @note Triggers the compiler warning -Wtype-limits if type of test value
     *       is unsigned ( unsigned is always positive and the test for < 0 does not make sense).
     */
    template <typename T>
    static int sign(T value) {
        return (T(0) < value) - (value < T(0));
    }

    /**
     * Computes new values proportional to the original ones such that they are
     * in the [0.0 1.0] range and sum up to 1.0.  This computation is done in-place.
     */
    static void softmax( std::vector<double>& in_out );

    /**
     * Prints a vector's contents to std::out.  This is usually called for debugging
     * purposes.
     */
    template <typename T>
    static int print( const std::vector<T>& vec ) {
        for (const auto value: vec)
          std::cout << value << ' ';
        std::cout << '\n';
    }

    /**
     * Does a find-and-replace in a text file.  The search string must
     * be a regular expression.
     * @returns The number of file lines where at least one replacement took place.
     */
    static int findAndReplace( const QString text_file_path,
                               const QString RE_find_what,
                               const QString replace_with );

    /**
     * Swaps the passed values if mustBeTheSmaller > mustBeTheGreater.
     */
    static void ensureAscending( double& mustBeTheSmaller,
                                 double& mustBeTheGreater );

    /**
     * Prints the given number with the given number of leading zeroes.
     */
    template <typename T>
    static QString zeroPad( T value, uint nZeroes ){
        //nZeroes is the number in printf("%05d") equivalent (value == 5)
        //10 is the radix, 16 prints the number in hex.
        return QStringLiteral("%1").arg(value, nZeroes, 10, QLatin1Char('0'));
    }

    /**
     * Adds the values stored in the array of doubles as a new column in a GEO-EAS data file.
     * The new column is appended to the end of each of this file's lines.
     * No check is done whether the array has the same value count as data line count in the file.
     * If the array has more values than data lines, the excess values will be ignored.  If there are less values,
     * the remaining data file lines will be filled with the given no-data value.
     */
    static void appendPhysicalGEOEASColumn( const spectral::arrayPtr data,
                                            const QString variable_name,
                                            const QString file_path,
                                            const QString NDV = "-999999" );

    /**
     * Parses a text file consisting of lines with VARIABLE=VALUE pairs.
     * Lines not following this format are ignored.
     * @return A vector whose elements are pairs of strings: 1st element is the variable name
     *         and 2nd element is the value.
     * @see getConfigurationValue()
     */
    static std::vector< std::pair<QString, QString> > parseConfigurationFile( const QString path );

    /**
     * Returns the value set for a given variable in a vector of pairs of VARIABLE-VALUE.
     * @param configs The vector containing the configurations.  The 1st in the pairs are the variable names.
     *                The 2nd in the pairs are the variable values.
     * @param variable The configuration variable to search for.
     * @return The configuration value.  It is empty whether the variable is undefined or is blank.
     * @see parseConfigurationFile()
     * @note The search is case-sensitive.
     * @note The search is O(n) and involves string comparisons, so the computational cost of this method is high.
     *       Hence, do not use this with large vectors.  This is intended for configurations sets with 1000s
     *       configurations at most.
     * @note If there is more than one variable with the same name, the value of the first one is returned.
     */
    static QString getConfigurationValue( const std::vector< std::pair<QString, QString> >& configs,
                                          const QString variable );

};

#endif // UTIL_H
