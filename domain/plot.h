#ifndef PLOT_H
#define PLOT_H

#include "file.h"

/**
 * @brief The Plot class represents a plot file saved by the user.
 */
class Plot : public File
{
public:
    Plot(QString path);

// File interface
public:
    QString getFileType(){ return "PLOT"; }
    virtual bool canHaveMetaData(){ return false; }
    virtual void updateMetaDataFile(){;}
    bool isDataFile(){ return false; }
	bool isDistribution(){ return false; }

// ProjectComponent interface
public:
    QIcon getIcon();
    void save(QTextStream *txt_stream);
};

#endif // PLOT_H
