#ifndef SEGMENTSETDIALOG_H
#define SEGMENTSETDIALOG_H

#include <QDialog>

namespace Ui {
class SegmentSetDialog;
}

class SegmentSetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SegmentSetDialog(QWidget *parent = nullptr, const QString file_path = "");
    ~SegmentSetDialog();

public slots:
    //indexes in GSLib format start with 1, not 0 (zero).
    int getXIniFieldIndex();
    int getYIniFieldIndex();
    int getZIniFieldIndex();
    int getXFinFieldIndex();
    int getYFinFieldIndex();
    int getZFinFieldIndex();
    QString getNoDataValue();

private:
    Ui::SegmentSetDialog *ui;
    QString _file_path;
};

#endif // SEGMENTSETDIALOG_H
