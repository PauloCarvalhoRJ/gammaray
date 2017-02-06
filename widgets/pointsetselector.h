#ifndef POINTSETSELECTOR_H
#define POINTSETSELECTOR_H

#include <QWidget>

namespace Ui {
class PointSetSelector;
}

class DataFile;

//TODO: this class has some in common with CartesianGridSelector, maybe promote them
//      to an abstract superclass (DataFileSelector?).

class PointSetSelector : public QWidget
{
    Q_OBJECT

public:
    explicit PointSetSelector(QWidget *parent = 0);
    ~PointSetSelector();

    /** Returns null pointer if no file is selected. */
    DataFile* getSelectedDataFile(){ return m_dataFile; }

signals:
    void pointSetSelected( DataFile* ps );

private:
    Ui::PointSetSelector *ui;
    DataFile* m_dataFile;

public slots:
    void onSelection( int index );
};

#endif // POINTSETSELECTOR_H
