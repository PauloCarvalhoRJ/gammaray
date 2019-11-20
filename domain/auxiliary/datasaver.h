#ifndef DATASAVER_H
#define DATASAVER_H

#include <QObject>

/** This is an auxiliary class used in DataFile::writeToFS() to enable the progress dialog.
 * The file is saved in a separate thread, so the progress bar updates.
 */
class DataSaver : public QObject
{
    Q_OBJECT

public:

    explicit DataSaver(std::vector< std::vector<double> >& data,
                       std::ostringstream& out,
                       QObject *parent = nullptr);

    bool isFinished(){ return _finished; }

public slots:
    void doSave( );
signals:
    void progress(int);

private:
    bool _finished;
    std::vector< std::vector<double> >& _data;
    std::ostringstream& _out;

};

#endif // DATASAVER_H
