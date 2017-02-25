#include "plot.h"

#include <QFileInfo>
#include <QTextStream>
#include "util.h"

Plot::Plot( QString path ) : File( path )
{
}

QIcon Plot::getIcon()
{
    QFileInfo fileInfo( this->_path);
    if(fileInfo.suffix() == "ps"){
        if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
            return QIcon(":icons/gs16");
        else
            return QIcon(":icons32/gs32");
    }else{
        if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
            return QIcon(":icons/plot16");
        else
            return QIcon(":icons32/plot32");
    }
}

void Plot::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
}
