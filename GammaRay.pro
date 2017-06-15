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
    domain/project.cpp \
    domain/application.cpp \
    domain/projectcomponent.cpp \
    domain/objectgroup.cpp \
    domain/projectroot.cpp \
    util.cpp \
    domain/pointset.cpp \
    domain/attribute.cpp \
    domain/file.cpp \
    gs/ghostscript.cpp \
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
    domain/cartesiangrid.cpp \
    gslib/gslibparams/gslibpargrid.cpp \
    gslib/gslibparams/gslibparcolor.cpp \
    gslib/gslibparams/gslibparrepeat.cpp \
    gslib/gslibparams/widgets/widgetgslibpargrid.cpp \
    gslib/gslibparams/widgets/widgetgslibparrepeat.cpp \
    gslib/gslibparams/widgets/widgetgslibparcolor.cpp \
    gslib/igslibparameterfinder.cpp \
    gslib/workerthread.cpp \
    domain/plot.cpp \
    domain/experimentalvariogram.cpp \
    domain/variogrammodel.cpp \
    widgets/variogrammodellist.cpp \
    domain/weight.cpp \
    domain/normalvariable.cpp \
    domain/univariatedistribution.cpp \
    widgets/univariatedistributionselector.cpp \
    domain/distributioncolumn.cpp \
    domain/roles.cpp \
    domain/distribution.cpp \
    widgets/distributioncolumnroleselector.cpp \
    domain/bivariatedistribution.cpp \
    widgets/variogrammodelselector.cpp \
    widgets/cartesiangridselector.cpp \
    widgets/pointsetselector.cpp \
    widgets/variableselector.cpp \
    widgets/qlabelwithcrosshairs.cpp \
    domain/valuepairs.cpp \
    domain/thresholdcdf.cpp \
    domain/categorypdf.cpp \
    widgets/valuepairvertical.cpp \
    widgets/fileselectorwidget.cpp \
    scripting.cpp \
    gslib/gslibparams/gslibparvmodel.cpp \
    gslib/gslibparams/widgets/widgetgslibparvmodel.cpp \
    domain/triads.cpp \
    domain/categorydefinition.cpp \
    domain/univariatecategoryclassification.cpp \
    widgets/categoryselector.cpp \
    widgets/intervalandcategorywidget.cpp \
    spatialindex/spatialindexpoints.cpp \
    softindiccalib/softindicatorcalibrationdialog.cpp \
    softindiccalib/softindicatorcalibplot.cpp \
    softindiccalib/softindicatorcalibcanvaspicker.cpp \
    dialogs/cokrigingdialog.cpp \
    dialogs/aboutdialog.cpp \
    dialogs/variogramanalysisdialog.cpp \
    dialogs/valuespairsdialog.cpp \
    dialogs/triadseditordialog.cpp \
    dialogs/setupdialog.cpp \
    dialogs/postikdialog.cpp \
    dialogs/pointsetdialog.cpp \
    dialogs/nscoredialog.cpp \
    dialogs/krigingdialog.cpp \
    dialogs/indicatorkrigingdialog.cpp \
    dialogs/filecontentsdialog.cpp \
    dialogs/distributionmodelingdialog.cpp \
    dialogs/distributioncolumnrolesdialog.cpp \
    dialogs/displayplotdialog.cpp \
    dialogs/declusteringdialog.cpp \
    dialogs/datafiledialog.cpp \
    dialogs/creategriddialog.cpp \
    dialogs/cartesiangriddialog.cpp \
    dialogs/bidistributionmodelingdialog.cpp \
    widgets/pswidget.cpp \
    viewer3d/view3dwidget.cpp \
    widgets/projecttreeview.cpp \
    viewer3d/viewer3dlistwidget.cpp \
    viewer3d/view3dstyle.cpp \
    viewer3d/view3dbuilders.cpp \
    viewer3d/view3dcolortables.cpp

HEADERS  += mainwindow.h \
    domain/project.h \
    domain/application.h \
    domain/projectcomponent.h \
    domain/objectgroup.h \
    domain/projectroot.h \
    util.h \
    exceptions/invalidgslibdatafileexception.h \
    domain/pointset.h \
    domain/attribute.h \
    domain/file.h \
    gs/ghostscript.h \
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
    domain/cartesiangrid.h \
    gslib/gslibparams/gslibpargrid.h \
    gslib/gslibparams/gslibparcolor.h \
    gslib/gslibparams/gslibparrepeat.h \
    gslib/gslibparams/widgets/widgetgslibpargrid.h \
    gslib/gslibparams/widgets/widgetgslibparrepeat.h \
    gslib/gslibparams/widgets/widgetgslibparcolor.h \
    gslib/igslibparameterfinder.h \
    gslib/workerthread.h \
    domain/plot.h \
    domain/experimentalvariogram.h \
    domain/variogrammodel.h \
    widgets/variogrammodellist.h \
    domain/weight.h \
    domain/normalvariable.h \
    domain/univariatedistribution.h \
    widgets/univariatedistributionselector.h \
    domain/distributioncolumn.h \
    domain/roles.h \
    domain/distribution.h \
    widgets/distributioncolumnroleselector.h \
    domain/bivariatedistribution.h \
    widgets/variogrammodelselector.h \
    widgets/cartesiangridselector.h \
    widgets/pointsetselector.h \
    widgets/variableselector.h \
    widgets/qlabelwithcrosshairs.h \
    domain/valuepairs.h \
    domain/thresholdcdf.h \
    domain/categorypdf.h \
    widgets/valuepairvertical.h \
    widgets/fileselectorwidget.h \
    scripting.h \
    exprtk.hpp \
    gslib/gslibparams/gslibparvmodel.h \
    gslib/gslibparams/widgets/widgetgslibparvmodel.h \
    domain/triads.h \
    domain/categorydefinition.h \
    domain/univariatecategoryclassification.h \
    widgets/categoryselector.h \
    widgets/intervalandcategorywidget.h \
    spatialindex/spatialindexpoints.h \
    softindiccalib/softindicatorcalibrationdialog.h \
    softindiccalib/softindicatorcalibplot.h \
    softindiccalib/softindicatorcalibcanvaspicker.h \
    dialogs/cokrigingdialog.h \
    dialogs/aboutdialog.h \
    dialogs/bidistributionmodelingdialog.h \
    dialogs/cartesiangriddialog.h \
    dialogs/creategriddialog.h \
    dialogs/datafiledialog.h \
    dialogs/declusteringdialog.h \
    dialogs/displayplotdialog.h \
    dialogs/distributioncolumnrolesdialog.h \
    dialogs/distributionmodelingdialog.h \
    dialogs/filecontentsdialog.h \
    dialogs/indicatorkrigingdialog.h \
    dialogs/krigingdialog.h \
    dialogs/nscoredialog.h \
    dialogs/pointsetdialog.h \
    dialogs/postikdialog.h \
    dialogs/setupdialog.h \
    dialogs/triadseditordialog.h \
    dialogs/valuespairsdialog.h \
    dialogs/variogramanalysisdialog.h \
    widgets/pswidget.h \
    viewer3d/view3dwidget.h \
    widgets/projecttreeview.h \
    viewer3d/viewer3dlistwidget.h \
    viewer3d/view3dstyle.h \
    viewer3d/view3dbuilders.h \
    viewer3d/view3dcolortables.h

