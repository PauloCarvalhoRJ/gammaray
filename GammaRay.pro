#-------------------------------------------------
#
# Project created by QtCreator 2014-09-13T15:55:28
#
#-------------------------------------------------

CONFIG += c++14 #required by Boost > 1.72

QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 5): QT += widgets

PROGRAM_NAME = GammaRay #this name identifies the program such as in Windows Registry
TARGET = GammaRay #this is the name for the executable, it does not need to be the same as PROGRAM_NAME
TEMPLATE = app

include(GammaRay.pri)

#========for the separate calculator scripting library built with libCalcScripting.pro========
LIBPATH += $$DESTDIR
CALCSCRIPTING_LIB_NAME = CalcScripting
win32{
	CALCSCRIPTING_LIB_NAME = CalcScripting1
}
LIBS += -l$$CALCSCRIPTING_LIB_NAME
#============================================================================================

win32 {
	#-Wa,-mbig-obj not currently supported (or possibly not necessary) by GCC 4.8 (Linus)
	#necessary for compiling svd.cpp in debug mode.
	QMAKE_CXXFLAGS_DEBUG += -Wa,-mbig-obj
	#Don't know why -Wa,-mbig-obj sticks... removing it for release mode.
	QMAKE_CXXFLAGS_RELEASE -= -Wa,-mbig-obj
}

