#ifndef VIEW3DTEXTCONFIGWIDGET_H
#define VIEW3DTEXTCONFIGWIDGET_H

#include <QWidget>

namespace Ui {
class View3DTextConfigWidget;
}

/** This is a floating widget to allow the user to quickly set text settings without having to open
 * and close a dialog, minimizing mouse clicks.
 */
class View3DTextConfigWidget : public QWidget
{
    Q_OBJECT

public:

    /** @param contextName see m_contextName */
    explicit View3DTextConfigWidget( QString contextName, QWidget *parent = nullptr);
    ~View3DTextConfigWidget();

    /** Returns the font size set by the user. */
    int getFontSize();

    /** Returns whether the text labels must be displayed. */
    bool isShowText();

    /** Returns the text color set by the user. */
    QColor getFontColor();

    /** Method involved in floating widget dynamics.  This is normally not called
     * by client code.
     */
    virtual void setFocus();

    /**
     * Saves the text settings in the registry/user home.
     */
    void rememberSettings();

    /**
     * Retrieves the text settings in the registry/user home.
     */
    void recallSettings();

Q_SIGNALS:

    /** Signal emitted whenever the user changes some setting. */
    void change();

private:
    Ui::View3DTextConfigWidget *ui;

    /** Member involved in floating widget dynamics. It increases with an in focus event and
     * decreases with an out focus event. So, when it reaches zero, it means no sub-widget has
     * focus in this widget and this entire widget can hide.
     */
    int m_focusRefCounter;

    /** The color selected by the user in a dialog. */
    QColor m_chosenColor;

    /** The context name identify the usage of this widget.
     * It is important to not mix settings between different applications
     * of this widget when saving/retrieving them from registry/user home.
     */
    QString m_contextName;

    void updateGUI();

private Q_SLOTS:

    /** This slot is triggered whenver the user changes some setting. */
    void onChange();

    /** Method involved in floating widget dynamics.  This is normally not called
     * by client code.
     */
    void focusChanged( bool in );

    /** This is called by a timer to check whether this widget has lost focus so it can
     * be automatically hidden (no need to click to close it).
     */
    void onCheckFocusOut();

    /** Called when the user clicks on the "..." button to open the labels color choosing dialog. */
    void onColorChoose();
};

#endif // VIEW3DTEXTCONFIGWIDGET_H
