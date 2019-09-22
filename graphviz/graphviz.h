#ifndef GRAPHVIZ_H
#define GRAPHVIZ_H
#include <QString>

/**
 * @brief The GraphViz class is the hub for interfacing with GraphViz programs, such as
 * dot.exe (Windows).
 *
 * Example of command line to render a dot syntax as .ps file. : dot -Tps filename.dot -o outfile.ps
 */
class GraphViz
{
public:
    GraphViz();

    /**
     * Calls GraphViz's dot program (DOT syntax parser) to create a PostScript vector image file.
     */
    static void makePSfromDOT( const QString input_dot_file_path,
                               const QString output_ps_file_path );
};

#endif // GRAPHVIZ_H
