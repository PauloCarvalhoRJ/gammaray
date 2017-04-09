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
    QString getFileName();
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
    virtual QString getPath();

    /** Returns whether the file type may have a companion metadata file. */
    virtual bool canHaveMetaData() = 0;

    /** Returns the path to the metadata file. */
    QString getMetaDataFilePath();

    /** Returns a string with the file type (e.g.: "POINTSET") */
    virtual QString getFileType() = 0;

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

protected:
    QString _path;

    // ProjectComponent interface
public:
    QString getName();
    virtual QIcon getIcon() = 0;
    bool isFile();
    bool isAttribute();
};

#endif // __VVVV___FILE_H
