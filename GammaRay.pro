#-------------------------------------------------
#
# Project created by QtCreator 2014-09-13T15:55:28
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GammaRay
TEMPLATE = app

release:DESTDIR = ../GammaRay_release/dist
release:OBJECTS_DIR = ../GammaRay_release/obj
release:MOC_DIR = ../GammaRay_release/moc
release:RCC_DIR = ../GammaRay_release/rcc
release:UI_DIR = ../GammaRay_release/ui

debug:DESTDIR = ../GammaRay_debug/dist
debug:OBJECTS_DIR = ../GammaRay_debug/obj
debug:MOC_DIR = ../GammaRay_debug/moc
debug:RCC_DIR = ../GammaRay_debug/rcc
debug:UI_DIR = ../GammaRay_debug/ui

CONFIG += c++11

#QMAKE_CXXFLAGS += -m64

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
    viewer3d/view3dcolortables.cpp \
    viewer3d/view3dconfigwidget.cpp \
    viewer3d/view3dconfigwidgetsbuilder.cpp \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributein3dcartesiangrid.cpp \
    viewer3d/view3dlistrecord.cpp \
    viewer3d/view3dviewdata.cpp \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinmapcartesiangrid.cpp \
    domain/auxiliary/dataloader.cpp \
    array3d.cpp \
    geostats/geostatsutils.cpp \
    geostats/matrix3x3.cpp \
    geostats/matrixmxn.cpp \
    dialogs/ndvestimationdialog.cpp \
    geostats/gridcell.cpp \
    geostats/ndvestimation.cpp \
    geostats/spatiallocation.cpp \
    geostats/ndvestimationrunner.cpp \
    geostats/ijkdelta.cpp \
    geostats/ijkindex.cpp \
    geostats/ijkdeltascache.cpp \
    dialogs/realizationselectiondialog.cpp \
    dialogs/gridresampledialog.cpp \
    dialogs/multivariogramdialog.cpp \
    imagejockey/imagejockeydialog.cpp \
    imagejockey/imagejockeygridplot.cpp \
    widgets/grcompass.cpp \
    geostats/experimentalvariogramparameters.cpp \
    imagejockey/spectrogram1dparameters.cpp \
    imagejockey/spectrogram1dplot.cpp \
    imagejockey/spectrogram1dplotpicker.cpp \
    imagejockey/equalizer/equalizerwidget.cpp \
    imagejockey/equalizer/equalizerslider.cpp \
    dialogs/sgsimdialog.cpp \
    widgets/distributionfieldselector.cpp \
    viewer3d/view3dverticalexaggerationwidget.cpp \
    widgets/focuswatcher.cpp \
    spectral/svd.cpp \
    spectral/pca.cpp \
    spectral/spectral.cpp \
    imagejockey/imagejockeysvdutils.cpp \
    imagejockey/svdparametersdialog.cpp \
    algorithms/ialgorithmdatasource.cpp \
    algorithms/bootstrap.cpp \
    dialogs/machinelearningdialog.cpp \
    algorithms/CART/cart.cpp \
    algorithms/CART/cartdecisionnode.cpp \
    algorithms/CART/cartleafnode.cpp \
    algorithms/CART/cartnode.cpp \
    algorithms/CART/cartsplitcriterion.cpp \
    algorithms/randomforest.cpp \
    algorithms/decisiontree.cpp \
    domain/auxiliary/variableremover.cpp \
    domain/auxiliary/datasaver.cpp

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
    viewer3d/view3dcolortables.h \
    viewer3d/view3dconfigwidget.h \
    viewer3d/view3dconfigwidgetsbuilder.h \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributein3dcartesiangrid.h \
    viewer3d/view3dlistrecord.h \
    viewer3d/view3dviewdata.h \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinmapcartesiangrid.h \
    domain/auxiliary/dataloader.h \
    array3d.h \
    geostats/geostatsutils.h \
    geostats/matrix3x3.h \
    geostats/matrixmxn.h \
    dialogs/ndvestimationdialog.h \
    geostats/gridcell.h \
    geostats/ndvestimation.h \
    geostats/spatiallocation.h \
    geostats/ndvestimationrunner.h \
    geostats/ijkdelta.h \
    geostats/ijkindex.h \
    geostats/ijkdeltascache.h \
    dialogs/realizationselectiondialog.h \
    dialogs/gridresampledialog.h \
    dialogs/multivariogramdialog.h \
    imagejockey/imagejockeydialog.h \
    imagejockey/imagejockeygridplot.h \
    widgets/grcompass.h \
    geostats/experimentalvariogramparameters.h \
    imagejockey/spectrogram1dparameters.h \
    imagejockey/spectrogram1dplot.h \
    imagejockey/spectrogram1dplotpicker.h \
    imagejockey/equalizer/equalizerwidget.h \
    imagejockey/equalizer/equalizerslider.h \
    dialogs/sgsimdialog.h \
    widgets/distributionfieldselector.h \
    viewer3d/view3dverticalexaggerationwidget.h \
    widgets/focuswatcher.h \
    spectral/svd.h \
    spectral/pca.h \
    spectral/spectral.h \
    imagejockey/imagejockeysvdutils.h \
    imagejockey/svdparametersdialog.h \
    algorithms/ialgorithmdatasource.h \
    algorithms/bootstrap.h \
    dialogs/machinelearningdialog.h \
    algorithms/CART/cart.h \
    algorithms/CART/cartdecisionnode.h \
    algorithms/CART/cartleafnode.h \
    algorithms/CART/cartnode.h \
    algorithms/CART/cartsplitcriterion.h \
    algorithms/randomforest.h \
    algorithms/decisiontree.h \
    domain/auxiliary/variableremover.h \
    domain/auxiliary/datasaver.h

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
    viewer3d/view3dwidget.ui \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributein3dcartesiangrid.ui \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinmapcartesiangrid.ui \
    dialogs/ndvestimationdialog.ui \
    dialogs/realizationselectiondialog.ui \
    dialogs/gridresampledialog.ui \
    dialogs/multivariogramdialog.ui \
    imagejockey/imagejockeydialog.ui \
    imagejockey/equalizer/equalizerwidget.ui \
    imagejockey/equalizer/equalizerslider.ui \
    dialogs/sgsimdialog.ui \
    widgets/distributionfieldselector.ui \
    viewer3d/view3dverticalexaggerationwidget.ui \
    imagejockey/svdparametersdialog.ui \
    dialogs/machinelearningdialog.ui

