#ifndef SVDANALYSISDIALOG_H
#define SVDANALYSISDIALOG_H

#include <QDialog>
#include <QModelIndex>

class SVDFactorTree;
class SVDFactor;
class QMenu;

namespace Ui {
	class SVDAnalysisDialog;
}

class ImageJockeyGridPlot;

class SVDAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SVDAnalysisDialog(QWidget *parent = 0);
    ~SVDAnalysisDialog();

	void setTree( SVDFactorTree* tree );

	/** Sets whether the tree pointed to by m_tree will be deleted upon closing this dialog. Default is false.
	 *  This is normally set when the dialog is non-modal.
	 */
	void setDeleteTreeOnClose( bool flag ){ m_deleteTree = flag; }

private:
    Ui::SVDAnalysisDialog *ui;
	SVDFactorTree *m_tree;
	bool m_deleteTree;
    //factor tree context menu
    QMenu *m_factorContextMenu;
    SVDFactor *m_right_clicked_factor;
	int m_numberOfSVDFactorsSetInTheDialog;
	ImageJockeyGridPlot* m_gridPlot;
	void refreshTreeStyle();
    void forcePlotUpdate();
    void adjustColorTableWidgets( int cmbIndex );

private slots:
    void onFactorContextMenu(const QPoint &mouse_location);
    void onFactorizeFurther();
	void onUserSetNumberOfSVDFactors(int number);
	void onOpenFactor();
	void onFactorClicked( QModelIndex index );
    void onCmbColorScaleValueChanged( int index );
};

#endif // SVDANALYSISDIALOG_H
