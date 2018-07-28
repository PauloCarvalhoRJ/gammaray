#include "file.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>

File::File(QString path)
{
    this->_path = path;
}

QString File::getFileName() const
{
    QFile file( this->_path );
    QFileInfo fileInfo(file.fileName());
    return fileInfo.fileName();

}

void File::changeDir(QString dir_path)
{
    //make object representing destination directory
    QDir dest_dir( dir_path );
    //make object representing current file
    QFile file( this->_path );
    //make object with current file information
    QFileInfo fileInfo( file.fileName() );
    //make object representing the future file
    QFile dest_file( dest_dir.absoluteFilePath( fileInfo.fileName() ) );
    //perform copy
    QFile::copy( file.fileName(), dest_file.fileName() );
    //change path property
    this->_path = dest_file.fileName();
}

void File::rename(QString new_name)
{
    //make a file object with the original name
    QFile file( this->_path );
    //builds a file info object to describe the current file
    QFileInfo original( this->_path );
    //constructs the path with the new file name
    QString newPath = original.canonicalPath() + QDir::separator() + new_name;
    //make a file object with the future name
    QFile dest_file( newPath );
    //if there is a file with the new name
    if( dest_file.exists() )
        //deletes it
        dest_file.remove();
    //perform the renaming
    file.rename( newPath );
    //updates the _path member.
    this->_path = file.fileName();
}

void File::deleteFromFS()
{
    QFile file( this->_path );
    file.remove(); //TODO: throw exception if remove() returns false (fails).  Also see QIODevice::errorString() to see error message.
    //TODO: files may have metadata files (.md) associated with them.  They must also be deleted.
}

QString File::getPath()
{
    return this->_path;
}

QString File::getMetaDataFilePath()
{
    QString md_file_path( this->_path );
    return md_file_path.append(".md");
}

void File::setPath(QString path)
{
    _path = path;
}

bool File::exists()
{
    QFile path( _path );
    return path.exists();
}

long File::getFileSize()
{
    if( ! exists() )
        return -1;
    QFileInfo info( _path );
    return info.size(); //assumes long is 64-bit integer
}


QString File::getName() const
{
    return this->getFileName();
}


bool File::isFile()
{
    return true;
}

bool File::isAttribute()
{
    return false;
}

QString File::getObjectLocator()
{
    return   _parent->getObjectLocator() + '/' + getName();
}
