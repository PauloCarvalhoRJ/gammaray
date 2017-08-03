#ifndef GRCOMPASS_H
#define GRCOMPASS_H

#include <qwt_compass.h>

/** A compass-like widget useful to visually input/display angles, azimuths, etc. */
class GRCompass: public QwtCompass
{
public:
    GRCompass(int style, QWidget *parent = nullptr );
};


#endif // GRCOMPASS_H
