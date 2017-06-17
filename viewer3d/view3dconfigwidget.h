#ifndef VIEW3DCONFIGWIDGET_H
#define VIEW3DCONFIGWIDGET_H

#include <QWidget>

/**
  *  This is the base class of 3D viewer configuration classes for each domain object type (e.g. cartesian grid).
  */
class View3DConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit View3DConfigWidget(QWidget *parent = 0);

signals:

public slots:

};

#endif // VIEW3DCONFIGWIDGET_H
