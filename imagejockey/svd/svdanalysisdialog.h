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

namespace spectral{
    struct array;
}

class IJGridViewerWidget;
class IJAbstractCartesianGrid;

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

    /**
     * Set the grid and variable index containing the phases from a Fourier transform.
     * Setting these is optional and is only necessary if the SVD analysis are on a Fourier image.
     */
    void setGridWithPhaseForPossibleRFFT( IJAbstractCartesianGrid* grid,
                                          int indexOfVariableWithPhase );

    /** Hides the analysis buttons.  This is useful to reuse this dialog to display
     * a series of grids. */
    void hideAnalysisButtons();

signals:
    /** Emitted when the user clicks on the "Save" or "Sum" button.
     *  The slot is responsible for deleting or managing the object.
     */
    void sumOfFactorsComputed( spectral::array* sumOfFactors );

private:
    Ui::SVDAnalysisDialog *ui;
	SVDFactorTree *m_tree;
	bool m_deleteTree;
    //factor tree context menu
    QMenu *m_factorContextMenu;
    SVDFactor *m_right_clicked_factor;
	int m_numberOfSVDFactorsSetInTheDialog;
    IJGridViewerWidget* m_gridViewerWidget;
    IJAbstractCartesianGrid* m_gridWithPhaseForPossibleRFFT;
    int m_variableIndexWithPhaseForPossibleRFFT;
    QStringList m_listForTreeStateKeeping;
	void refreshTreeStyle();
    void forcePlotUpdate();
    void saveTreeUIState();
    void restoreTreeUIState();

private slots:
    void onFactorContextMenu(const QPoint &mouse_location);
    void onFactorizeFurther();
	void onUserSetNumberOfSVDFactors(int number);
	void onOpenFactor();
	void onFactorClicked( QModelIndex index );
    void onSave();
    void onPreview();
    void onPreviewRFFT();
    void onDeleteChildren();
    void onAggregate();
    void onSaveAFactor();
	void onCheckSelected();
	void onUncheckSelected();
	void onInvertCheckOfSelected();
	void onCustomAnalysis();
	void onSaveFactorData(const SVDFactor* factor);
};

#endif // SVDANALYSISDIALOG_H
