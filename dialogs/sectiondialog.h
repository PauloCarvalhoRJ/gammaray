#ifndef SECTIONDIALOG_H
#define SECTIONDIALOG_H

#include <QDialog>

namespace Ui {
class SectionDialog;
}

class SectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SectionDialog(QWidget *parent = nullptr);
    ~SectionDialog();

public Q_SLOTS:
    void onChoosePointSet();
    void onChooseCartesianGrid();
    void onCreate();

private:
    Ui::SectionDialog *ui;
};

#endif // SECTIONDIALOG_H
