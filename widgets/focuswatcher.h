#ifndef FOCUSWATCHER_H
#define FOCUSWATCHER_H

#include <QObject>
#include <QEvent>

/**
 * The FocusWatcher class can be used to monitor focus events without subclassing widgets.
 * See example of use in View3DVerticalExaggerationWidget.
 */
class FocusWatcher : public QObject
{

   Q_OBJECT

public:
   explicit FocusWatcher(QObject* parent = nullptr) : QObject(parent)
   {
      if (parent)
         parent->installEventFilter(this);
   }
   virtual bool eventFilter(QObject *obj, QEvent *event) override
   {
      Q_UNUSED(obj)
      if (event->type() == QEvent::FocusIn)
         emit focusChanged(true);
      else if (event->type() == QEvent::FocusOut)
         emit focusChanged(false);

      return false;
   }

Q_SIGNALS:
   void focusChanged(bool in);
};

#endif // FOCUSWATCHER_H
