#include "view3dtextconfigwidget.h"
#include "ui_view3dtextconfigwidget.h"
#include "widgets/focuswatcher.h"

#include <QColorDialog>
#include <QSettings>
#include <QTimer>

View3DTextConfigWidget::View3DTextConfigWidget(QString contextName, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::View3DTextConfigWidget),
    m_focusRefCounter(0),
    m_chosenColor(0, 0, 0),
    m_contextName( contextName )
{
    ui->setupUi(this);

    //monitor focus events without subclassing the widget
    connect(new FocusWatcher(ui->spinFontSize), &FocusWatcher::focusChanged,
            this, &View3DTextConfigWidget::focusChanged);
    connect(new FocusWatcher(ui->chkShow), &FocusWatcher::focusChanged,
            this, &View3DTextConfigWidget::focusChanged);
    connect(new FocusWatcher(ui->btnColor), &FocusWatcher::focusChanged,
            this, &View3DTextConfigWidget::focusChanged);

    //setup a timer to check for out focus periodically to hide this widget automatically.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onCheckFocusOut()));
    timer->start(1000); //time specified in ms

    recallSettings();
}

View3DTextConfigWidget::~View3DTextConfigWidget()
{
    rememberSettings();
    delete ui;
}

int View3DTextConfigWidget::getFontSize()
{
    return ui->spinFontSize->value();
}

bool View3DTextConfigWidget::isShowText()
{
    return ui->chkShow->isChecked();
}

QColor View3DTextConfigWidget::getFontColor()
{
    return m_chosenColor;
}

void View3DTextConfigWidget::setFocus()
{
    QWidget::setFocus();
    ui->spinFontSize->setFocus();
    m_focusRefCounter = 1;
}

void View3DTextConfigWidget::rememberSettings()
{
    QSettings qsettings;
    qsettings.beginGroup( "View3DTextConfigWidget." + m_contextName );
    qsettings.setValue( "textSize", ui->spinFontSize->value() );
    qsettings.setValue( "textVisibility", ui->chkShow->isChecked() );
    qsettings.setValue( "textColor", m_chosenColor );
    qsettings.endGroup();
}

void View3DTextConfigWidget::recallSettings()
{
    QSettings qsettings;
    qsettings.beginGroup( "View3DTextConfigWidget." + m_contextName  );
    if( qsettings.contains( "textSize" ) ){
        ui->spinFontSize->setValue( qsettings.value( "textSize", 12 ).toInt() );
        ui->chkShow->setChecked( qsettings.value( "textVisibility", true ).toBool() );
        m_chosenColor = qsettings.value( "textColor" ).value<QColor>();
        updateGUI();
        onChange();
    }
    qsettings.endGroup();
}

void View3DTextConfigWidget::updateGUI()
{
    // Paint the label with the text color chosen by the user in a dialog.
    QString values = QString("%1, %2, %3").arg( m_chosenColor.red()).
                                           arg( m_chosenColor.green()).
                                           arg( m_chosenColor.blue());
    ui->lblColor->setStyleSheet("QLabel { background-color: rgb("+values+"); }");
}

void View3DTextConfigWidget::onChange()
{
    emit change();
}

void View3DTextConfigWidget::focusChanged(bool in)
{
    if( ! in )
        m_focusRefCounter--;
    else
        m_focusRefCounter++;
}

void View3DTextConfigWidget::onCheckFocusOut()
{
    if( m_focusRefCounter == 0 )
        this->hide();
}

void View3DTextConfigWidget::onColorChoose()
{
    QColorDialog colorD;

    m_focusRefCounter++; //this prevents this widget to hide when it looses focus to the dialog.
    int result = colorD.exec();
    m_focusRefCounter--;

    if( result == QDialog::Accepted ){
        m_chosenColor = colorD.selectedColor();
        updateGUI();
        onChange();
    }
}