SOURCES += main.cpp\
    dialogs/choosevariabledialog.cpp \
    dialogs/contactanalysisdialog.cpp \
    dialogs/driftanalysisdialog.cpp \
    dialogs/faciestransitionmatrixoptionsdialog.cpp \
    dialogs/gridrepositiondialog.cpp \
    dialogs/listbuilderdialog.cpp \
    dialogs/mcrfbayesiansimdialog.cpp \
    dialogs/populatewithproportionsfromvpcdialog.cpp \
    dialogs/sectiondialog.cpp \
    dialogs/subgriddialog.cpp \
    dialogs/transiogrambanddialog.cpp \
    domain/auxiliary/verticalproportioncurvemaker.cpp \
    domain/section.cpp \
    domain/verticalproportioncurve.cpp \
    geometry/boundingbox.cpp \
    geometry/intersectionfinder.cpp \
    geometry/quadrilateral.cpp \
    geometry/triangle.cpp \
    geostats/contactanalysis.cpp \
    geostats/driftanalysis.cpp \
    geostats/mcmcdataimputation.cpp \
    geostats/quadratic3dtrendmodelfitting.cpp \
    geostats/searchannulus.cpp \
    geostats/searchannulusstratigraphic.cpp \
    geostats/searchbox.cpp \
    geostats/searchsphericalshell.cpp \
    geostats/searchverticaldumbbell.cpp \
    geostats/searchwasher.cpp \
    gslib/gslibparameterfiles/kt3dtrendmodelparameters.cpp \
    gslib/gslibparams/gslibpardir.cpp \
    gslib/gslibparams/widgets/widgetgslibpardir.cpp \
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
    vertpropcurves/verticalproportioncurvedialog.cpp \
    vertpropcurves/verticalproportioncurvescanvaspicker.cpp \
    vertpropcurves/verticalproportioncurvesplot.cpp \
    viewer3d/v3dmouseinteractor.cpp \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeascontourlines.cpp \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinpointset.cpp \
    viewer3d/view3dtextconfigwidget.cpp \
    widgets/linechartwidget.cpp \
    widgets/listbuilder.cpp \
    widgets/qlistwidgetdnd.cpp \
    widgets/transiogrambandchartview.cpp \
    widgets/variablelistbuilder.cpp \
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
    gslib/gslibparams/gslibparvmodel.cpp \
    gslib/gslibparams/widgets/widgetgslibparvmodel.cpp \
    domain/triads.cpp \
    domain/categorydefinition.cpp \
    domain/univariatecategoryclassification.cpp \
    widgets/categoryselector.cpp \
    widgets/intervalandcategorywidget.cpp \
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
    domain/auxiliary/datasaver.cpp \
    imagejockey/svd/svdparametersdialog.cpp \
    imagejockey/svd/svdfactor.cpp \
    imagejockey/svd/svdfactortree.cpp \
    imagejockey/svd/svdanalysisdialog.cpp \
    imagejockey/svd/svdfactortreeview.cpp \
    imagejockey/svd/svdfactorsel/svdfactorsselectiondialog.cpp \
    imagejockey/svd/svdfactorsel/svdfactorsselectionchartview.cpp \
    imagejockey/svd/svdfactorsel/svdfactorsselectionchartcallout.cpp \
    imagejockey/ijabstractcartesiangrid.cpp \
    imagejockey/ijabstractvariable.cpp \
    imagejockey/imagejockeyutils.cpp \
    imagejockey/ijexperimentalvariogramparameters.cpp \
    imagejockey/ijmatrix3x3.cpp \
	imagejockey/ijspatiallocation.cpp \
	imagejockey/widgets/ijcartesiangridselector.cpp \
	imagejockey/widgets/ijvariableselector.cpp \
    imagejockey/widgets/grcompass.cpp \
    imagejockey/widgets/ijgridviewerwidget.cpp \
    calculator/calculatordialog.cpp \
    calculator/calclinenumberarea.cpp \
	calculator/calccodeeditor.cpp \
	imagejockey/vardecomp/variographicdecompositiondialog.cpp \
	imagejockey/widgets/ijquick3dviewer.cpp \
	dialogs/factorialkrigingdialog.cpp \
    geostats/fkestimation.cpp \
    geostats/searchstrategy.cpp \
    geostats/fkestimationrunner.cpp \
    geostats/datacell.cpp \
    geostats/searchneighborhood.cpp \
    geostats/searchellipsoid.cpp \
    geostats/pointsetcell.cpp \
    geostats/indexedspatiallocation.cpp \
    domain/geogrid.cpp \
    domain/gridfile.cpp \
    domain/auxiliary/meshloader.cpp \
    geometry/vector3d.cpp \
    geometry/face3d.cpp \
    dialogs/sisimdialog.cpp \
    dialogs/variograminputdialog.cpp \
    geometry/hexahedron.cpp \
    geometry/pyramid.cpp \
    geometry/tetrahedron.cpp \
    imagejockey/emd/emdanalysisdialog.cpp \
    imagejockey/gabor/gaborfilterdialog.cpp \
    imagejockey/gabor/gaborscandialog.cpp \
    imagejockey/gabor/gaborutils.cpp \
    imagejockey/gabor/gaborfrequencyazimuthselections.cpp \
    imagejockey/wavelet/wavelettransformdialog.cpp \
    imagejockey/wavelet/waveletutils.cpp \
    imagejockey/ijvariographicmodel2d.cpp \
    dialogs/automaticvarfitdialog.cpp \
    dialogs/emptydialog.cpp \
    geostats/nestedvariogramstructuresparameters.cpp \
    geostats/automaticvariogramfitting.cpp \
    dialogs/automaticvarfitexperimentsdialog.cpp \
    dialogs/segmentsetdialog.cpp \
    domain/segmentset.cpp \
    domain/quintuplets.cpp \
    gslib/gslibparams/gslibparcustomcolor.cpp \
    gslib/gslibparams/widgets/widgetgslibcustomcolor.cpp \
    dialogs/choosecategorydialog.cpp \
    domain/faciestransitionmatrix.cpp \
    dialogs/projectfilechoosedialog.cpp \
    dialogs/entropycyclicityanalysisdialog.cpp \
    dialogs/faciesrelationshipdiagramdialog.cpp \
    graphviz/graphviz.cpp \
    dialogs/transiogramdialog.cpp \
    domain/auxiliary/faciestransitionmatrixmaker.cpp \
    domain/auxiliary/thicknesscalculator.cpp \
    widgets/transiogramchartview.cpp \
    domain/verticaltransiogrammodel.cpp \
    dialogs/dynamicfaciesrelationshipdiagramdialog.cpp \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinsegmentset.cpp \
    geostats/segmentsetcell.cpp \
    domain/auxiliary/valuestransferer.cpp \
    dialogs/mcrfsimdialog.cpp \
    dialogs/lvadatasetdialog.cpp \
    geostats/mcrfsim.cpp \
    gslib/gslibparameterfiles/commonsimulationparameters.cpp \
    spatialindex/spatialindex.cpp \
    geostats/taumodel.cpp \
    dialogs/mcmcdataimputationdialog.cpp \
    imagejockey/paraviewscalarbar/vtkBoundingRectContextDevice2D.cpp \
    imagejockey/paraviewscalarbar/vtkContext2DScalarBarActor.cpp \
    imagejockey/paraviewscalarbar/vtkParaViewScalarBar.cpp \
    imagejockey/paraviewscalarbar/vtkPVScalarBarRepresentation.cpp

