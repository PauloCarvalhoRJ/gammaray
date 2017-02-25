#-------------------------------------------------
#
# Project created by QtCreator 2014-09-13T15:55:28
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GammaRay
TEMPLATE = app

release:DESTDIR = ../GammaRay_release/dest
release:OBJECTS_DIR = ../GammaRay_release/obj
release:MOC_DIR = ../GammaRay_release/moc
release:RCC_DIR = ../GammaRay_release/rcc
release:UI_DIR = ../GammaRay_release/ui

debug:DESTDIR = ../GammaRay_debug/dest
debug:OBJECTS_DIR = ../GammaRay_debug/obj
debug:MOC_DIR = ../GammaRay_debug/moc
debug:RCC_DIR = ../GammaRay_debug/rcc
debug:UI_DIR = ../GammaRay_debug/ui

CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    aboutdialog.cpp \
    setupdialog.cpp \
    domain/project.cpp \
    domain/application.cpp \
    domain/projectcomponent.cpp \
    domain/objectgroup.cpp \
    domain/projectroot.cpp \
    datafiledialog.cpp \
    pointsetdialog.cpp \
    util.cpp \
    domain/pointset.cpp \
    domain/attribute.cpp \
    domain/file.cpp \
    filecontentsdialog.cpp \
    gs/ghostscript.cpp \
    pswidget.cpp \
    displayplotdialog.cpp \
    gslib/gslib.cpp \
    gslib/gslibparams/gslibpartype.cpp \
    gslib/gslibparams/gslibparinputdata.cpp \
    gslib/gslibparams/gslibparvarweight.cpp \
    gslib/gslibparams/gslibparfile.cpp \
    gslib/gslibparams/gslibparlimitsdouble.cpp \
    gslib/gslibparameterfiles/gslibparameterfile.cpp \
    gslib/gslibparams/gslibpardouble.cpp \
    gslib/gslibparams/gslibparuint.cpp \
    gslib/gslibparams/gslibparoption.cpp \
    gslib/gslibparams/gslibparint.cpp \
    gslib/gslibparams/gslibparstring.cpp \
    gslib/gslibparams/gslibparrange.cpp \
    gslib/gslibparams/widgets/widgetgslibpardouble.cpp \
    gslib/gslibparams/widgets/widgetgslibparfile.cpp \
    gslib/gslibparams/widgets/widgetgslibparinputdata.cpp \
    gslib/gslibparams/gslibparmultivaluedfixed.cpp \
    domain/datafile.cpp \
    gslib/gslibparametersdialog.cpp \
    gslib/gslibparams/gslibparmultivaluedvariable.cpp \
    gslib/gslibparams/widgets/widgetgslibparmultivaluedvariable.cpp \
    gslib/gslibparams/widgets/widgetgslibparint.cpp \
    gslib/gslibparams/widgets/widgetgslibparlimitsdouble.cpp \
    gslib/gslibparams/widgets/widgetgslibparmultivaluedfixed.cpp \
    gslib/gslibparams/widgets/widgetgslibparuint.cpp \
    gslib/gslibparams/widgets/widgetgslibparstring.cpp \
    gslib/gslibparams/widgets/widgetgslibparoption.cpp \
    gslib/gslibparams/widgets/widgetgslibparrange.cpp \
    cartesiangriddialog.cpp \
    domain/cartesiangrid.cpp \
    gslib/gslibparams/gslibpargrid.cpp \
    gslib/gslibparams/gslibparcolor.cpp \
    gslib/gslibparams/gslibparrepeat.cpp \
    gslib/gslibparams/widgets/widgetgslibpargrid.cpp \
    gslib/gslibparams/widgets/widgetgslibparrepeat.cpp \
    gslib/gslibparams/widgets/widgetgslibparcolor.cpp \
    variogramanalysisdialog.cpp \
    gslib/igslibparameterfinder.cpp \
    gslib/workerthread.cpp \
    declusteringdialog.cpp \
    nscoredialog.cpp \
    domain/plot.cpp \
    domain/experimentalvariogram.cpp \
    domain/variogrammodel.cpp \
    creategriddialog.cpp \
    widgets/variogrammodellist.cpp \
    domain/weight.cpp \
    domain/normalvariable.cpp \
    distributionmodelingdialog.cpp \
    domain/univariatedistribution.cpp \
    bidistributionmodelingdialog.cpp \
    widgets/univariatedistributionselector.cpp \
    domain/distributioncolumn.cpp \
    domain/roles.cpp \
    domain/distribution.cpp \
    widgets/distributioncolumnroleselector.cpp \
    distributioncolumnrolesdialog.cpp \
    domain/bivariatedistribution.cpp \
    krigingdialog.cpp \
    widgets/variogrammodelselector.cpp \
    widgets/cartesiangridselector.cpp \
    widgets/pointsetselector.cpp \
    widgets/variableselector.cpp \
    widgets/qlabelwithcrosshairs.cpp \
    domain/valuepairs.cpp \
    domain/thresholdcdf.cpp \
    domain/categorypdf.cpp \
    widgets/valuepairvertical.cpp \
    valuespairsdialog.cpp \
    indicatorkrigingdialog.cpp \
    widgets/fileselectorwidget.cpp \
    scripting.cpp \
    gslib/gslibparams/gslibparvmodel.cpp \
    gslib/gslibparams/widgets/widgetgslibparvmodel.cpp

