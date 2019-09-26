#ifndef PROJECTFILECHOOSEDIALOG_H
#define PROJECTFILECHOOSEDIALOG_H

#include <QDialog>
#include "widgets/fileselectorwidget.h"

namespace Ui {
class ProjectFileChooseDialog;
}

class FileSelectorWidget;
class File;

class ProjectFileChooseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectFileChooseDialog(const QString title,
                                     const QString caption,
                                     FileSelectorType fileType,
                                     QWidget *parent = nullptr);
    ~ProjectFileChooseDialog();

    /** Returns null pointer if no file is selected. */
    File* getSelectedFile();

private:
    Ui::ProjectFileChooseDialog *ui;

    FileSelectorWidget* m_fileSelectorWidget;
};

#endif // PROJECTFILECHOOSEDIALOG_H
