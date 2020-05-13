#ifndef THINSECTIONANALYSISDIALOG_H
#define THINSECTIONANALYSISDIALOG_H

#include <QDialog>

namespace Ui {
class ThinSectionAnalysisDialog;
}

class QFileIconProvider;

class ThinSectionAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ThinSectionAnalysisDialog(QWidget *parent = nullptr);
    ~ThinSectionAnalysisDialog();

private:
    Ui::ThinSectionAnalysisDialog *ui;

    QString m_directoryPath;

    QFileIconProvider* m_qFileIconProvider;

private Q_SLOTS:

    void onOpenDir();

    void onDirectoryChanged();

    void onPlanePolarizationImageSelected();

    void onCrossPolarizationImageSelected();

    void onUpdateImageDisplays();

};

#endif // THINSECTIONANALYSISDIALOG_H