HEADERS  += mainwindow.h \
    aboutdialog.h \
    setupdialog.h \
    domain/project.h \
    domain/application.h \
    domain/projectcomponent.h \
    domain/objectgroup.h \
    domain/projectroot.h \
    datafiledialog.h \
    pointsetdialog.h \
    util.h \
    exceptions/invalidgslibdatafileexception.h \
    domain/pointset.h \
    domain/attribute.h \
    domain/file.h \
    filecontentsdialog.h \
    gs/ghostscript.h \
    pswidget.h \
    displayplotdialog.h \
    gslib/gslib.h \
    gslib/gslibparams/gslibpartype.h \
    gslib/gslibparams/gslibparinputdata.h \
    gslib/gslibparams/gslibparvarweight.h \
    gslib/gslibparams/gslibparfile.h \
    gslib/gslibparams/gslibparlimitsdouble.h \
    gslib/gslibparameterfiles/gslibparameterfile.h \
    gslib/gslibparams/gslibpardouble.h \
    gslib/gslibparams/gslibparuint.h \
    gslib/gslibparams/gslibparoption.h \
    gslib/gslibparams/gslibparint.h \
    gslib/gslibparams/gslibparstring.h \
    gslib/gslibparams/gslibparrange.h \
    gslib/gslibparameterfiles/gslibparamtypes.h \
    exceptions/invalidmethodexception.h \
    exceptions/externalprogramexception.h \
    gslib/gslibparams/widgets/widgetgslibpardouble.h \
    gslib/gslibparams/widgets/widgetgslibparfile.h \
    gslib/gslibparams/widgets/widgetgslibparinputdata.h \
    gslib/gslibparams/gslibparmultivaluedfixed.h \
    domain/datafile.h \
    gslib/gslibparametersdialog.h \
    gslib/gslibparams/gslibparmultivaluedvariable.h \
    gslib/gslibparams/widgets/widgetgslibparmultivaluedvariable.h \
    gslib/gslibparams/widgets/gslibparamwidgets.h \
    gslib/gslibparams/widgets/widgetgslibparint.h \
    gslib/gslibparams/widgets/widgetgslibparlimitsdouble.h \
    gslib/gslibparams/widgets/widgetgslibparmultivaluedfixed.h \
    gslib/gslibparams/widgets/widgetgslibparuint.h \
    gslib/gslibparams/widgets/widgetgslibparstring.h \
    gslib/gslibparams/widgets/widgetgslibparoption.h \
    gslib/gslibparams/widgets/widgetgslibparrange.h \
    cartesiangriddialog.h \
    domain/cartesiangrid.h \
    gslib/gslibparams/gslibpargrid.h \
    gslib/gslibparams/gslibparcolor.h \
    gslib/gslibparams/gslibparrepeat.h \
    gslib/gslibparams/widgets/widgetgslibpargrid.h \
    gslib/gslibparams/widgets/widgetgslibparrepeat.h \
    gslib/gslibparams/widgets/widgetgslibparcolor.h \
    variogramanalysisdialog.h \
    gslib/igslibparameterfinder.h \
    gslib/workerthread.h \
    declusteringdialog.h \
    nscoredialog.h \
    domain/plot.h \
    domain/experimentalvariogram.h \
    domain/variogrammodel.h \
    creategriddialog.h \
    widgets/variogrammodellist.h \
    domain/weight.h \
    domain/normalvariable.h \
    distributionmodelingdialog.h \
    domain/univariatedistribution.h \
    bidistributionmodelingdialog.h \
    widgets/univariatedistributionselector.h \
    domain/distributioncolumn.h \
    domain/roles.h \
    domain/distribution.h \
    widgets/distributioncolumnroleselector.h \
    distributioncolumnrolesdialog.h \
    domain/bivariatedistribution.h \
    krigingdialog.h \
    widgets/variogrammodelselector.h \
    widgets/cartesiangridselector.h \
    widgets/pointsetselector.h \
    widgets/variableselector.h \
    widgets/qlabelwithcrosshairs.h \
    domain/valuepairs.h \
    domain/thresholdcdf.h \
    domain/categorypdf.h \
    widgets/valuepairvertical.h \
    valuespairsdialog.h \
    indicatorkrigingdialog.h \
    widgets/fileselectorwidget.h \
    scripting.h \
    exprtk.hpp \
    gslib/gslibparams/gslibparvmodel.h \
    gslib/gslibparams/widgets/widgetgslibparvmodel.h

