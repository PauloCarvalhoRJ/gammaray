import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import download, unzip, replace_in_file, copy, get

# This recipe builds, installs and tests the VTK library (Visualization Toolkit).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class VTKConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "vtk"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "9.3.1"
    # A brief description of what the dependency does.
    description = "Visualization Toolkit by Kitware"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "library"
    # generators = "cmake", "virtualenv"
    settings = "os", "compiler", "build_type", "arch"
    # A set of additional custom options that are necessary to build the dependency.
    options = {"shared": [True, False],
               "qt": [True, False],
               "python": [True, False],
               "mpi": [True, False],
               "fPIC": [True, False],
               "boost_version": ['1.65', '1.68', '1.71', '1.83'],
               "os_version": ["linux", "windows", "other"]}
    # Declare which are the default values to the options in the options attribute.
    default_options = { 
        "shared": True, 
        "qt": True, 
        "python": False, 
        "mpi": False, 
        "fPIC": False, 
        "boost_version": "1.83" }
    # Copy/distribute VTK's configure files.
    exports = ["CMakeLists.txt", "FindVTK.cmake", "*.diff"]
    # The original VTK 8.2 Conan 1 recipe.
    url="http://github.com/bilke/conan-vtk"
    # Declare the license of the dependency.
    license="http://www.vtk.org/licensing/"
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    #exports_sources = "VTK-{}.tar.gz".format(version)

    # Non-Conan API attributes.
    version_split = version.split('.')
    short_version = "%s.%s" % (version_split[0], version_split[1])
    SHORT_VERSION = short_version
    source_url = "https://www.vtk.org/files/release/9.3/VTK-9.3.1.tar.gz"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def python_library(self):
        if self.settings.os == "Linux":
            return os.path.join(self.dependencies["python3"].cpp_info.libdirs[0], \
                             "libpython{}m.so".format(self.dependencies["python3"].cpp_info.version[0:-2]))
        return os.path.join(self.dependencies["python3"].cpp_info.libdirs[0], "..", "libs", \
                             "python{}.lib".format(self.dependencies["python3"].cpp_info.version[0:-2].replace(".", ""))).replace("\\", "/")                

    @property
    def python_executable(self):
        if self.settings.os == "Linux":
            return os.path.join(self.dependencies["python3"].cpp_info.bindirs[0], "python")
        return os.path.join(self.dependencies["python3"].cpp_info.bindirs[0], "..", "python.exe").replace("\\", "/")

    @property
    def source_subfolder(self):
        return "VTK-{}".format(self.version)

    @property
    def source_zip_filename(self):
        return "{}.tar.gz".format(self.source_subfolder)

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    def requirements(self):
        self.requires("boost/[>=%s]" % self.options.boost_version)
        if self.options.python:
            self.requires("python3/[>=3.10]")
        if self.options.qt:
            self.requires("qt/[>=5.15]")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #unzip(self, filename=os.path.join(self.source_folder, self.source_zip_filename), destination=".")
        #copy(self, pattern="*", src=os.path.join(self.source_folder, self.source_subfolder), dst=".")
        get(self, **self.conan_data["sources"][self.version],
            strip_root=True, destination=".")

    # Configure or constrain the available options in a package before assigning them a value. A typical use case is
    # to remove an option in a given platform. For example, the SSE2 flag doesn’t exist in architectures different 
    # than 32 bits, so it should be removed in this method.
    def config_options(self):
        if self.settings.compiler == "msvc":
            self.options.rm_safe("fPIC")
            self.options.os_version = "windows"
        elif self.settings.os == "Linux":
            self.options.os_version = "linux"
        else:
            self.options.os_version = "other"

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        cmake_layout(self)

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):  
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.cache_variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.cache_variables["VTK_WRAP_PYTHON"] = self.options.python
        tc.cache_variables["VTK_BUILD_TESTING"] = "OFF"
        tc.cache_variables["VTK_BUILD_EXAMPLES"] = "OFF"
        tc.cache_variables["VTK_BUILD_DOCUMENTATION"] = "OFF"
        if self.options.shared == False:
            tc.cache_variables["BUILD_SHARED_LIBS"] = "NO"
        if self.options.qt == True:
            tc.cache_variables["VTK_GROUP_ENABLE_Qt"] = "YES"
            tc.cache_variables["VTK_QT_VERSION"] = "5"
            tc.cache_variables["QT_QMAKE_EXECUTABLE"] = self.dependencies["qt"].cpp_info.bindirs[0].replace("\\", "/") + \
                                                           ("/qmake.exe" if (self.settings.os == "Windows") else "/qmake")
            tc.cache_variables["VTK_BUILD_QT_DESIGNER_PLUGIN"] = False
            tc.cache_variables["VTK_MODULE_ENABLE_VTK_GUISupportQtQuick"] = "NO" # Enabling this gives odd "The plugin '.../qmlvtkplugin.dll' 
                                                                                 # uses incompatible Qt library. (Cannot mix debug and release libraries.)
                                                                                 # Even though Qt and VTK are being built consistently in mode.
                                                                                 # Fortunately, QtQuick is not needed by GammaRay.
        if self.options.python == True:
            tc.cache_variables["VTK_WRAP_PYTHON"] = True
            tc.cache_variables["VTK_PYTHON_VERSION"] = self.dependencies["python3"].version[0:3]
            tc.cache_variables["PYTHON_EXECUTABLE"]= self.python_executable
            tc.cache_variables["PYTHON_LIBRARY"] = self.python_library
        if self.options.mpi == True:
            tc.cache_variables["VTK_Group_MPI"] = True
        if self.settings.compiler != "msvc":
            if self.options.get_safe("fPIC", False): #fPIC is removed if options.shared==True (see the configure() callback).
                tc.cache_variables["CMAKE_POSITION_INDEPENDENT_CODE"] = True
        tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        cmake = CMake(self)
        cmake.configure()

        #patches = ["fix_compilation_with_gcc10.diff"]
        #for patch in patches:
        #    tools.patch(os.path.join(self.build_folder, self.source_subfolder), os.path.join(self.source_folder, patch))

        # It seems that these Qt macros have been replaced with less clash-prone Q_SIGNALS and Q_SLOTS macros in VTK 9.
        # Changing Qt definitions for macros.
        # Qt's signals: and slots: macros can conflict with Boost definitions.
        #files = ["Views/Qt/vtkQtTreeView.h", \
        #         "GUISupport/QtOpenGL/QVTKWidget2.h", \
        #         "GUISupport/Qt/vtkQtAbstractModelAdapter.h", \
        #         "GUISupport/Qt/vtkQtConnection.h", \
        #         "GUISupport/Qt/vtkQtTableModelAdapter.h", \
        #         "GUISupport/Qt/QVTKOpenGLWidget.h", \
        #         "GUISupport/Qt/QVTKOpenGLNativeWidget.h", \
        #         "GUISupport/Qt/vtkTDxQtUnixDevices.h", \
        #         "GUISupport/Qt/QVTKOpenGLWindow.h"]
        #for f in files:
        #    for key, macro in zip(["signals:", "slots:"], ["Q_SIGNALS:", "Q_SLOTS:"]):
        #        replace_in_file(self, os.path.join(self.source_folder, f), key, macro, strict=False )

        cmake.build()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        cmake = CMake(self)
        cmake.install()

        # Fixing python libraries in VTKTargets.cmake
        if self.options.python:
            cmake_file = os.path.join(self.package_folder, "lib", "cmake", "vtk-{}".format(self.version[0:-2]), "VTKTargets.cmake")
            replace_in_file(self, cmake_file,
                            "add_library(vtkPythonInterpreter SHARED IMPORTED)",
                            "add_library(vtkPythonInterpreter SHARED IMPORTED)\n\nfind_package(Python3 COMPONENTS Development)")
            self.output.warn(self.python_library)
            # FIXME: This call used to be replace_path_in_file() for Conan 1.  We hope the paths don't mismatch letter case.
            #        So, it would be good to have a Conan 2 replacement.  
            #        See https://github.com/conan-io/conan/issues/16779
            replace_in_file(self, cmake_file, self.python_library, "Python3::Python")

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        libs = [
            "vtkChartsCore-%s" % self.short_version,
            "vtkCommonColor-%s" % self.short_version,
            "vtkCommonComputationalGeometry-%s" % self.short_version,
            "vtkCommonCore-%s" % self.short_version,
            "vtkCommonDataModel-%s" % self.short_version,
            "vtkCommonExecutionModel-%s" % self.short_version,
            "vtkCommonMath-%s" % self.short_version,
            "vtkCommonMisc-%s" % self.short_version,
            "vtkCommonSystem-%s" % self.short_version,
            "vtkCommonTransforms-%s" % self.short_version,
            "vtkDICOMParser-%s" % self.short_version,
            "vtkDomainsChemistry-%s" % self.short_version,
            "vtkDomainsChemistryOpenGL2-%s" % self.short_version,
            "vtkFiltersAMR-%s" % self.short_version,
            "vtkFiltersCellGrid-%s" % self.short_version,
            "vtkFiltersCore-%s" % self.short_version,
            "vtkFiltersExtraction-%s" % self.short_version,
            "vtkFiltersFlowPaths-%s" % self.short_version,
            "vtkFiltersGeneral-%s" % self.short_version,
            "vtkFiltersGeneric-%s" % self.short_version,
            "vtkFiltersGeometry-%s" % self.short_version,
            "vtkFiltersGeometryPreview-%s" % self.short_version,
            "vtkFiltersHybrid-%s" % self.short_version,
            "vtkFiltersHyperTree-%s" % self.short_version,
            "vtkFiltersImaging-%s" % self.short_version,
            "vtkFiltersModeling-%s" % self.short_version,
            "vtkFiltersParallel-%s" % self.short_version,
            "vtkFiltersParallelImaging-%s" % self.short_version,
            "vtkFiltersPoints-%s" % self.short_version,
            "vtkFiltersProgrammable-%s" % self.short_version,
            "vtkFiltersReduction-%s" % self.short_version,
            "vtkFiltersSMP-%s" % self.short_version,
            "vtkFiltersSelection-%s" % self.short_version,
            "vtkFiltersSources-%s" % self.short_version,
            "vtkFiltersStatistics-%s" % self.short_version,
            "vtkFiltersTensor-%s" % self.short_version,
            "vtkFiltersTexture-%s" % self.short_version,
            "vtkFiltersTopology-%s" % self.short_version,
            "vtkFiltersVerdict-%s" % self.short_version,
            "vtkGeovisCore-%s" % self.short_version,
            "vtkIOAMR-%s" % self.short_version,
            "vtkIOAsynchronous-%s" % self.short_version,
            "vtkIOCGNSReader-%s" % self.short_version,
            "vtkIOCONVERGECFD-%s" % self.short_version,
            "vtkIOCellGrid-%s" % self.short_version,
            "vtkIOCesium3DTiles-%s" % self.short_version,
            "vtkIOChemistry-%s" % self.short_version,
            "vtkIOCityGML-%s" % self.short_version,
            "vtkIOCore-%s" % self.short_version,
            "vtkIOEnSight-%s" % self.short_version,
            "vtkIOExodus-%s" % self.short_version,
            "vtkIOExport-%s" % self.short_version,
            "vtkIOExportGL2PS-%s" % self.short_version,
            "vtkIOExportPDF-%s" % self.short_version,
            "vtkIOFLUENTCFF-%s" % self.short_version,
            "vtkIOGeometry-%s" % self.short_version,
            "vtkIOHDF-%s" % self.short_version,
            "vtkIOIOSS-%s" % self.short_version,
            "vtkIOImage-%s" % self.short_version,
            "vtkIOImport-%s" % self.short_version,
            "vtkIOInfovis-%s" % self.short_version,
            "vtkIOLSDyna-%s" % self.short_version,
            "vtkIOLegacy-%s" % self.short_version,
            "vtkIOMINC-%s" % self.short_version,
            "vtkIOMotionFX-%s" % self.short_version,
            "vtkIOMovie-%s" % self.short_version,
            "vtkIONetCDF-%s" % self.short_version,
            "vtkIOOggTheora-%s" % self.short_version,
            "vtkIOPLY-%s" % self.short_version,
            "vtkIOParallel-%s" % self.short_version,
            "vtkIOParallelXML-%s" % self.short_version,
            "vtkIOSQL-%s" % self.short_version,
            "vtkIOSegY-%s" % self.short_version,
            "vtkIOTecplotTable-%s" % self.short_version,
            "vtkIOVeraOut-%s" % self.short_version,
            "vtkIOVideo-%s" % self.short_version,
            "vtkIOXML-%s" % self.short_version,
            "vtkIOXMLParser-%s" % self.short_version,
            "vtkImagingColor-%s" % self.short_version,
            "vtkImagingCore-%s" % self.short_version,
            "vtkImagingFourier-%s" % self.short_version,
            "vtkImagingGeneral-%s" % self.short_version,
            "vtkImagingHybrid-%s" % self.short_version,
            "vtkImagingMath-%s" % self.short_version,
            "vtkImagingMorphological-%s" % self.short_version,
            "vtkImagingSources-%s" % self.short_version,
            "vtkImagingStatistics-%s" % self.short_version,
            "vtkImagingStencil-%s" % self.short_version,
            "vtkInfovisCore-%s" % self.short_version,
            "vtkInfovisLayout-%s" % self.short_version,
            "vtkInteractionImage-%s" % self.short_version,
            "vtkInteractionStyle-%s" % self.short_version,
            "vtkInteractionWidgets-%s" % self.short_version,
            "vtkParallelCore-%s" % self.short_version,
            "vtkParallelDIY-%s" % self.short_version,
            "vtkRenderingAnnotation-%s" % self.short_version,
            "vtkRenderingCellGrid-%s" % self.short_version,
            "vtkRenderingContext2D-%s" % self.short_version,
            "vtkRenderingContextOpenGL2-%s" % self.short_version,
            "vtkRenderingCore-%s" % self.short_version,
            "vtkRenderingFreeType-%s" % self.short_version,
            "vtkRenderingGL2PSOpenGL2-%s" % self.short_version,
            "vtkRenderingHyperTreeGrid-%s" % self.short_version,
            "vtkRenderingImage-%s" % self.short_version,
            "vtkRenderingLICOpenGL2-%s" % self.short_version,
            "vtkRenderingLOD-%s" % self.short_version,
            "vtkRenderingLabel-%s" % self.short_version,
            "vtkRenderingOpenGL2-%s" % self.short_version,
            "vtkRenderingSceneGraph-%s" % self.short_version,
            "vtkRenderingUI-%s" % self.short_version,
            "vtkRenderingVolume-%s" % self.short_version,
            "vtkRenderingVolumeOpenGL2-%s" % self.short_version,
            "vtkRenderingVtkJS-%s" % self.short_version,
            "vtkTestingRendering-%s" % self.short_version,
            "vtkViewsContext2D-%s" % self.short_version,
            "vtkViewsCore-%s" % self.short_version,
            "vtkViewsInfovis-%s" % self.short_version,
            "vtkWrappingTools-%s" % self.short_version,
            "vtkcgns-%s" % self.short_version,
            "vtkdoubleconversion-%s" % self.short_version,
            "vtkexodusII-%s" % self.short_version,
            "vtkexpat-%s" % self.short_version,
            "vtkfmt-%s" % self.short_version,
            "vtkfreetype-%s" % self.short_version,
            "vtkgl2ps-%s" % self.short_version,
            "vtkglew-%s" % self.short_version,
            "vtkhdf5-%s" % self.short_version,
            "vtkhdf5_hl-%s" % self.short_version,
            "vtkioss-%s" % self.short_version,
            "vtkjpeg-%s" % self.short_version,
            "vtkjsoncpp-%s" % self.short_version,
            "vtkkissfft-%s" % self.short_version,
            "vtklibharu-%s" % self.short_version,
            "vtklibproj-%s" % self.short_version,
            "vtklibxml2-%s" % self.short_version,
            "vtkloguru-%s" % self.short_version,
            "vtklz4-%s" % self.short_version,
            "vtklzma-%s" % self.short_version,
            "vtkmetaio-%s" % self.short_version,
            "vtknetcdf-%s" % self.short_version,
            "vtkogg-%s" % self.short_version,
            "vtkpng-%s" % self.short_version,
            "vtkpugixml-%s" % self.short_version,
            "vtksqlite-%s" % self.short_version,
            "vtksys-%s" % self.short_version,
            "vtktheora-%s" % self.short_version,
            "vtktiff-%s" % self.short_version,
            "vtkverdict-%s" % self.short_version,
            "vtkzlib-%s" % self.short_version
        ]
        if self.options.qt:
            libs.append("vtkRenderingQt-%s" % self.short_version)
            libs.append("vtkViewsQt-%s" % self.short_version)
            libs.append("vtkGUISupportQt-%s" % self.short_version)
            libs.append("vtkGUISupportQtSQL-%s" % self.short_version)
        if self.options.mpi:
            libs.append("vtkParallelMPI-%s" % self.short_version)
        self.cpp_info.libs = libs
        self.cpp_info.includedirs = [
            "include/vtk-%s" % self.short_version,
            "include/vtk-%s/vtknetcdf/include" % self.short_version,
        ]
