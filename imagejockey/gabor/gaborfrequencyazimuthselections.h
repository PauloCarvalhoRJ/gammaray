#ifndef GABORFREQUENCYAZIMUTHSELECTIONS_H
#define GABORFREQUENCYAZIMUTHSELECTIONS_H

#include <vector>

struct GaborFrequencyAzimuthSelection{
    double minF, maxF, minAz, maxAz;
};

/**
 * The GaborFrequencyAzimuthSelections class holds frequency and azimuth selections
 * made by the user in GaborScanDialog.
 * Each record in this collections has four values: min. and max. frequency; min. and max. azimuth,
 * that is: a bounding box in frequency x azimuth space.  A collection of such bounding boxes allows
 * the user to create arbitrary selections to deparate structural components.
 */
typedef std::vector< GaborFrequencyAzimuthSelection > GaborFrequencyAzimuthSelections;


#endif // GABORFREQUENCYAZIMUTHSELECTIONS_H
