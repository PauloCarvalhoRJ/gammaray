#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);

    QStringList binaries;
    binaries << "archivegen"
             << "binarycreator"
             << "devtool"
             << "installerbase"
             << "repogen";

    QTextStream cout(stdout);

    bool isSomeBinaryMissing = false;
    for (const QString &bin : binaries) {
        const QString executablePath = QStandardPaths::findExecutable(bin);
        const QString status = executablePath.isEmpty() ? "NOT FOUND" : "FOUND";
        cout << QString("Binary '%1' was %2 in '%3'\n").arg(bin, status, executablePath);

        if (executablePath.isEmpty()) {
            isSomeBinaryMissing = true;
        }
    }

    return isSomeBinaryMissing ? -1 : 0;
}
