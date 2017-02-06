#include "plot.h"

#include <QFileInfo>
#include <QTextStream>

Plot::Plot( QString path ) : File( path )
{
}

QIcon Plot::getIcon()
{
    QFileInfo fileInfo( this->_path);
    if(fileInfo.suffix() == "ps")
        return QIcon(":icons/gs16");
    else
        return QIcon(":icons/plot16");
}

void Plot::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
}
