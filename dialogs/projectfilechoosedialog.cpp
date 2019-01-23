#include "projectfilechoosedialog.h"
#include "ui_projectfilechoosedialog.h"

ProjectFileChooseDialog::ProjectFileChooseDialog(const QString title,
                                                 const QString caption,
                                                 FileSelectorType fileType,
                                                 QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProjectFileChooseDialog)
{
    ui->setupUi(this);
    m_fileSelectorWidget = new FileSelectorWidget( fileType, true );
    ui->layoutPlaceholder->addWidget( m_fileSelectorWidget );
    ui->lblCaption->setText( caption );
    setWindowTitle( title );
}

ProjectFileChooseDialog::~ProjectFileChooseDialog()
{
    delete ui;
}

File *ProjectFileChooseDialog::getSelectedFile()
{
    return m_fileSelectorWidget->getSelectedFile();
}