FORMS    += mainwindow.ui \
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
    gslib/gslibparams/widgets/widgetgslibpargrid.ui \
    gslib/gslibparams/widgets/widgetgslibparrepeat.ui \
    gslib/gslibparams/widgets/widgetgslibparcolor.ui \
    widgets/variogrammodellist.ui \
    widgets/univariatedistributionselector.ui \
    widgets/distributioncolumnroleselector.ui \
    widgets/variogrammodelselector.ui \
    widgets/cartesiangridselector.ui \
    widgets/pointsetselector.ui \
    widgets/variableselector.ui \
    widgets/valuepairvertical.ui \
    widgets/fileselectorwidget.ui \
    gslib/gslibparams/widgets/widgetgslibparvmodel.ui \
    widgets/categoryselector.ui \
    widgets/intervalandcategorywidget.ui \
    softindiccalib/softindicatorcalibrationdialog.ui \
    dialogs/cokrigingdialog.ui \
    dialogs/aboutdialog.ui \
    dialogs/bidistributionmodelingdialog.ui \
    dialogs/cartesiangriddialog.ui \
    dialogs/creategriddialog.ui \
    dialogs/datafiledialog.ui \
    dialogs/declusteringdialog.ui \
    dialogs/displayplotdialog.ui \
    dialogs/distributioncolumnrolesdialog.ui \
    dialogs/distributionmodelingdialog.ui \
    dialogs/filecontentsdialog.ui \
    dialogs/indicatorkrigingdialog.ui \
    dialogs/krigingdialog.ui \
    dialogs/nscoredialog.ui \
    dialogs/pointsetdialog.ui \
    dialogs/postikdialog.ui \
    dialogs/setupdialog.ui \
    dialogs/triadseditordialog.ui \
    dialogs/valuespairsdialog.ui \
    dialogs/variogramanalysisdialog.ui \
    widgets/pswidget.ui \
    viewer3d/view3dwidget.ui

# The Boost include path.
BOOST_INSTALL = $$(BOOST_ROOT)
isEmpty(BOOST_INSTALL){
    error(BOOST_ROOT environment variable not defined.)
}
INCLUDEPATH += $$BOOST_INSTALL/include

# The Qwt include and lib path and libraries.
QWT_INSTALL = $$(QWT_ROOT)
isEmpty(QWT_INSTALL){
    error(QWT_ROOT environment variable not defined.)
}
INCLUDEPATH += $$QWT_INSTALL/include
LIBPATH     += $$QWT_INSTALL/lib
LIBS        += -lqwt

# The VTK include and lib paths and libraries
VTK_INSTALL = $$(VTK_ROOT)
isEmpty(VTK_INSTALL){
    error(VTK_ROOT environment variable not defined.)
}
VTK_VERSION_SUFFIX=-6.3  #this suffix may be empty
INCLUDEPATH += $$VTK_INSTALL/include/vtk$$VTK_VERSION_SUFFIX
LIBPATH     += $$VTK_INSTALL/lib
LIBS        += -lvtkGUISupportQt$$VTK_VERSION_SUFFIX \
               -lvtkCommonCore$$VTK_VERSION_SUFFIX \
               -lvtkFiltersSources$$VTK_VERSION_SUFFIX \
               -lvtkRenderingCore$$VTK_VERSION_SUFFIX \
               -lvtkCommonExecutionModel$$VTK_VERSION_SUFFIX \
               -lvtkInteractionStyle$$VTK_VERSION_SUFFIX \
               -lvtkRenderingOpenGL2$$VTK_VERSION_SUFFIX \
               -lvtkRenderingAnnotation$$VTK_VERSION_SUFFIX \
               -lvtkRenderingFreeType$$VTK_VERSION_SUFFIX \
               -lvtkInteractionWidgets$$VTK_VERSION_SUFFIX \
               -lvtkCommonDataModel$$VTK_VERSION_SUFFIX


# The application version
# Don't forget to update the Util::importSettingsFromPreviousVersion() method to
# enable the import of registry/user settings of previous versions.
VERSION = 1.7.1

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
