cmake_minimum_required(VERSION 3.21)
project(gammaray LANGUAGES CXX) #gammaray will be the name of the executable.

# Tell CMake to call Qt's MOC (meta-object compiler) before the C++ compiler on objects containig 
# the Q_OBJECT, SIGNAL, SLOT, CONNECT, etc. macros in its declaration so they are expanded into
# actual C++ code before compiling.
set(CMAKE_AUTOMOC ON)

# Tell CMake to call Qt's RCC (resource compiler) to convert any .qrc files into actual
# C++ source files.
set(CMAKE_AUTORCC ON)

# Tell CMake to call Qt's UIC (user interface compiler) to convert any .ui XML files into 
# actual C++ source files.
set(CMAKE_AUTOUIC ON)

# Find Qt's libraries and headers.
find_package(Qt5 REQUIRED Core CONFIG)

# The calculator script engine is put into a separate CalcScripting.dll/.so library as it 
# takes too long to rebuild due to the large number of symbols.
add_library( CalcScripting SHARED
             calculator/calcscripting.cpp
             calculator/icalcproperty.cpp
             calculator/icalcpropertycollection.cpp )
target_include_directories(CalcScripting PRIVATE thirdparty)
target_link_libraries(CalcScripting PRIVATE Qt5::Core Qt5::Gui)
target_compile_features(CalcScripting PRIVATE cxx_std_14)

# Next, configure GammaRay itself.

