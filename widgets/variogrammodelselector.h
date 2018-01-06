#ifndef VARIOGRAMMODELSELECTOR_H
#define VARIOGRAMMODELSELECTOR_H

#include <QWidget>

namespace Ui {
class VariogramModelSelector;
}

class VariogramModel;

class VariogramModelSelector : public QWidget
{
    Q_OBJECT

public:
    /** @param show_not_set If true, adds a "NOT SET" item as the first item. */
    explicit VariogramModelSelector(bool show_not_set = false, QWidget *parent = 0);
    ~VariogramModelSelector();

    /** Returns the pointer to the selected variogram model or nullptr if none was selected. */
    VariogramModel *getSelectedVModel();

    /** Re-populates the variogram model list. */
    void updateList();

    /** Makes the combo box show the desired variogam model given its name.
        Nothing happens if the variogram model is not found (perhaps calling updateList() before). */
    void selectVariogram( const QString name );

signals:
    void variogramSelected();

private:
    Ui::VariogramModelSelector *ui;
    bool m_HasNotSetItem;

private slots:
    void onVariogramSelected( int index );
};

#endif // VARIOGRAMMODELSELECTOR_H
