#ifndef __VVVV___FILE_H
#define __VVVV___FILE_H
#include "projectcomponent.h"
#include <QString>
#include <QIcon>
#include <vector>
#include "exceptions/invalidmethodexception.h"

/**
 * @brief The File class is the base class of all project components that are
 *  files (PointSet, Cartesian Grid, etc.)
 */
class File : public ProjectComponent
{
public:
    File( QString path );
    /** Returns the file name without path. */
	QString getFileName() const;
    /** Copies the file to given directory and
     * changes the _path member variable accordingly.
     */
    void changeDir( QString dir_path );

    /** Renames the file and
     * changes the _path member variable accordingly.
     * The new filename must include the extension.
     * If there is a file with the same name, overwrites.
     */
    void rename( QString new_name );

    /** Deletes the file from filesystem. */
    virtual void deleteFromFS();

    /** Returns the full path to the file. */
    virtual QString getPath() const;

    /** Returns whether the file type may have a companion metadata file. */
    virtual bool canHaveMetaData() = 0;

    /** Returns the path to the metadata file. */
    QString getMetaDataFilePath();

    /** Returns a string with the file type (e.g.: "POINTSET") */
    virtual QString getFileType() const = 0;

    /** Updates the physical metadata file, if applicable.
      * Subclasses without metadata can implement an empty method.*/
    virtual void updateMetaDataFile() = 0;

    /** Changes the value of _path member variable. It doesn't do any file operation.
     *  It will, consequently, affect the behavior of data I/O functions that may exist in File implementations.
     * To actually change the file's path, consider the rename() and changeDir() methods.*/
    void setPath( QString path );

    /** Saves the object's data to the file system given by the _path member variable. */
    virtual void writeToFS(){}

    /** Loads the object's data from the file system given by the _path member variable. */
    virtual void readFromFS(){}

    /** Clears any content loaded from the pysical file.  Mind any unsaved changes, calling
     * writeToFS() to make sure they are saved.*/
    virtual void clearLoadedContents(){}

    /** Returns weather there is actually a physical file in the path given by the _path member variable.*/
    bool exists();

    /** Returns weather the file can be edited using GammaRay tools. */
    virtual bool isEditable(){ return false; }

    /** Returns the number of content elements such as text lines, file records, tags, etc.
     */
    virtual long getContentsCount(){ return 0; }

    /**
     * Creates a widget adequate to edit a content element (e.g. file record).
     */
    virtual QWidget* createContentElementWidget() { return nullptr; }

    /**
     * Creates a widget filled with a content element (e.g. file record) given by its index.
     */
    virtual QWidget* createWidgetFilledWithContentElement( uint /*iContent*/ ) { return nullptr; }

    /**
     * Add content element (e.g. file record) using user-entered data in the given widget.
     */
    virtual void addContentElementFromWidget( QWidget* /*w*/ ){ throw InvalidMethodException(); }

    /**
      *  Returns whether this file as a data file.
      */
    virtual bool isDataFile() = 0;

    /**
      *  Returns the file size in bytes.  Returns -1 if file does not exist.
      */
    virtual qint64 getFileSize();

	/**
	  *  Returns whether this file as a distribution.
	  */
	virtual bool isDistribution() = 0;

    /**
     * Duplicates all file system files associated with this File (data and metadata files).  The paths to them can be
     * retrived via the returned new File object.
     * @note This is not a clone operation, that is, in-memory member values are not copied.  They must be repopulated
     *       with a call to readFromFS().
     * @note The returned File object is not added to the Project tree automatically.
     * @attention Classes that implement this method should receive treatment in the Project::addFile() method.  Failing to do
     *            so results in a file copy not being added to the project tree with an error message.
     */
    virtual File* duplicatePhysicalFiles( const QString new_file_name ) = 0;

protected:
    QString _path;

    /**
     * A convenience method for classes overriding duplicatePhysicalFiles().
     * The method duplicates the main data file as well as the associated metadata file (.md) if they exist.
     * @returns The complete path to the copied data file (the metadata file path is the same, but with
     *          the extension .md appended).
     */
    QString duplicateDataAndMetaDataFiles( const QString new_file_name );

    // ProjectComponent interface
public:
	virtual QString getName() const;
    virtual QIcon getIcon() = 0;
    virtual bool isFile() const;
	virtual bool isAttribute();
    virtual QString getObjectLocator();
    virtual QString getTypeName() const { return getFileType(); }
};

#endif // __VVVV___FILE_H
