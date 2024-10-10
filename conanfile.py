from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import cross_building
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.files import (
                    apply_conandata_patches, copy, unzip,
                    export_conandata_patches, get, rmdir
               )
from conan.tools.scm import Version
import os

required_conan_version = ">=1.60.0 <2.0 || >=2.0.5"

# This recipe builds, installs and tests the GammaRay application.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class QwtConan(ConanFile):
    # This is the name that will be used to locate the program's binaries in Conan repository.
    name = "gammaray"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "6.22"
    # Declare the license of the dependency.
    license = "CC-BY-SA 3.0"
    # Declare where the content was obtained from.
    url = "https://github.com/PauloCarvalhoRJ/gammaray"
    # Declare the home page of the project.
    homepage = "https://github.com/PauloCarvalhoRJ/gammaray"
    # State some keywords describing what the dependency does.
    topics = ("geostatistics", "kriging", "simulation", "geomodeling", "estimation")
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "application"
    # A brief description of what the dependency does.
    description = (
        "GammaRay is a graphical user interface (GUI) that automates geostatistical "
        "workflows by driving and coordinating the several modules of the renowned "
        "Geostatistical Software Library (GSLib). The main purpose of GammaRay is to "
        "add a user-friendly interface layer on top of the scientifically and numerically "
        "robust GSLib, greatly automating parameter file editing and module chaining so the "
        "practitioner can focus on geostatistics."
    )
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # Declares this dependency's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Exports (copies) the GammaRay files (e.g. *.cpp, *.h, *.qrc, etc.) from here (the Git repository) to 
    # the source directory in Conan's workspace. It has the same effect as the export_sources attribute, but
    # in method form. 
    def export_sources(self):
        # C++ header files.
        copy(self, 
             pattern="*.h", 
             src=self.recipe_folder, 
             dst=self.export_sources_folder)
        copy(self, 
             pattern="*.hpp", 
             src=self.recipe_folder, 
             dst=self.export_sources_folder)
        copy(self, 
             pattern="*thirdparty/Eigen/*", #some Eigen headers don't have file name extension
             src=self.recipe_folder, 
             dst=self.export_sources_folder)
        # C++ source files.
        copy(self, 
             pattern="*.cpp", 
             src=self.recipe_folder, 
             dst=self.export_sources_folder)
        # Qt .ui (user interface definition XML) files.
        copy(self, 
             pattern="*.ui", 
             src=self.recipe_folder, 
             dst=self.export_sources_folder)
        # Qt resource files.
        copy(self, 
             pattern="*.qrc", 
             src=self.recipe_folder, 
             dst=self.export_sources_folder)
        # Button and menu icons.
        copy(self, 
             pattern="*.png", 
             src=self.recipe_folder, 
             dst=self.export_sources_folder)
        # The CMake configuration script.
        copy(self, 
             pattern="CMakeLists.txt", 
             src=self.recipe_folder, 
             dst=self.export_sources_folder)

    # Configure or constrain the available options in a package before assigning them a value. A typical use case is
    # to remove an option in a given platform. For example, the SSE2 flag doesn’t exist in architectures different 
    # than 32 bits, so it should be removed in this method.
    #def config_options(self):
    #    pass

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    #def configure(self):
    #    pass

    # Sets the build and runtime dependencies of GammaRay.
    # This is the same as the requires attribute, but in method form.
    def requirements(self):
        # See target names in the respective find_package()'s and target_link_libraries()'s
        # in the program's CMakeLists.txt.  Some of them differ from Conan package names
        # and some of the larger ones have component targets (e.g. Qt and Boost).
        self.requires("qt/[>5.15]")
        self.requires("qwt/[>=6.2.0]")
        self.requires("vtk/[>=9.1]")
        # self.requires("eigen/[>=3]") //Eigen is currently directly in /thirdpary source subdirectory
        self.requires("fftw/[>=3]") 
        self.requires("gsl/[>=2.7]")
        self.requires("boost/[>=1.78]")
        self.requires("itk/[>=5.3]")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        tc = CMakeToolchain(self)

        # This info is needed by ITK's UseITK.cmake when dependant packages like GammaRay use
        # ITK package during configuration with CMake.
        itk_cmake_dir = self.dependencies['itk'].cpp_info.get_property("itk_cmake_dir")
        tc.cache_variables["ITK_CMAKE_DIR"] = itk_cmake_dir.replace("\\", "/") # backslashes in Windows paths result in syntax errors in CMake

        #tc.variables["QWT_DLL"] = self.options.shared
        #tc.variables["QWT_STATIC"] = not self.options.shared
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    # Allows to customize some standard attributes (e.g. source folder is called "src").
    def layout(self):
        cmake_layout(self)

    # This callback is evoked to perform some validations (e.g. unsupported operating system) before building.
    # The detection of an invalid configuration must be signaled by raising an exception.
    def validate(self):
        qt_options = self.dependencies["qt"].options
        if not qt_options.widgets:
            raise ConanInvalidConfiguration("GammaRay requires qt:widgets=True")
        if not (qt_options.qttools and qt_options.gui):
            raise ConanInvalidConfiguration("GammaRay requires qt:qttools=True and qt::gui=True")

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        self.tool_requires("qt/<host_version>")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the source files are already in the source repository, this only
    # copies them into the build directory in Conan's workspace.
    def source(self):
        copy(self, pattern="*", src=self.source_folder, dst=".")

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        cmake = CMake(self)
        cmake.install()

    # This is called when running recipes for packages which are dependant of this one.
    # Since GammaRay is supposed to be an end-user application, nothing is currently being
    # propagated to possible consumer packages (this may change in the future).
    def package_info(self):
        pass
