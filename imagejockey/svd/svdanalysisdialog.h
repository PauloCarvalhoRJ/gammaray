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

	void setTree( SVDFactorTree* tree );

	/** Sets whether the tree pointed to by m_tree will be deleted upon closing this dialog. Default is false.*/
	void setDeleteTreeOnClose( bool flag ){ m_deleteTree = flag; }

private:
    Ui::SVDAnalysisDialog *ui;
	SVDFactorTree *m_tree;
	bool m_deleteTree;
};

#endif // SVDANALYSISDIALOG_H
