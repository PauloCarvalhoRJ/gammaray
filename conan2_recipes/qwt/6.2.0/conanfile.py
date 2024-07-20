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

# This recipe builds, installs and tests the qwt library (Qt for technical applications).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class QwtConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "qwt"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "6.2.0"
    # Declare the license of the dependency.
    license = "LGPL-2.1-or-later"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://qwt.sourceforge.io/"
    # State some keywords describing what the dependency does.
    topics = ("chart", "data-visualization", "graph", "plot", "qt")
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "library"
    # A brief description of what the dependency does.
    description = (
        "The Qwt library contains GUI Components and utility classes which are primarily useful for programs "
        "with a technical background. Beside a framework for 2D plots it provides scales, sliders, dials, compasses, "
        "thermometers, wheels and knobs to control or display values, arrays, or ranges of type double."
    )
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "plot": [True, False],
        "widgets": [True, False],
        "svg": [True, False],
        "opengl": [True, False],
        "designer": [True, False],
        "polar": [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "plot": True,
        "widgets": True,
        "svg": False,
        "opengl": True,
        "designer": False,
        "polar": True,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "qwt-{}.zip".format(version)
    # Declares this dependency's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _is_legacy_one_profile(self):
        return not hasattr(self, "settings_build")

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Has the same effect as the export_sources attribute, but in method form. Hence, this callback is called when
    # exporting (copying) files from the source repository to the source directory in Conan's workplace.
    def export_sources(self):
        export_conandata_patches(self)

    # Configure or constrain the available options in a package before assigning them a value. A typical use case is
    # to remove an option in a given platform. For example, the SSE2 flag doesn’t exist in architectures different 
    # than 32 bits, so it should be removed in this method.
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

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
        cmake_layout(self, src_folder="src")

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    def requirements(self):
        # The transitive_* flags mean that the Qt's headers and librarires must also be dependencies of
        # whatever depends on qwt.
        self.requires("qt/[>5.15]", transitive_headers=True, transitive_libs=True)

    # This callback is evoked to perform some validations (e.g. unsupported operating system) before building.
    # The detection of an invalid configuration must be signaled by raising an exception.
    def validate(self):
        if hasattr(self, "settings_build") and cross_building(self):
            raise ConanInvalidConfiguration("Qwt recipe does not support cross-compilation yet")
        qt_options = self.dependencies["qt"].options
        if self.options.widgets and not qt_options.widgets:
            raise ConanInvalidConfiguration("qwt:widgets=True requires qt:widgets=True")
        if self.options.svg and not qt_options.qtsvg:
            raise ConanInvalidConfiguration("qwt:svg=True requires qt:qtsvg=True")
        if self.options.opengl and qt_options.opengl == "no":
            raise ConanInvalidConfiguration("qwt:opengl=True is not compatible with qt:opengl=no")
        if self.options.designer and not (qt_options.qttools and qt_options.gui and qt_options.widgets):
            raise ConanInvalidConfiguration("qwt:designer=True requires qt:qttools=True, qt::gui=True and qt::widgets=True")

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        if not self._is_legacy_one_profile:
            self.tool_requires("qt/<host_version>")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "qwt-{}.zip".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "qwt-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        if self._is_legacy_one_profile:
            env = VirtualRunEnv(self)
            env.generate(scope="build")
        else:
            env = VirtualBuildEnv(self)
            env.generate()

        tc = CMakeToolchain(self)
        tc.variables["QWT_DLL"] = self.options.shared
        tc.variables["QWT_STATIC"] = not self.options.shared
        tc.variables["QWT_PLOT"] = self.options.plot
        tc.variables["QWT_WIDGETS"] = self.options.widgets
        tc.variables["QWT_SVG"] = self.options.svg
        tc.variables["QWT_OPENGL"] =self.options.opengl
        tc.variables["QWT_DESIGNER"] = self.options.designer
        tc.variables["QWT_POLAR"] = self.options.polar
        tc.variables["QWT_BUILD_PLAYGROUND"] = False
        tc.variables["QWT_BUILD_EXAMPLES"] = False
        tc.variables["QWT_BUILD_TESTS"] = False
        tc.variables["QWT_FRAMEWORK"] = False
        tc.variables["CMAKE_INSTALL_DATADIR"] = "res"
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.libs = ["qwt"]
        self.cpp_info.requires = ["qt::qtCore", "qt::qtConcurrent", "qt::qtPrintSupport"]
        if self.settings.os == "Windows" and self.options.shared:
            self.cpp_info.defines.append("QWT_DLL")
        if not self.options.plot:
            self.cpp_info.defines.append("NO_QWT_PLOT")
        if not self.options.polar:
            self.cpp_info.defines.append("NO_QWT_POLAR")
        if self.options.widgets:
            self.cpp_info.requires.append("qt::qtWidgets")
        else:
            self.cpp_info.defines.append("NO_QWT_WIDGETS")
        if self.options.opengl:
            self.cpp_info.requires.append("qt::qtOpenGL")
            if Version(self.dependencies["qt"].ref.version).major >= "6":
                self.cpp_info.requires.append("qt::qtOpenGLWidgets")
        else:
            self.cpp_info.defines.append("QWT_NO_OPENGL")
        if self.options.svg:
            self.cpp_info.requires.append("qt::qtSvg")
        else:
            self.cpp_info.defines.append("QWT_NO_SVG")

        if self.options.designer:
            qt_plugin_path = os.path.join(
                self.package_folder, "res" if self.settings.os == "Windows" else "lib",
                f"qt{Version(self.dependencies['qt'].ref.version).major}", "plugins",
            )
            self.runenv_info.prepend_path("QT_PLUGIN_PATH", qt_plugin_path)

            # TODO: to remove in conan v2
            self.env_info.QT_PLUGIN_PATH.append(qt_plugin_path)
