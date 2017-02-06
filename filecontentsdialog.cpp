#include "filecontentsdialog.h"
#include "ui_filecontentsdialog.h"
#include <QTextStream>

FileContentsDialog::FileContentsDialog(QWidget *parent, const QString file_path, const QString title) :
    QDialog(parent),
    ui(new Ui::FileContentsDialog)
{
    ui->setupUi(this);

    this->setWindowTitle( title );

    //read file content sample
    QFile file( file_path );
    file.open( QFile::ReadOnly | QFile::Text );
    QTextStream in(&file);
    //read up to 100 first lines
    while ( !in.atEnd() )
    {
       QString line = in.readLine();
       this->ui->txtFileContents->appendPlainText( line );
    }
    file.close();
    //send text cursor to home
    QTextCursor tmpCursor = ui->txtFileContents->textCursor();
    tmpCursor.movePosition(QTextCursor::Start);
    ui->txtFileContents->setTextCursor(tmpCursor);
}

FileContentsDialog::~FileContentsDialog()
{
    delete ui;
}
