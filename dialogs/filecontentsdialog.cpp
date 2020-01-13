#include "filecontentsdialog.h"
#include "ui_filecontentsdialog.h"
#include <QFile>
#include <QTextStream>
#include <QStringBuilder>

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
    QString fileContents;
    while ( !in.atEnd() )
    {
       QString line = in.readLine();
       fileContents = fileContents % line % '\n';
    }
    this->ui->txtFileContents->setPlainText( fileContents );
    file.close();
    //send text cursor to home
    QTextCursor tmpCursor = ui->txtFileContents->textCursor();
    tmpCursor.movePosition(QTextCursor::Start);
    ui->txtFileContents->setTextCursor(tmpCursor);

    adjustSize();
}

FileContentsDialog::~FileContentsDialog()
{
    delete ui;
}