HEADERS  += mainwindow.h \
    dialogs/choosevariabledialog.h \
    dialogs/contactanalysisdialog.h \
    dialogs/driftanalysisdialog.h \
    dialogs/faciestransitionmatrixoptionsdialog.h \
    dialogs/gridrepositiondialog.h \
    dialogs/listbuilderdialog.h \
    dialogs/mcrfbayesiansimdialog.h \
    dialogs/populatewithproportionsfromvpcdialog.h \
    dialogs/sectiondialog.h \
    dialogs/subgriddialog.h \
    dialogs/transiogrambanddialog.h \
    domain/auxiliary/verticalproportioncurvemaker.h \
    domain/project.h \
    domain/application.h \
    domain/projectcomponent.h \
    domain/objectgroup.h \
    domain/projectroot.h \
    domain/section.h \
    domain/verticalproportioncurve.h \
    geometry/boundingbox.h \
    geometry/intersectionfinder.h \
    geometry/quadrilateral.h \
    geometry/triangle.h \
    geostats/contactanalysis.h \
    geostats/driftanalysis.h \
    geostats/mcmcdataimputation.h \
    geostats/quadratic3dtrendmodelfitting.h \
    geostats/searchannulus.h \
    geostats/searchannulusstratigraphic.h \
    geostats/searchbox.h \
    geostats/searchsphericalshell.h \
    geostats/searchverticaldumbbell.h \
    geostats/searchwasher.h \
    gslib/gslibparameterfiles/kt3dtrendmodelparameters.h \
    gslib/gslibparams/gslibpardir.h \
    gslib/gslibparams/widgets/widgetgslibpardir.h \
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
    vertpropcurves/verticalproportioncurvedialog.h \
    vertpropcurves/verticalproportioncurvescanvaspicker.h \
    vertpropcurves/verticalproportioncurvesplot.h \
    viewer3d/v3dmouseinteractor.h \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeascontourlines.h \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinpointset.h \
    viewer3d/view3dtextconfigwidget.h \
    widgets/linechartwidget.h \
    widgets/listbuilder.h \
    widgets/qlistwidgetdnd.h \
    widgets/transiogrambandchartview.h \
    widgets/variablelistbuilder.h \
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
    gslib/gslibparams/gslibparvmodel.h \
    gslib/gslibparams/widgets/widgetgslibparvmodel.h \
    domain/triads.h \
    domain/categorydefinition.h \
    domain/univariatecategoryclassification.h \
    widgets/categoryselector.h \
    widgets/intervalandcategorywidget.h \
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
    domain/auxiliary/datasaver.h \
    imagejockey/svd/svdparametersdialog.h \
    imagejockey/svd/svdfactor.h \
    imagejockey/svd/svdfactortree.h \
    imagejockey/svd/svdanalysisdialog.h \
	imagejockey/svd/svdfactortreeview.h \
	imagejockey/svd/svdfactorsel/svdfactorsselectiondialog.h \
    imagejockey/svd/svdfactorsel/svdfactorsselectionchartview.h \
    imagejockey/svd/svdfactorsel/svdfactorsselectionchartcallout.h \
    imagejockey/ijabstractcartesiangrid.h \
    imagejockey/ijabstractvariable.h \
    imagejockey/imagejockeyutils.h \
    imagejockey/ijexperimentalvariogramparameters.h \
    imagejockey/ijmatrix3x3.h \
	imagejockey/ijspatiallocation.h \
	imagejockey/widgets/ijcartesiangridselector.h \
	imagejockey/widgets/ijvariableselector.h \
    imagejockey/widgets/grcompass.h \
    imagejockey/widgets/ijgridviewerwidget.h \
    calculator/calcscripting.h \
    calculator/icalcpropertycollection.h \
    calculator/calculatordialog.h \
    calculator/icalcproperty.h \
    calculator/calclinenumberarea.h \
	calculator/calccodeeditor.h \
	imagejockey/vardecomp/variographicdecompositiondialog.h \
	imagejockey/widgets/ijquick3dviewer.h \
	dialogs/factorialkrigingdialog.h \
    geostats/fkestimation.h \
    geostats/searchstrategy.h \
    geostats/fkestimationrunner.h \
    geostats/datacell.h \
    geostats/searchneighborhood.h \
    geostats/searchellipsoid.h \
    geostats/pointsetcell.h \
    geostats/indexedspatiallocation.h \
    domain/geogrid.h \
    domain/gridfile.h \
    domain/auxiliary/meshloader.h \
    geometry/vector3d.h \
    geometry/face3d.h \
    dialogs/sisimdialog.h \
    dialogs/variograminputdialog.h \
    geometry/hexahedron.h \
    geometry/pyramid.h \
    geometry/tetrahedron.h \
    imagejockey/emd/emdanalysisdialog.h \
    imagejockey/gabor/gaborfilterdialog.h \
    imagejockey/gabor/gaborscandialog.h \
    imagejockey/gabor/gaborutils.h \
    imagejockey/gabor/gaborfrequencyazimuthselections.h \
    imagejockey/wavelet/wavelettransformdialog.h \
    imagejockey/wavelet/waveletutils.h \
    imagejockey/ijvariographicmodel2d.h \
    dialogs/automaticvarfitdialog.h \
    dialogs/emptydialog.h \
    geostats/nestedvariogramstructuresparameters.h \
    geostats/automaticvariogramfitting.h \
    dialogs/automaticvarfitexperimentsdialog.h \
    dialogs/segmentsetdialog.h \
    domain/segmentset.h \
    domain/quintuplets.h \
    gslib/gslibparams/gslibparcustomcolor.h \
    gslib/gslibparams/widgets/widgetgslibcustomcolor.h \
    dialogs/choosecategorydialog.h \
    domain/faciestransitionmatrix.h \
    dialogs/projectfilechoosedialog.h \
    dialogs/entropycyclicityanalysisdialog.h \
    dialogs/faciesrelationshipdiagramdialog.h \
    graphviz/graphviz.h \
    dialogs/transiogramdialog.h \
    domain/auxiliary/faciestransitionmatrixmaker.h \
    domain/auxiliary/thicknesscalculator.h \
    widgets/transiogramchartview.h \
    domain/verticaltransiogrammodel.h \
    dialogs/dynamicfaciesrelationshipdiagramdialog.h \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinsegmentset.h \
    geostats/segmentsetcell.h \
    domain/auxiliary/valuestransferer.h \
    dialogs/mcrfsimdialog.h \
    dialogs/lvadatasetdialog.h \
    geostats/mcrfsim.h \
    gslib/gslibparameterfiles/commonsimulationparameters.h \
    spatialindex/spatialindex.h \
    geostats/taumodel.h \
    dialogs/mcmcdataimputationdialog.h \
    imagejockey/paraviewscalarbar/vtkBoundingRectContextDevice2D.h \
    imagejockey/paraviewscalarbar/vtkContext2DScalarBarActor.h \
    imagejockey/paraviewscalarbar/vtkParaViewScalarBar.h \
    imagejockey/paraviewscalarbar/vtkPVScalarBarRepresentation.h


