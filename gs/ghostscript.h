#ifndef GHOSTSCRIPT_H
#define GHOSTSCRIPT_H
#include <QString>

class QDir;

/**
 * @brief The Ghostscript class is the hub for interfacing with Ghostscript programs.
 */
class Ghostscript
{
public:
    Ghostscript();

    /**
     * Returns the name of the Ghostscript parser program file name.  It is OS-dependent.
     * see http://svn.ghostscript.com/ghostscript/branches/gs-db/doc/Use.htm#Help_command
     * @param bin_dir A QDir object refering the bin directory in the GhostScript installation,
     *                e.g. C:\Program Files (x86)\gs\gs8.53\bin
     */
    static QString getGsProgramName(QDir &bin_dir);

    /**
     * Calls Ghostscript parser to create a PNG image file from the given PS file.
     * see http://svn.ghostscript.com/ghostscript/branches/gs-db/doc/Devices.htm#PNG
     */
    static void makePNG( const QString input_ps_file_path, const QString output_png_file_path, int resolution );
};

#endif // GHOSTSCRIPT_H