# Function to get all propreties that cmake supports. (used to debug script)
if(NOT CMAKE_PROPERTY_LIST)
    execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)
    # Convert command output into a CMake list
    string(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
    string(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
    list(REMOVE_DUPLICATES CMAKE_PROPERTY_LIST)
endif()
    
# This can be used during script debugging to get a list of all CMake properties (normally not used).
function(print_properties)
    message("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction()
#print_properties()
    
# This function lists all properties for a given target (e.g. Qt::Core).
# This is normally used during script debugging, especially when figuring out configuration
# properties of dependencies.
# Credit: https://stackoverflow.com/a/34292622/2153955
function(print_target_properties target)
    if(NOT TARGET ${target})
      message(STATUS "There is no target named '${target}'")
      return()
    endif()

    foreach(property ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" property ${property})

        # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
        if(property STREQUAL "LOCATION" OR property MATCHES "^LOCATION_" OR property MATCHES "_LOCATION$")
            continue()
        endif()

        get_property(was_set TARGET ${target} PROPERTY ${property} SET)
        if(was_set)
            get_target_property(value ${target} ${property})
            message("${target} ${property} = ${value}")
        endif()
    endforeach()
endfunction()
# print_target_properties(Qt5::Core)

# This is the name used to identify the software in Windows registry, for example.
# This is not the executable name.
set(PROGRAM_NAME "GammaRay") 

# The application version.
# Don't forget to update the Util::importSettingsFromPreviousVersion() method to
# enable the import of registry/user settings of previous versions.
set(VERSION "6.22")

# Find the headers and libraries of the dependencies required by GammaRay in addition to Qt.
# The headers and libraries are in lists called targets (e.g. qwt::qwt).
find_package(qwt REQUIRED CONFIG)
find_package(vtk REQUIRED CONFIG)
# find_package(Eigen3 REQUIRED CONFIG) #Eigen is currently directly in /thirdpary source subdirectory
find_package(FFTW3 REQUIRED CONFIG)
find_package(GSL REQUIRED CONFIG)
find_package(Boost REQUIRED CONFIG)
find_package(ITK REQUIRED CONFIG)

# List all the .cpp files needed to build the program.
set(SOURCE_FILES 
    main.cpp
    dialogs/choosevariabledialog.cpp 
    dialogs/contactanalysisdialog.cpp 
    dialogs/driftanalysisdialog.cpp 
    dialogs/faciestransitionmatrixoptionsdialog.cpp 
    dialogs/gridrepositiondialog.cpp 
    dialogs/listbuilderdialog.cpp 
    dialogs/mcrfbayesiansimdialog.cpp 
    dialogs/populatewithproportionsfromvpcdialog.cpp 
    dialogs/sectiondialog.cpp 
    dialogs/subgriddialog.cpp 
    dialogs/transiogrambanddialog.cpp 
    domain/auxiliary/verticalproportioncurvemaker.cpp 
    domain/section.cpp 
    domain/verticalproportioncurve.cpp 
    geometry/boundingbox.cpp 
    geometry/intersectionfinder.cpp 
    geometry/quadrilateral.cpp 
    geometry/triangle.cpp 
    geostats/contactanalysis.cpp 
    geostats/driftanalysis.cpp 
    geostats/mcmcdataimputation.cpp 
    geostats/quadratic3dtrendmodelfitting.cpp 
    geostats/searchannulus.cpp 
    geostats/searchannulusstratigraphic.cpp 
    geostats/searchbox.cpp 
    geostats/searchsphericalshell.cpp 
    geostats/searchverticaldumbbell.cpp 
    geostats/searchwasher.cpp 
    gslib/gslibparameterfiles/kt3dtrendmodelparameters.cpp 
    gslib/gslibparams/gslibpardir.cpp 
    gslib/gslibparams/widgets/widgetgslibpardir.cpp 
    mainwindow.cpp 
    domain/project.cpp 
    domain/application.cpp 
    domain/projectcomponent.cpp 
    domain/objectgroup.cpp 
    domain/projectroot.cpp 
    util.cpp 
    domain/pointset.cpp 
    domain/attribute.cpp 
    domain/file.cpp 
    gs/ghostscript.cpp 
    gslib/gslib.cpp 
    gslib/gslibparams/gslibpartype.cpp 
    gslib/gslibparams/gslibparinputdata.cpp 
    gslib/gslibparams/gslibparvarweight.cpp 
    gslib/gslibparams/gslibparfile.cpp 
    gslib/gslibparams/gslibparlimitsdouble.cpp 
    gslib/gslibparameterfiles/gslibparameterfile.cpp 
    gslib/gslibparams/gslibpardouble.cpp 
    gslib/gslibparams/gslibparuint.cpp 
    gslib/gslibparams/gslibparoption.cpp 
    gslib/gslibparams/gslibparint.cpp 
    gslib/gslibparams/gslibparstring.cpp 
    gslib/gslibparams/gslibparrange.cpp 
    gslib/gslibparams/widgets/widgetgslibpardouble.cpp 
    gslib/gslibparams/widgets/widgetgslibparfile.cpp 
    gslib/gslibparams/widgets/widgetgslibparinputdata.cpp 
    gslib/gslibparams/gslibparmultivaluedfixed.cpp 
    domain/datafile.cpp 
    gslib/gslibparametersdialog.cpp 
    gslib/gslibparams/gslibparmultivaluedvariable.cpp 
    gslib/gslibparams/widgets/widgetgslibparmultivaluedvariable.cpp 
    gslib/gslibparams/widgets/widgetgslibparint.cpp 
    gslib/gslibparams/widgets/widgetgslibparlimitsdouble.cpp 
    gslib/gslibparams/widgets/widgetgslibparmultivaluedfixed.cpp 
    gslib/gslibparams/widgets/widgetgslibparuint.cpp 
    gslib/gslibparams/widgets/widgetgslibparstring.cpp 
    gslib/gslibparams/widgets/widgetgslibparoption.cpp 
    gslib/gslibparams/widgets/widgetgslibparrange.cpp 
    domain/cartesiangrid.cpp 
    gslib/gslibparams/gslibpargrid.cpp 
    gslib/gslibparams/gslibparcolor.cpp 
    gslib/gslibparams/gslibparrepeat.cpp 
    gslib/gslibparams/widgets/widgetgslibpargrid.cpp 
    gslib/gslibparams/widgets/widgetgslibparrepeat.cpp 
    gslib/gslibparams/widgets/widgetgslibparcolor.cpp 
    gslib/igslibparameterfinder.cpp 
    gslib/workerthread.cpp 
    domain/plot.cpp 
    domain/experimentalvariogram.cpp 
    domain/variogrammodel.cpp 
    vertpropcurves/verticalproportioncurvedialog.cpp 
    vertpropcurves/verticalproportioncurvescanvaspicker.cpp 
    vertpropcurves/verticalproportioncurvesplot.cpp 
    viewer3d/v3dmouseinteractor.cpp 
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeascontourlines.cpp 
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinpointset.cpp 
    viewer3d/view3dtextconfigwidget.cpp 
    widgets/linechartwidget.cpp 
    widgets/listbuilder.cpp 
    widgets/qlistwidgetdnd.cpp 
    widgets/transiogrambandchartview.cpp 
    widgets/variablelistbuilder.cpp 
    widgets/variogrammodellist.cpp 
    domain/weight.cpp 
    domain/normalvariable.cpp 
    domain/univariatedistribution.cpp 
    widgets/univariatedistributionselector.cpp 
    domain/distributioncolumn.cpp 
    domain/roles.cpp 
    domain/distribution.cpp 
    widgets/distributioncolumnroleselector.cpp 
    domain/bivariatedistribution.cpp 
    widgets/variogrammodelselector.cpp 
    widgets/cartesiangridselector.cpp 
    widgets/pointsetselector.cpp 
    widgets/variableselector.cpp 
    widgets/qlabelwithcrosshairs.cpp 
    domain/valuepairs.cpp 
    domain/thresholdcdf.cpp 
    domain/categorypdf.cpp 
    widgets/valuepairvertical.cpp 
    widgets/fileselectorwidget.cpp 
    gslib/gslibparams/gslibparvmodel.cpp 
    gslib/gslibparams/widgets/widgetgslibparvmodel.cpp 
    domain/triads.cpp 
    domain/categorydefinition.cpp 
    domain/univariatecategoryclassification.cpp 
    widgets/categoryselector.cpp 
    widgets/intervalandcategorywidget.cpp 
    softindiccalib/softindicatorcalibrationdialog.cpp 
    softindiccalib/softindicatorcalibplot.cpp 
    softindiccalib/softindicatorcalibcanvaspicker.cpp 
    dialogs/cokrigingdialog.cpp 
    dialogs/aboutdialog.cpp 
    dialogs/variogramanalysisdialog.cpp 
    dialogs/valuespairsdialog.cpp 
    dialogs/triadseditordialog.cpp 
    dialogs/setupdialog.cpp 
    dialogs/postikdialog.cpp 
    dialogs/pointsetdialog.cpp 
    dialogs/nscoredialog.cpp 
    dialogs/krigingdialog.cpp 
    dialogs/indicatorkrigingdialog.cpp 
    dialogs/filecontentsdialog.cpp 
    dialogs/distributionmodelingdialog.cpp 
    dialogs/distributioncolumnrolesdialog.cpp 
    dialogs/displayplotdialog.cpp 
    dialogs/declusteringdialog.cpp 
    dialogs/datafiledialog.cpp 
    dialogs/creategriddialog.cpp 
    dialogs/cartesiangriddialog.cpp 
    dialogs/bidistributionmodelingdialog.cpp 
    widgets/pswidget.cpp 
    viewer3d/view3dwidget.cpp 
    widgets/projecttreeview.cpp 
    viewer3d/viewer3dlistwidget.cpp 
    viewer3d/view3dbuilders.cpp 
    viewer3d/view3dcolortables.cpp 
    viewer3d/view3dconfigwidget.cpp 
    viewer3d/view3dconfigwidgetsbuilder.cpp 
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributein3dcartesiangrid.cpp 
    viewer3d/view3dlistrecord.cpp 
    viewer3d/view3dviewdata.cpp 
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinmapcartesiangrid.cpp 
    domain/auxiliary/dataloader.cpp 
    array3d.cpp 
    geostats/geostatsutils.cpp 
    geostats/matrix3x3.cpp 
    geostats/matrixmxn.cpp 
    dialogs/ndvestimationdialog.cpp 
    geostats/gridcell.cpp 
    geostats/ndvestimation.cpp 
    geostats/spatiallocation.cpp 
    geostats/ndvestimationrunner.cpp 
    geostats/ijkdelta.cpp 
    geostats/ijkindex.cpp 
    geostats/ijkdeltascache.cpp 
    dialogs/realizationselectiondialog.cpp 
    dialogs/gridresampledialog.cpp 
    dialogs/multivariogramdialog.cpp 
    imagejockey/imagejockeydialog.cpp 
    imagejockey/imagejockeygridplot.cpp 
    imagejockey/spectrogram1dparameters.cpp 
    imagejockey/spectrogram1dplot.cpp 
    imagejockey/spectrogram1dplotpicker.cpp 
    imagejockey/equalizer/equalizerwidget.cpp 
    imagejockey/equalizer/equalizerslider.cpp 
    dialogs/sgsimdialog.cpp 
    widgets/distributionfieldselector.cpp 
    viewer3d/view3dverticalexaggerationwidget.cpp 
    widgets/focuswatcher.cpp 
    spectral/svd.cpp 
    spectral/pca.cpp 
    spectral/spectral.cpp 
    algorithms/ialgorithmdatasource.cpp 
    algorithms/bootstrap.cpp 
    dialogs/machinelearningdialog.cpp 
    algorithms/CART/cart.cpp 
    algorithms/CART/cartdecisionnode.cpp 
    algorithms/CART/cartleafnode.cpp 
    algorithms/CART/cartnode.cpp 
    algorithms/CART/cartsplitcriterion.cpp 
    algorithms/randomforest.cpp 
    algorithms/decisiontree.cpp 
    domain/auxiliary/variableremover.cpp 
    domain/auxiliary/datasaver.cpp 
    imagejockey/svd/svdparametersdialog.cpp 
    imagejockey/svd/svdfactor.cpp 
    imagejockey/svd/svdfactortree.cpp 
    imagejockey/svd/svdanalysisdialog.cpp 
    imagejockey/svd/svdfactortreeview.cpp 
    imagejockey/svd/svdfactorsel/svdfactorsselectiondialog.cpp 
    imagejockey/svd/svdfactorsel/svdfactorsselectionchartview.cpp 
    imagejockey/svd/svdfactorsel/svdfactorsselectionchartcallout.cpp 
    imagejockey/ijabstractcartesiangrid.cpp 
    imagejockey/ijabstractvariable.cpp 
    imagejockey/imagejockeyutils.cpp 
    imagejockey/ijexperimentalvariogramparameters.cpp 
    imagejockey/ijmatrix3x3.cpp 
    imagejockey/ijspatiallocation.cpp 
    imagejockey/widgets/ijcartesiangridselector.cpp 
    imagejockey/widgets/ijvariableselector.cpp 
    imagejockey/widgets/grcompass.cpp 
    imagejockey/widgets/ijgridviewerwidget.cpp 
    calculator/calculatordialog.cpp 
    calculator/calclinenumberarea.cpp 
    calculator/calccodeeditor.cpp 
    imagejockey/vardecomp/variographicdecompositiondialog.cpp 
    imagejockey/widgets/ijquick3dviewer.cpp 
    dialogs/factorialkrigingdialog.cpp 
    geostats/fkestimation.cpp 
    geostats/searchstrategy.cpp 
    geostats/fkestimationrunner.cpp 
    geostats/datacell.cpp 
    geostats/searchneighborhood.cpp 
    geostats/searchellipsoid.cpp 
    geostats/pointsetcell.cpp 
    geostats/indexedspatiallocation.cpp 
    domain/geogrid.cpp 
    domain/gridfile.cpp 
    domain/auxiliary/meshloader.cpp 
    geometry/vector3d.cpp 
    geometry/face3d.cpp 
    dialogs/sisimdialog.cpp 
    dialogs/variograminputdialog.cpp 
    geometry/hexahedron.cpp 
    geometry/pyramid.cpp 
    geometry/tetrahedron.cpp 
    imagejockey/emd/emdanalysisdialog.cpp 
    imagejockey/gabor/gaborfilterdialog.cpp 
    imagejockey/gabor/gaborscandialog.cpp 
    imagejockey/gabor/gaborutils.cpp 
    imagejockey/gabor/gaborfrequencyazimuthselections.cpp 
    imagejockey/wavelet/wavelettransformdialog.cpp 
    imagejockey/wavelet/waveletutils.cpp 
    imagejockey/ijvariographicmodel2d.cpp 
    dialogs/automaticvarfitdialog.cpp 
    dialogs/emptydialog.cpp 
    geostats/nestedvariogramstructuresparameters.cpp 
    geostats/automaticvariogramfitting.cpp 
    dialogs/automaticvarfitexperimentsdialog.cpp 
    dialogs/segmentsetdialog.cpp 
    domain/segmentset.cpp 
    domain/quintuplets.cpp 
    gslib/gslibparams/gslibparcustomcolor.cpp 
    gslib/gslibparams/widgets/widgetgslibcustomcolor.cpp 
    dialogs/choosecategorydialog.cpp 
    domain/faciestransitionmatrix.cpp 
    dialogs/projectfilechoosedialog.cpp 
    dialogs/entropycyclicityanalysisdialog.cpp 
    dialogs/faciesrelationshipdiagramdialog.cpp 
    graphviz/graphviz.cpp 
    dialogs/transiogramdialog.cpp 
    domain/auxiliary/faciestransitionmatrixmaker.cpp 
    domain/auxiliary/thicknesscalculator.cpp 
    widgets/transiogramchartview.cpp 
    domain/verticaltransiogrammodel.cpp 
    dialogs/dynamicfaciesrelationshipdiagramdialog.cpp 
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinsegmentset.cpp 
    geostats/segmentsetcell.cpp 
    domain/auxiliary/valuestransferer.cpp 
    dialogs/mcrfsimdialog.cpp 
    dialogs/lvadatasetdialog.cpp 
    geostats/mcrfsim.cpp 
    gslib/gslibparameterfiles/commonsimulationparameters.cpp 
    spatialindex/spatialindex.cpp 
    geostats/taumodel.cpp 
    dialogs/mcmcdataimputationdialog.cpp 
    imagejockey/paraviewscalarbar/vtkBoundingRectContextDevice2D.cpp 
    imagejockey/paraviewscalarbar/vtkContext2DScalarBarActor.cpp 
    imagejockey/paraviewscalarbar/vtkParaViewScalarBar.cpp 
    imagejockey/paraviewscalarbar/vtkPVScalarBarRepresentation.cpp )

# List all the Qt .ui files to be converted to C++ code by Qt's UIC compiler.
set ( UI_FILES
    mainwindow.ui 
    dialogs/choosevariabledialog.ui 
    dialogs/contactanalysisdialog.ui 
    dialogs/driftanalysisdialog.ui 
    dialogs/faciestransitionmatrixoptionsdialog.ui 
    dialogs/gridrepositiondialog.ui 
    dialogs/listbuilderdialog.ui 
    dialogs/mcrfbayesiansimdialog.ui 
    dialogs/populatewithproportionsfromvpcdialog.ui 
    dialogs/sectiondialog.ui 
    dialogs/subgriddialog.ui 
    gslib/gslibparams/widgets/widgetgslibpardir.ui 
    gslib/gslibparams/widgets/widgetgslibpardouble.ui 
    gslib/gslibparams/widgets/widgetgslibparfile.ui 
    gslib/gslibparams/widgets/widgetgslibparinputdata.ui 
    gslib/gslibparametersdialog.ui 
    gslib/gslibparams/widgets/widgetgslibparmultivaluedvariable.ui 
    gslib/gslibparams/widgets/widgetgslibparint.ui 
    gslib/gslibparams/widgets/widgetgslibparlimitsdouble.ui 
    gslib/gslibparams/widgets/widgetgslibparmultivaluedfixed.ui 
    gslib/gslibparams/widgets/widgetgslibparuint.ui 
    gslib/gslibparams/widgets/widgetgslibparstring.ui 
    gslib/gslibparams/widgets/widgetgslibparoption.ui 
    gslib/gslibparams/widgets/widgetgslibparrange.ui 
    gslib/gslibparams/widgets/widgetgslibpargrid.ui 
    gslib/gslibparams/widgets/widgetgslibparrepeat.ui 
    gslib/gslibparams/widgets/widgetgslibparcolor.ui 
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeascontourlines.ui 
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinpointset.ui 
    viewer3d/view3dtextconfigwidget.ui 
    widgets/linechartwidget.ui 
    vertpropcurves/verticalproportioncurvedialog.ui 
    widgets/variablelistbuilder.ui 
    widgets/variogrammodellist.ui 
    widgets/univariatedistributionselector.ui 
    widgets/distributioncolumnroleselector.ui 
    widgets/variogrammodelselector.ui 
    widgets/cartesiangridselector.ui 
    widgets/pointsetselector.ui 
    widgets/variableselector.ui 
    widgets/valuepairvertical.ui 
    widgets/fileselectorwidget.ui 
    gslib/gslibparams/widgets/widgetgslibparvmodel.ui 
    widgets/categoryselector.ui 
    widgets/intervalandcategorywidget.ui 
    softindiccalib/softindicatorcalibrationdialog.ui 
    dialogs/cokrigingdialog.ui 
    dialogs/aboutdialog.ui 
    dialogs/bidistributionmodelingdialog.ui 
    dialogs/cartesiangriddialog.ui 
    dialogs/creategriddialog.ui 
    dialogs/datafiledialog.ui 
    dialogs/declusteringdialog.ui 
    dialogs/displayplotdialog.ui 
    dialogs/distributioncolumnrolesdialog.ui 
    dialogs/distributionmodelingdialog.ui 
    dialogs/filecontentsdialog.ui 
    dialogs/indicatorkrigingdialog.ui 
    dialogs/krigingdialog.ui 
    dialogs/nscoredialog.ui 
    dialogs/pointsetdialog.ui 
    dialogs/postikdialog.ui 
    dialogs/setupdialog.ui 
    dialogs/triadseditordialog.ui 
    dialogs/valuespairsdialog.ui 
    dialogs/variogramanalysisdialog.ui 
    widgets/pswidget.ui 
    viewer3d/view3dwidget.ui 
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributein3dcartesiangrid.ui 
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinmapcartesiangrid.ui 
    dialogs/ndvestimationdialog.ui 
    dialogs/realizationselectiondialog.ui 
    dialogs/gridresampledialog.ui 
    dialogs/multivariogramdialog.ui 
    imagejockey/imagejockeydialog.ui 
    imagejockey/equalizer/equalizerwidget.ui 
    imagejockey/equalizer/equalizerslider.ui 
    dialogs/sgsimdialog.ui 
    widgets/distributionfieldselector.ui 
    viewer3d/view3dverticalexaggerationwidget.ui 
    dialogs/machinelearningdialog.ui 
    imagejockey/svd/svdparametersdialog.ui 
    imagejockey/svd/svdanalysisdialog.ui 
    imagejockey/svd/svdfactorsel/svdfactorsselectiondialog.ui 
    imagejockey/widgets/ijcartesiangridselector.ui 
    imagejockey/widgets/ijvariableselector.ui 
    imagejockey/widgets/ijgridviewerwidget.ui 
    calculator/calculatordialog.ui 
    imagejockey/vardecomp/variographicdecompositiondialog.ui 
    imagejockey/widgets/ijquick3dviewer.ui 
    dialogs/factorialkrigingdialog.ui 
    dialogs/sisimdialog.ui 
    dialogs/variograminputdialog.ui 
    imagejockey/emd/emdanalysisdialog.ui 
    imagejockey/gabor/gaborfilterdialog.ui 
    imagejockey/gabor/gaborscandialog.ui 
    imagejockey/wavelet/wavelettransformdialog.ui 
    dialogs/automaticvarfitdialog.ui 
    dialogs/emptydialog.ui 
    dialogs/automaticvarfitexperimentsdialog.ui 
    dialogs/segmentsetdialog.ui 
    gslib/gslibparams/widgets/widgetgslibcustomcolor.ui 
    dialogs/choosecategorydialog.ui 
    dialogs/projectfilechoosedialog.ui 
    dialogs/entropycyclicityanalysisdialog.ui 
    dialogs/faciesrelationshipdiagramdialog.ui 
    dialogs/transiogramdialog.ui 
    dialogs/dynamicfaciesrelationshipdiagramdialog.ui 
    viewer3d/view3dconfigwidgets/v3dcfgwidforattributeinsegmentset.ui 
    dialogs/mcrfsimdialog.ui 
    dialogs/lvadatasetdialog.ui 
    dialogs/mcmcdataimputationdialog.ui
)

# List all Qt resource files to be converted to C++ code by Qt's RCC compiler.
set( QRC_FILES
     resources.qrc
     imagejockey/ijresources.qrc
     calculator/calcresources.qrc )

# Set the executable (target) and which source files to use to build it.
add_executable(${PROJECT_NAME} ${UI_FILES} ${SOURCE_FILES} ${QRC_FILES})

# Set the pre-processor definitions needed to compile the program.
# ATTENTION: do not remove the quotation marks below because these definitions are expanded
#            into string literals in the code.
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_NAME="${PROGRAM_NAME}")
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${VERSION}")
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_NAME_VER="${PROGRAM_NAME} ${VERSION}")

# Alows to find the program's own header files referenced in the .cpp source files.
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# Alows to find in-source third party header files referenced in the .cpp source files.
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

# Tell which libraries to link against.
# The targets listes here also resolve include directories for each of the dependency or dependency component.
# Also see the find_package()'s further above and the requirements(self) callback in
# conanfile.py if you're building with Conan.
# If you build with Conan, its output messages list all of declared component targets.
# The libraries (and headers) are in lists identified by targets containing everything (qwt::qwt) or
# component targets (e.g.: Qt5::Core) containing part of a dependency.
target_link_libraries( ${PROJECT_NAME} PRIVATE 
                       CalcScripting # This target is configured in this CMakeLists.txt (see further up)
                       qwt::qwt 
                       Qt5::Core 
                       Qt5::Gui 
                       Qt5::Charts # not everything in Qt is required
                       vtk::vtk # it seems VTK target has only one component target, unlike Qt, Boost and others.
                       # Eigen3::Eigen #Eigen is currently directly in /thirdparty source subdirectory
                       FFTW3::fftw3
                       FFTW3::fftw3f # FFTW3::fftw3l also exists, but it is not required
                       Boost::headers #Boost::boost also exists, but GammaRay currently only needs the header-only Boost libraries
                       GSL::gsl # GSL::gslcblas also exists, but it is not required
                       ITKCommon       # \
                       ITKIOImageBase  # |
                       itkvnl          # |
                       ITKIOPNG        #  > ITK seems to declare an entire component target for each single library.
                       ITKTransform    # |  
                       ITKSmoothing    # |
                       ITKConvolution  # |
                       ITK::itksys     # /
                     )

# Due to Boost 1.83 header libraries, GammaRay needs to be compiled with the C++14 standard,
# even though its own code is C++11.
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)

#if(NOT WIN32)
    # Must compile with "-fPIC" since Qt was built with -reduce-relocations.
    #target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
#endif()

# Get Qt's installation directory.
# ATTENTION: This assumes Qt has only one directory where the DLLs/SOs are (normally 
#            <Qt's install prefix>/lib).
# ATTENTION 2: The returned QT_LIB_DIR may contain generator expressions ($<...> syntax),
#              which are evaluated into valid information AFTER generation time. That is,
#              trying to evaluate it during configuration or generation time may result into
#              invalid values (e.g. trying to do a message(...)).
#              Functions such as install() are executed AFTER generation time, hence it is safe
#              to use the QT_LIB_DIR local variable with them.
get_target_property(QT_LIB_DIR Qt5::Core INTERFACE_LINK_DIRECTORIES)

# Declare what is to be distributed to users (program executable and required .DLLs/.SOs).
install(TARGETS CalcScripting DESTINATION .) #default dest. is lib subdirectory
install(TARGETS ${PROJECT_NAME} RUNTIME_DEPENDENCIES DESTINATION .)
#install(DIRECTORY   ${QT_LIB_DIR}/../lib/   --> the main Qt libs are already installed by the above install() commands.
#        DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
#        USE_SOURCE_PERMISSIONS)    
# It is necessary to copy some of the Qt plug-in DLLs/SOs too.  These libraries are not linked
# against GammaRay executable but loaded during runtime by Qt. Hence, they are not figured out by CMake
# in a install( ... RUNTIME_DEPENDENCIES ...) command.
install(DIRECTORY   ${QT_LIB_DIR}/../plugins/imageformats/
        DESTINATION imageformats
        USE_SOURCE_PERMISSIONS) 
install(DIRECTORY   ${QT_LIB_DIR}/../plugins/platforms/
        DESTINATION platforms
        USE_SOURCE_PERMISSIONS) 
