#ifndef WAVELETUTILS_H
#define WAVELETUTILS_H

class IJAbstractCartesianGrid;

class WaveletUtils
{
public:
    WaveletUtils();

    void transform( IJAbstractCartesianGrid* cg, int variableIndex );

};

#endif // WAVELETUTILS_H
