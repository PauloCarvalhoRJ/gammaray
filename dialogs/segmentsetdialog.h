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
    explicit SegmentSetDialog(QWidget *parent = nullptr);
    ~SegmentSetDialog();

private:
    Ui::SegmentSetDialog *ui;
};

#endif // SEGMENTSETDIALOG_H
