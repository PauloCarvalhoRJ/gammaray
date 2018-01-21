#ifndef SVDANALYSISDIALOG_H
#define SVDANALYSISDIALOG_H

#include <QDialog>

#include "svdfactortree.h"

namespace Ui {
class SVDAnalysisDialog;
}

class SVDAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SVDAnalysisDialog(QWidget *parent = 0);
    ~SVDAnalysisDialog();

    void setTree( SVDFactorTree&& tree );

private:
    Ui::SVDAnalysisDialog *ui;
    SVDFactorTree m_tree;
};

#endif // SVDANALYSISDIALOG_H
