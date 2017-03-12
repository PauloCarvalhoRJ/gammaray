#ifndef FILESELECTORWIDGET_H
#define FILESELECTORWIDGET_H

#include <QWidget>

namespace Ui {
class FileSelectorWidget;
}

class File;

/*! The file types to list. */
enum class FileSelectorType : uint {
    CDFs = 0, /*!< Only threshold c.d.f. files can be selected. */
    PDFs     /*!< Only category p.d.f. files can be selected. */
};

/**
 * The FileSelectorWidget is a generic file selector.  It can be used to select any type of file
 * but it lacks specialization.
 */
class FileSelectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileSelectorWidget( FileSelectorType filesOfTypes, QWidget *parent = 0);
    ~FileSelectorWidget();

    /** Returns null pointer if no file is selected. */
    File* getSelectedFile();

signals:
    void fileSelected( File* file );

private:
    Ui::FileSelectorWidget *ui;
    FileSelectorType m_filesOfTypes;
    File* m_File;

public slots:
    void onSelection( int index );
};

#endif // FILESELECTORWIDGET_H
