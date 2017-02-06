#ifndef WIDGETGSLIBPARFILE_H
#define WIDGETGSLIBPARFILE_H

#include <QWidget>

class GSLibParFile;

namespace Ui {
class WidgetGSLibParFile;
}

class WidgetGSLibParFile : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParFile(QWidget *parent = 0);
    ~WidgetGSLibParFile();

    void fillFields( GSLibParFile* param );
    void updateValue( GSLibParFile* param );
public slots:
    void onBtnFileClicked(bool);

private:
    Ui::WidgetGSLibParFile *ui;
};

#endif // WIDGETGSLIBPARFILE_H
