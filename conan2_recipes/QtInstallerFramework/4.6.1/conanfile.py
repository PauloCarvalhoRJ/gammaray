from conan import ConanFile
#from conan.tools.cmake import CMake
from conan.tools.files import unzip, copy
import os
from conan.tools.layout import basic_layout

# This recipe doesn't actually build QtIFW.  Instead, it only extracts either the Linux or Windows
# executables from one of the accompaining tar.xz archives files according to the operating system.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class QtIFWConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "qtifw"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "4.6.1"
    # A brief description of what this recipe does.
    description = "Provides a set of tools and utilities to create installers"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    # In this case: only operating system.
    # settings = "os", "compiler", "build_type", "arch"
    settings = "os"
    # Do not copy the source files to the build directory in order to build. This speeds up build time by 
    # avoiding unnecessary copies but the you HAVE to make sure no configure or build script/process change any 
    # of the source files (e.g. apply a patch).
    no_copy_source = True
    # A set of additional custom options that are necessary to build the dependency.
    options = {"os_version": ["linux", "windows", "other"]}
    #generators = "CMakeDeps"
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    # exports_sources = "QtIFW-{}-{}.tar.xz".format(version, "linux" if (settings.os == "Linux") else "windows") --> doesn't work in Conan 2
    exports_sources = "QtIFW*.tar.xz" # so we just import both archives in Conan 2... :(

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    # Returns the name of the QtIFW source sub-folder varying with its version.
    @property
    def source_subfolder(self):
        return "QtIFW-{}".format(self.version)

    # Returns the path to the correct QtIFW tarball depending on the operating system.
    @property
    def source_zip_filename(self):
        return "QtIFW-{}-{}.tar.xz".format(self.version, "linux" if (self.settings.os == "Linux") else "windows")

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        if self.settings.os == "Windows":
            self.options.os_version = "windows"
        elif self.settings.os == "Linux":
            self.options.os_version = "linux"
        else:
            self.options.os_version = "other"

    # Customize the default Conan workspace before building (e.g. define the name for the source directory).
    def layout(self):
        basic_layout(self)

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        unzip(self, filename=os.path.join(self.source_folder, self.source_zip_filename))

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, pattern="*", src=self.source_subfolder, dst=self.package_folder)

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `build_requires = ...` attributes.
    def package_info(self):
        # Dependant packages of this package will have the QtIFW binaries in the PATH environment
        # variable during their build time so their own build process can find and use QtIFW.  
        # No need to clutter the env nor have difficult-to-manage/maintain bash/batch scripts.
        self.buildenv_info.append("PATH", os.path.join(self.package_folder, "bin"))
