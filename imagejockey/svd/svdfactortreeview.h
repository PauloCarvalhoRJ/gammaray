#ifndef SVDFACTORTREEVIEW_H
#define SVDFACTORTREEVIEW_H

#include <QTreeView>

/** This is a QTreeView subclass tailored to support certain operations on SVDFactor objects.
 * If you use Qt Creator, you can use the "Promote to..." option from the context menu when you right click
 * on the QTreeView on the form's object list. You then have to enter the name of the subclass (SVDFactorTreeView)
 * and you have to enter the name of new header file (imagejockey/svd/svdfactortreeview.h), where the subclass is declared.
 * So you can use this custom class in Qt Creator's Designer.
 */
class SVDFactorTreeView : public QTreeView
{

    Q_OBJECT

public:
    explicit SVDFactorTreeView(QWidget *parent = nullptr);

    //OVERRIDE QTreeView EVENTS HERE AS REQUIRED.

signals:

public slots:

};

#endif // SVDFACTORTREEVIEW_H
