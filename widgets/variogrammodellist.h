#ifndef VARIOGRAMMODELLIST_H
#define VARIOGRAMMODELLIST_H

#include <QWidget>
#include <QModelIndex>

class VariogramModel;

namespace Ui {
class VariogramModelList;
}

class VariogramModelList : public QWidget
{
    Q_OBJECT

public:
    explicit VariogramModelList(QWidget *parent = 0);
    ~VariogramModelList();

    /** Returns the pointer to the selected variogram model or nullptr if none or more than one was selected. */
    VariogramModel* getSelectedVModel();

signals:
    void variogramClicked();

private:
    Ui::VariogramModelList *ui;

private slots:
    void onVariogramSelection( QModelIndex index );
};

#endif // VARIOGRAMMODELLIST_H