FORMS    += mainwindow.ui \
    dialogs/choosevariabledialog.ui \
    dialogs/contactanalysisdialog.ui \
    dialogs/driftanalysisdialog.ui \
    dialogs/faciestransitionmatrixoptionsdialog.ui \
    dialogs/gridrepositiondialog.ui \
    dialogs/listbuilderdialog.ui \
    dialogs/mcrfbayesiansimdialog.ui \
    dialogs/populatewithproportionsfromvpcdialog.ui \
    dialogs/sectiondialog.ui \
    dialogs/subgriddialog.ui \
    gslib/gslibparams/widgets/widgetgslibpardir.ui \
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
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeascontourlines.ui \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinpointset.ui \
    viewer3d/view3dtextconfigwidget.ui \
    widgets/linechartwidget.ui \
    vertpropcurves/verticalproportioncurvedialog.ui \
    widgets/variablelistbuilder.ui \
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
    dialogs/machinelearningdialog.ui \
    imagejockey/svd/svdparametersdialog.ui \
    imagejockey/svd/svdanalysisdialog.ui \
	imagejockey/svd/svdfactorsel/svdfactorsselectiondialog.ui \
	imagejockey/widgets/ijcartesiangridselector.ui \
        imagejockey/widgets/ijvariableselector.ui \
    imagejockey/widgets/ijgridviewerwidget.ui \
	calculator/calculatordialog.ui \
	imagejockey/vardecomp/variographicdecompositiondialog.ui \
	imagejockey/widgets/ijquick3dviewer.ui \
	dialogs/factorialkrigingdialog.ui \
    dialogs/sisimdialog.ui \
    dialogs/variograminputdialog.ui \
    imagejockey/emd/emdanalysisdialog.ui \
    imagejockey/gabor/gaborfilterdialog.ui \
    imagejockey/gabor/gaborscandialog.ui \
    imagejockey/wavelet/wavelettransformdialog.ui \
    dialogs/automaticvarfitdialog.ui \
    dialogs/emptydialog.ui \
    dialogs/automaticvarfitexperimentsdialog.ui \
    dialogs/segmentsetdialog.ui \
    gslib/gslibparams/widgets/widgetgslibcustomcolor.ui \
    dialogs/choosecategorydialog.ui \
    dialogs/projectfilechoosedialog.ui \
    dialogs/entropycyclicityanalysisdialog.ui \
    dialogs/faciesrelationshipdiagramdialog.ui \
    dialogs/transiogramdialog.ui \
    dialogs/dynamicfaciesrelationshipdiagramdialog.ui \
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinsegmentset.ui \
    dialogs/mcrfsimdialog.ui \
    dialogs/lvadatasetdialog.ui \
    dialogs/mcmcdataimputationdialog.ui

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
LIBS        += -lqwt-qt5
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
    gcc:QMAKE_CXXFLAGS += -isystem $$_VTK_INCLUDE  #This is to suppress the countless compiler warnings from VTK headers
  clang:QMAKE_CXXFLAGS += -isystem $$_VTK_INCLUDE  #This is to suppress the countless compiler warnings from VTK headers
  mingw:QMAKE_CXXFLAGS += -isystem $$_VTK_INCLUDE  #This is to suppress the countless compiler warnings from VTK headers
