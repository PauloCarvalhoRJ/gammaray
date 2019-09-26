#ifndef PROJECT_H
#define PROJECT_H

#include <QString>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QModelIndexList>
#include "domain/roles.h"

class QDir;
class ObjectGroup;
class DataFile;
class ProjectRoot;
class File;
class Plot;
class CartesianGrid;
class ExperimentalVariogram;
class VariogramModel;
class Distribution;
class ThresholdCDF;
class CategoryPDF;
class ProjectComponent;
class IJAbstractCartesianGrid;
class VerticalTransiogramModel;

/**
 * @brief The Project class holds all information about a geostats study.
 */
class Project : public QAbstractItemModel
{
public:
    Project( const QString path );
    ~Project();

    /**
     * @brief Creates or updates the project data file, which is file called gammaray.prj
     * in the project root directory.
     */
    void save();

    /** Returns the name of the project.  Currently it is just the name of the project
    root directory without the path.*/
    QString getName();

    ObjectGroup* getDataFilesGroup();
    ObjectGroup* getVariogramsGroup();
    ObjectGroup* getDistributionsGroup();
    ObjectGroup* getResourcesGroup();

    /** Returns the path to the project directory. */
    QString getPath();

    /** Adds the given data file object to this project. */
    void addDataFile( DataFile *df );

    /** Adds the given plot object to this project. */
    void addPlot( Plot *plot );

    /** Adds the given experimental variogram object to this project. */
    void addExperimentalVariogram( ExperimentalVariogram *exp_var );

    /** Adds the given variogram model object to this project. */
    void addVariogramModel(VariogramModel *var_model );

    /** Adds the given distribution object to this project. */
    void addDistribution( Distribution *dist );

    /** Adds the given threshold c.d.f. object to this project. */
    void addThresholdCDF( ThresholdCDF *tcdf );

    /** Adds the given category p.d.f. object to this project. */
    void addCategoryPDF( CategoryPDF *cpdf );

    /** Adds the given vertical transiogram model object to this project. */
    void addVerticalTransiogramModel( VerticalTransiogramModel *vtm );

    /** Adds the given generic file object as a resource. */
    void addResourceFile( File *file );

    /** Creates a new plot object given path to a plot file and its new name once
     * in the project.  The file is copied into the project's directory, renamed
     * and registered in the project's metadata file.  from_path must be a complete
     * path, including the original file name. new_file_name must include extension.
     */
    void importPlot( const QString from_path, const QString new_file_name );

    /** Creates a new cartesian grid object given the CartesianGrid pointer of an object created from an
     * external cartesian grid file and its new name once
     * in the project.  The file is copied into the project's directory, renamed
     * and registered in the project's metadata file.  cg must contain all grid metadata
     * as well as the original source path, including the original file name. new_file_name must include extension.
     * @note A new object is created, so it is up to the client code the management of the passed pointer.
     * @return The pointer to the final Cartesian grid object.
     */
    CartesianGrid* importCartesianGrid( CartesianGrid* cg, const QString new_file_name );

    /** Creates a new experimental variogram object given path to the file and its new name once
     * in the project.  The file is copied into the project's directory, renamed
     * and registered in the project's metadata file.  from_path must be a complete
     * path, including the original file name. new_file_name must include extension.
     * @param vargplt_par_file The path to a vargplt parameter file so the user can view the experimental
     *                         variogram again later without redoing the variogram analysis.
     * @note This method DOES NOT copy the file specified by vargplt_par_file to the project directory.
     *       If the vargplt_par_file file must reside in the project directory, it is necessary to copy
     *       it beforehand, by calling Util::copyFile() for example.
     */
    void importExperimentalVariogram(const QString from_path,
                                     const QString new_file_name ,
                                     const QString vargplt_par_file);

    /** Creates a new variogram model object given path to the a vmodel parameter file and its new name once
     * in the project.  The file is copied into the project's directory, renamed
     * and registered in the project's metadata file.  from_path must be a complete
     * path, including the original file name. new_file_name must include extension.
     */
    void importVariogramModel( const QString from_path, const QString new_file_name );

    /** Creates a new univariate distribution object given path to a
     * histsmth output file and its new name once
     * in the project.  The file is copied into the project's directory, renamed
     * and registered in the project's metadata file.  from_path must be a complete
     * path, including the original file name. new_file_name must include extension.
     * @param roles A collection of distribution file roles (e.g. probability values) for the columns.
     * This list can be empty.
     */
    void importUnivariateDistribution(const QString from_path, const QString new_file_name , const QMap<uint, Roles::DistributionColumnRole> &roles);

