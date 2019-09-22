#ifndef FILESELECTORWIDGET_H
#define FILESELECTORWIDGET_H

#include <QWidget>

namespace Ui {
class FileSelectorWidget;
}

class File;
class DataFile;
class Distribution;

/*! The file types to list. */
enum class FileSelectorType : uint {
    CDFs = 0,                    /*!< Only threshold c.d.f. files can be selected. */
    PDFs,                        /*!< Only category p.d.f. files can be selected. */
    DataFiles,                   /*!< Data files can be selected. */
    CategoryDefinitions,         /*!< Categorical definition files can be selected. */
    CDsAndCDFs,                  /*!< Categorical definition and threshold c.d.f. files can be selected. */
    CDsCDFsandPDFs,              /*!< Categorical definition, threshold c.d.f. and category p.d.f. files can be selected. */
    PointSets,                   /*!< Data files of point set type can be selected. */
    CartesianGrids,              /*!< Data files of Cartesian grid type can be selected. */
    Bidistributions,             /*!< Bidistributions can be selected. */
    FaciesTransitionMatrices     /*!< Facies transitions matrices can be selected. */
};

/**
 * The FileSelectorWidget is a generic file selector.  It can be used to select any type of file
 * but it lacks specialization.
 */
class FileSelectorWidget : public QWidget
{
    Q_OBJECT

public:
    /** @param show_not_set Adds a "NOT SET" item to indicate optional selection. */
    explicit FileSelectorWidget( FileSelectorType filesOfTypes, bool show_not_set = false, QWidget *parent = 0 );
    ~FileSelectorWidget();

    /** Returns null pointer if no file is selected. */
    File* getSelectedFile();

    /** Sets an option caption text. */
    void setCaption( QString caption );

signals:
    void fileSelected( File* file );

    /** Signal emited along with fileSelected(File*) if the selected file is a data file. */
    void dataFileSelected( DataFile* dataFile );

	/** Signal emited along with fileSelected(File*) if the selected file is a distribution. */
	void distributionSelected( Distribution* dataFile );

private:
    Ui::FileSelectorWidget *ui;
    FileSelectorType m_filesOfTypes;
    File* m_File;
    bool m_HasNotSetItem;

public slots:
    void onSelection( int index );
};

#endif // FILESELECTORWIDGET_H
