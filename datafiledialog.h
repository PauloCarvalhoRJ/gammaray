#ifndef DATAFILEDIALOG_H
#define DATAFILEDIALOG_H

#include <QDialog>

namespace Ui {
class DataFileDialog;
}

class DataFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataFileDialog(QWidget *parent = 0, const QString file_path = "");
    ~DataFileDialog();

    enum { UNDEFINED, CARTESIANGRID, POINTSET} iDataFileType;

    int getDataFileType();

public slots:
    void accept();

private:
    Ui::DataFileDialog *ui;
    int _file_type;
};

#endif // DATAFILEDIALOG_H