FORMS    += mainwindow.ui \
    aboutdialog.ui \
    setupdialog.ui \
    datafiledialog.ui \
    pointsetdialog.ui \
    filecontentsdialog.ui \
    pswidget.ui \
    displayplotdialog.ui \
    gslib/gslibparams/widgets/widgetgslibpardouble.ui \
    gslib/gslibparams/widgets/widgetgslibparfile.ui \
    gslib/gslibparams/widgets/widgetgslibparinputdata.ui \
    gslib/gslibparametersdialog.ui \
    gslib/gslibparams/widgets/widgetgslibparmultivaluedvariable.ui \
    gslib/gslibparams/widgets/widgetgslibparint.ui \
    gslib/gslibparams/widgets/widgetgslibparlimitsdouble.ui \
    gslib/gslibparams/widgets/widgetgslibparmultivaluedfixed.ui \
    gslib/gslibparams/widgets/widgetgslibparuint.ui \
    gslib/gslibparams/widgets/widgetgslibparstring.ui \
    gslib/gslibparams/widgets/widgetgslibparoption.ui \
    gslib/gslibparams/widgets/widgetgslibparrange.ui \
    cartesiangriddialog.ui \
    gslib/gslibparams/widgets/widgetgslibpargrid.ui \
    gslib/gslibparams/widgets/widgetgslibparrepeat.ui \
    gslib/gslibparams/widgets/widgetgslibparcolor.ui \
    variogramanalysisdialog.ui \
    declusteringdialog.ui \
    nscoredialog.ui \
    creategriddialog.ui \
    widgets/variogrammodellist.ui \
    distributionmodelingdialog.ui \
    bidistributionmodelingdialog.ui \
    widgets/univariatedistributionselector.ui \
    widgets/distributioncolumnroleselector.ui \
    distributioncolumnrolesdialog.ui \
    krigingdialog.ui \
    widgets/variogrammodelselector.ui \
    widgets/cartesiangridselector.ui \
    widgets/pointsetselector.ui \
    widgets/variableselector.ui \
    widgets/valuepairvertical.ui \
    valuespairsdialog.ui \
    indicatorkrigingdialog.ui \
    widgets/fileselectorwidget.ui \
    gslib/gslibparams/widgets/widgetgslibparvmodel.ui

# The application version
VERSION = 1.1.0

# Define a preprocessor macro so we can get the application version in application code.
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# Define application name macro
DEFINES += APP_NAME=\\\"$$TARGET\\\"

# Define application name and version macro
# \040 means a whitespace.  Don't replace it with an explicit space because
# it doesn't compile.
DEFINES += APP_NAME_VER=\\\"$$TARGET\\\040$$VERSION\\\"

RESOURCES += \
    resources.qrc

#set the Windows executable icon
win32:RC_ICONS += art/exeicon.ico
