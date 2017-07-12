#ifndef DATALOADER_H
#define DATALOADER_H

#include <QObject>
#include <QFile>

/** This is an auxiliary class used in DataFile::loadData() to enable the progress dialog.
 * The file is read in a separate thread, so the progress bar updates.
 */
class DataLoader : public QObject
{
    Q_OBJECT

public:
    explicit DataLoader(QFile &file,
                        std::vector< std::vector<double> > &data,
                        uint &data_line_count,
                        ulong firstDataLineToRead,
                        ulong lastDataLineToRead,
                        QObject *parent = 0);

    bool isFinished(){ return _finished; }

public slots:
    void doLoad( );
signals:
    void progress(int);

private:
    QFile &_file;
    std::vector< std::vector<double> > &_data;
    uint &_data_line_count;
    bool _finished;
    ulong _firstDataLineToRead;
    ulong _lastDataLineToRead;
};

#endif // DATALOADER_H
