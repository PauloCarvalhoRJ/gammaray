#ifndef VALUEPAIRVERTICAL_H
#define VALUEPAIRVERTICAL_H

#include <QWidget>

namespace Ui {
class ValuePairVertical;
}

/** This widget is composed by two edit boxes one on top of the other. */
class ValuePairVertical : public QWidget
{
    Q_OBJECT

public:
    explicit ValuePairVertical(QWidget *parent = 0);
    ~ValuePairVertical();

    /** Returns the text entered in the upper box. */
    QString get1st();

    /** Returns the text entered in the lower box. */
    QString get2nd();

    /** Sets the text of the upper box. */
    void set1st( QString value );

    /** Sets the text of the lower box. */
    void set2nd( QString value );

private:
    Ui::ValuePairVertical *ui;
};

#endif // VALUEPAIRVERTICAL_H