msvc:QMAKE_CXXFLAGS += /external:I $$_VTK_INCLUDE  #This is to suppress the countless compiler warnings from VTK headers
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
			   -lvtkCommonMisc$$_VTK_VERSION_SUFFIX \
			   -lvtkCommonComputationalGeometry$$_VTK_VERSION_SUFFIX \
			   -lvtkCommonMath$$_VTK_VERSION_SUFFIX \
			   -lvtksys$$_VTK_VERSION_SUFFIX \
			   -lvtkFiltersGeometry$$_VTK_VERSION_SUFFIX \
			   -lvtkCommonColor$$_VTK_VERSION_SUFFIX \
			   -lvtkCommonSystem$$_VTK_VERSION_SUFFIX \
			   -lvtkglew$$_VTK_VERSION_SUFFIX \
			   -lvtkfreetype$$_VTK_VERSION_SUFFIX \
			   -lvtkzlib$$_VTK_VERSION_SUFFIX \
			   -lvtkFiltersHybrid$$_VTK_VERSION_SUFFIX \
			   -lvtkFiltersModeling$$_VTK_VERSION_SUFFIX \
			   -lvtkImagingGeneral$$_VTK_VERSION_SUFFIX \
			   -lvtkRenderingVolume$$_VTK_VERSION_SUFFIX \
			   -lvtkFiltersStatistics$$_VTK_VERSION_SUFFIX \
                -lvtkImagingStencil$$_VTK_VERSION_SUFFIX \
                -lvtkImagingHybrid$$_VTK_VERSION_SUFFIX \
                -lvtkRenderingContext2D$$_VTK_VERSION_SUFFIX \
                -lvtkChartsCore$$_VTK_VERSION_SUFFIX \
                -lvtkRenderingContextOpenGL2$$_VTK_VERSION_SUFFIX \
                -lvtkRenderingVolumeOpenGL2$$_VTK_VERSION_SUFFIX

