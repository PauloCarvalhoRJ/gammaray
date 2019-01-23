#ifndef PROJECTFILECHOOSEDIALOG_H
#define PROJECTFILECHOOSEDIALOG_H

#include <QDialog>

namespace Ui {
class ProjectFileChooseDialog;
}

class ProjectFileChooseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectFileChooseDialog(QWidget *parent = nullptr);
    ~ProjectFileChooseDialog();

private:
    Ui::ProjectFileChooseDialog *ui;
};

#endif // PROJECTFILECHOOSEDIALOG_H