    /** Creates a new bivariate distribution object given path to a
     * scatsmth output file and its new name once
     * in the project.  The file is copied into the project's directory, renamed
     * and registered in the project's metadata file.  from_path must be a complete
     * path, including the original file name. new_file_name must include extension.
     * @param roles A collection of distribution file roles (e.g. probability values) for the columns.
     * This list can be empty.
     */
    void importBivariateDistribution(const QString from_path, const QString new_file_name , const QMap<uint, Roles::DistributionColumnRole> &roles);

    /** Creates a new facies transition matrix object given a path to a
     * correctly formatted file and its new name once
     * in the project.  The file is copied into the project's directory, renamed
     * and registered in the project's metadata file.  from_path must be a complete
     * path, including the original file name. new_file_name must include extension.
     * @param associatedCategoryDefinitionName The name of an existing CategoryDefinition object whose category names match the names in the
     *                                         file.  Pass an empty string if you do not specify a category definition (may hinder some functionalities).
     */
    void importFaciesTransitionMatrix( const QString from_path, const QString new_file_name, QString associatedCategoryDefinitionName );

    /** Creates a new threshold c.d.f. object from a file assumed to be already present in the project's directory.
     * If the object's path is elsewhere, then the project may contain an orphan reference if either the project
     * or the referenced file is moved.
     */
    void registerThresholdCDF( ThresholdCDF* tcdf );

    /** Creates a new category p.d.f. object from a file assumed to be already present in the project's directory.
     * If the object's path is elsewhere, then the project may contain an orphan reference if either the project
     * or the referenced file is moved.
     */
    void registerCategoryPDF( CategoryPDF* cpdf );

    /** Creates a new vertical transiogram model object from a file assumed to be already present in the project's directory.
     * If the object's path is elsewhere, then the project may contain an orphan reference if either the project
     * or the referenced file is moved.
     */
    void registerVerticalTransiogramModel( VerticalTransiogramModel* vtm );

    /** Creates a new generic file object from a file assumed to be already present in the project's directory.
     * If the object's path is elsewhere, then the project may contain an orphan reference if either the project
     * or the referenced file is moved.
     */
    void registerFileAsResource( File* file );

    /** Returns a list of tree model indexes, which are references to the
     * objects references in the tree widget */
    QModelIndexList getPersistentIndexList();

    /** Returns a list of all items under the given parent item.
     * This method is recursive.  Pass QModelIndex() to return all intems.
     */
    QModelIndexList getIndexList(const QModelIndex &parent);

    /**
     * @brief Removes de file from project.
     * @param delete_the_file If true, the file is also physically deleted from file system.
     */
    void removeFile(File* file, bool delete_the_file);

    /**
     * @brief Returns the path to the parameter file templates.
     */
    QString getTemplatesPath();

    /**
     * Makes the complete path to a parameter file template given the program name.
     * It is merely a string construction based on project information, not meaning that the file and/or directories exist.
     */
    QString getTemplatePath( const QString program_name );

    /**
     * @brief Returns the path to the temporary files directory.
     */
    QString getTmpPath();

    /**
     * @brief Generates a unique filename for the project's temporary files directory with the given extension.
     * This is just a string construction based on the directory contents.  The file is not created.
     */
    QString generateUniqueTmpFilePath( const QString file_extension );

    /**
     * Tests whether there is a file with the given name in the project directory.  File name must incude extension.
     * @note In Windows, data.dat and DATA.DAT, for example, are considered the same file.
     */
    bool fileExists( const QString filename );

    /**
     * Returns whether the given File object is a child in the project tree.
     */
    bool fileIsChild( File* file );

    /**
     * Searches through the object tree for an object corresponding to the given
     * object locator.
     */
    ProjectComponent* findObject( const QString object_locator );

    /** Calls DataFile::freeLoadedData() on all objects of type DataFile (or of its subclasses) in the project.
     */
    void freeLoadedData();

	/** Returns a collection with the Cartesian grids of the project. */
	std::vector<IJAbstractCartesianGrid *> getAllCartesianGrids( );

private:
    QDir* _project_directory;
    ObjectGroup* _data_files;
    ObjectGroup* _variograms;
    ObjectGroup* _distributions;
    ObjectGroup* _plots;
    ObjectGroup* _resources;
    ProjectRoot* _root;

    // QAbstractItemModel interface
public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
};

#endif // PROJECT_H
