#ifndef VARIABLEREMOVER_H
#define VARIABLEREMOVER_H

#include <QObject>

class DataFile;

/** This is an auxiliary class used in DataFile::deleteVariable() to enable the progress dialog.
 * The file is modified in a separate thread, so the progress bar updates.
 */
class VariableRemover : public QObject
{

    Q_OBJECT

public:
    explicit VariableRemover(DataFile &dataFile, uint columnToDelete, QObject *parent = 0);

    bool isFinished(){ return _finished; }

public slots:
    void doRemove( );
signals:
    void progress(int);

private:
    bool _finished;
    DataFile &_dataFile;
    uint _columnToDelete;
};

#endif // VARIABLEREMOVER_H
