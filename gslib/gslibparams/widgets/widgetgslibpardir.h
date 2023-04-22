#ifndef WIDGETGSLIBPARDIR_H
#define WIDGETGSLIBPARDIR_H

#include <QWidget>

class GSLibParDir;

namespace Ui {
class WidgetGSLibParDir;
}

class WidgetGSLibParDir : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParDir(QWidget *parent = nullptr);
    ~WidgetGSLibParDir();

    void fillFields( GSLibParDir* param );
    void updateValue( GSLibParDir* param );

public slots:
    void onBtnFileClicked(bool);

private:
    Ui::WidgetGSLibParDir *ui;
};

#endif // WIDGETGSLIBPARDIR_H