#==================== The Boost include path.==================
_BOOST_INCLUDE = $$(BOOST_INCLUDE)
isEmpty(_BOOST_INCLUDE){
    error(BOOST_INCLUDE environment variable not defined.)
}
INCLUDEPATH += $$_BOOST_INCLUDE
#==============================================================

#========= The Qwt include and lib path and libraries.=========
_QWT_INCLUDE = $$(QWT_INCLUDE)
isEmpty(_QWT_INCLUDE){
    error(QWT_INCLUDE environment variable not defined.)
}
_QWT_LIB = $$(QWT_LIB)
isEmpty(_QWT_LIB){
    error(QWT_LIB environment variable not defined.)
}
INCLUDEPATH += $$_QWT_INCLUDE
LIBPATH     += $$_QWT_LIB
LIBS        += -lqwt
#==============================================================

#========== The VTK include and lib paths and libraries==================
_VTK_INCLUDE = $$(VTK_INCLUDE)
isEmpty(_VTK_INCLUDE){
    error(VTK_INCLUDE environment variable not defined.)
}
_VTK_LIB = $$(VTK_LIB)
isEmpty(_VTK_LIB){
    error(VTK_LIB environment variable not defined.)
}
_VTK_VERSION_SUFFIX = $$(VTK_VERSION_SUFFIX)
isEmpty(_VTK_VERSION_SUFFIX){
    warning(VTK_VERSION_SUFFIX environment variable not defined or empty.)
}
INCLUDEPATH += $$_VTK_INCLUDE
LIBPATH     += $$_VTK_LIB
LIBS        += -lvtkGUISupportQt$$_VTK_VERSION_SUFFIX \
               -lvtkCommonCore$$_VTK_VERSION_SUFFIX \
               -lvtkFiltersSources$$_VTK_VERSION_SUFFIX \
               -lvtkRenderingCore$$_VTK_VERSION_SUFFIX \
               -lvtkCommonExecutionModel$$_VTK_VERSION_SUFFIX \
               -lvtkInteractionStyle$$_VTK_VERSION_SUFFIX \
               -lvtkRenderingOpenGL2$$_VTK_VERSION_SUFFIX \
               -lvtkRenderingAnnotation$$_VTK_VERSION_SUFFIX \
               -lvtkRenderingFreeType$$_VTK_VERSION_SUFFIX \
               -lvtkInteractionWidgets$$_VTK_VERSION_SUFFIX \
               -lvtkCommonDataModel$$_VTK_VERSION_SUFFIX \
               -lvtkFiltersGeneral$$_VTK_VERSION_SUFFIX \
               -lvtkCommonTransforms$$_VTK_VERSION_SUFFIX \
               -lvtkImagingSources$$_VTK_VERSION_SUFFIX \
               -lvtkImagingCore$$_VTK_VERSION_SUFFIX \
               -lvtkFiltersCore$$_VTK_VERSION_SUFFIX \
               -lvtkFiltersExtraction$$_VTK_VERSION_SUFFIX \
               -lvtkImagingFourier$$_VTK_VERSION_SUFFIX \
               -lvtkImagingMath$$_VTK_VERSION_SUFFIX
               #-lvtkGUISupportQtOpenGL2$$_VTK_VERSION_SUFFIX

#=============================================================================

#========= The FFTW3 include and lib path and libraries.=========
_FFTW3_INCLUDE = $$(FFTW3_INCLUDE)
isEmpty(_FFTW3_INCLUDE){
    error(FFTW3_INCLUDE environment variable not defined.)
}
_FFTW3_LIB = $$(FFTW3_LIB)
isEmpty(_FFTW3_LIB){
    error(FFTW3_LIB environment variable not defined.)
}
INCLUDEPATH += $$_FFTW3_INCLUDE
LIBPATH     += $$_FFTW3_LIB
LIBS        += -lfftw3
LIBS        += -lfftw3f
#==============================================================

#=====================Embedded thirdparty libraries===========================
INCLUDEPATH += thirdparty
#=============================================================================

#Library used in Util::getPhysicalRAMusage()
win32 {
    LIBS += -lPsapi
}

# The application version
# Don't forget to update the Util::importSettingsFromPreviousVersion() method to
# enable the import of registry/user settings of previous versions.
VERSION = 3.6.1

# Define a preprocessor macro so we can get the application version in application code.
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# Define application nam    QMAKE_DEFAULT_INCDIRS =e macro
DEFINES += APP_NAME=\\\"$$TARGET\\\"

# Define application name and version macro
# \040 means a whitespace.  Don't replace it with an explicit space because
# it doesn't compile.
DEFINES += APP_NAME_VER=\\\"$$TARGET\\\040$$VERSION\\\"

RESOURCES += \
    resources.qrc

#set the Windows executable icon
win32:RC_ICONS += art/exeicon.ico