#=============================================================================

#========== The ITK include and lib paths and libraries==================
_ITK_INCLUDE = $$(ITK_INCLUDE)
isEmpty(_ITK_INCLUDE){
    error(ITK_INCLUDE environment variable not defined.)
}
_ITK_LIB = $$(ITK_LIB)
isEmpty(_ITK_LIB){
    error(ITK_LIB environment variable not defined.)
}
_ITK_VERSION_SUFFIX = $$(ITK_VERSION_SUFFIX)
isEmpty(_ITK_VERSION_SUFFIX){
    warning(ITK_VERSION_SUFFIX environment variable not defined or empty.)
}
    gcc:QMAKE_CXXFLAGS += -isystem $$_ITK_INCLUDE  #This is to suppress the countless compiler warnings from ITK headers
  clang:QMAKE_CXXFLAGS += -isystem $$_ITK_INCLUDE  #This is to suppress the countless compiler warnings from ITK headers
  mingw:QMAKE_CXXFLAGS += -isystem $$_ITK_INCLUDE  #This is to suppress the countless compiler warnings from ITK headers
msvc:QMAKE_CXXFLAGS += /external:I $$_ITK_INCLUDE  #This is to suppress the countless compiler warnings from ITK headers
INCLUDEPATH += $$_ITK_INCLUDE
LIBPATH     += $$_ITK_LIB
LIBS        += -lITKCommon$$_ITK_VERSION_SUFFIX \
               -lITKIOImageBase$$_ITK_VERSION_SUFFIX \
               -litkvnl$$_ITK_VERSION_SUFFIX \
               -litkvnl_algo$$_ITK_VERSION_SUFFIX \
               -lITKIOPNG$$_ITK_VERSION_SUFFIX \
               -lITKTransform$$_ITK_VERSION_SUFFIX \
               -lITKSmoothing$$_ITK_VERSION_SUFFIX \
               -lITKConvolution$$_ITK_VERSION_SUFFIX

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

#========= The GSL (GNU Scientific Library) include and lib path and libraries.=========
_GSL_INCLUDE = $$(GSL_INCLUDE)
isEmpty(_GSL_INCLUDE){
        error(GSL_INCLUDE environment variable not defined.)
}
_GSL_LIB = $$(GSL_LIB)
isEmpty(_GSL_LIB){
        error(GSL_LIB environment variable not defined.)
}
INCLUDEPATH += $$_GSL_INCLUDE
LIBPATH     += $$_GSL_LIB
LIBS        += -lgsl
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
VERSION = 6.22

# Define a preprocessor macro so we can get the application version in application code.
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# Define application name macro
DEFINES += APP_NAME=\\\"$$PROGRAM_NAME\\\"

# Define application name and version macro
# \040 means a whitespace.  Don't replace it with an explicit space because
# it doesn't compile.
DEFINES += APP_NAME_VER=\\\"$$PROGRAM_NAME\\\040$$VERSION\\\"

RESOURCES += \
    resources.qrc \
    imagejockey/ijresources.qrc\
    calculator/calcresources.qrc

#set the Windows executable icon
win32:RC_ICONS += art/exeicon.ico
