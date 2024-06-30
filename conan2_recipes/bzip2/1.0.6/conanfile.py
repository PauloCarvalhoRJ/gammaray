import os
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.files import unzip, replace_in_file, copy, rmdir
import shutil

# This recipe builds, installs (packages) and tests the bzip2 library.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class Bzip2Conan(ConanFile):
    name = "bzip2"
    version = "1.0.6"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "http://www.bzip.org"
    license = "bzip2-1.0.6"
    description = "bzip2 is a free and open-source file compression program that uses the Burrows Wheeler algorithm."
    topics = ("conan", "bzip2", "data-compressor", "file-compression")
    settings = "os", "compiler", "arch", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "build_executable": [True, False],
        "os_version": ["linux", "windows", "other"]
    }
    default_options = {"shared": False, "fPIC": True, "build_executable": True}
    no_copy_source = True
    source_url = "https://sourceware.org/pub/bzip2/bzip2-1.0.6.tar.gz"
    exports_sources = ["bzip2-1.0.6.tar.gz",
                       "CMakeLists.txt"]
    generators = "CMakeDeps"
    zip_file = ""
    build_requires = "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    # Returns the name of the bzip source sub-folder varying with the bzip2 version.
    @property
    def source_subfolder(self):
        return "bzip2-{}".format(self.version)

    # Returns the path to the tarball containing the source code of bzip2.
    @property
    def source_zip_filename(self):
        return "{}.tar.gz".format(self.source_subfolder)

    # Called by build(). Not Conan API.  Runs configuration with CMake.
    def _configure_cmake(self):
        # major = self.version.split(".")[0]
        cmake = CMake(self)
        # Setting CMake variables/definitions here no longer is possible in Conan 2 (see the generate() callback).
        #   see https://github.com/conan-io/conan/issues/16495
        # cmake.definitions["BZ2_VERSION_STRING"] = self.version
        # cmake.definitions["BZ2_VERSION_MAJOR"] = major
        # cmake.definitions["BZ2_BUILD_EXE"] = "ON" if self.options.build_executable else "OFF"
        # cmake.configure(source_folder=self.build_folder)
        cmake.configure(build_script_folder=self.build_folder)
        return cmake

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

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
        del self.settings.compiler.cppstd
        if self.settings.os == "Windows":
            self.options.os_version = "windows"
        elif self.settings.os == "Linux":
            del self.settings.compiler.libcxx
            self.options.os_version = "linux"
        else:
            del self.settings.compiler.libcxx
            self.options.os_version = "other"

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        major = self.version.split(".")[0]
        tc = CMakeToolchain(self)
        tc.variables["BZ2_VERSION_STRING"] = self.version
        tc.variables["BZ2_VERSION_MAJOR"] = major
        tc.variables["BZ2_BUILD_EXE"] = "ON" if self.options.build_executable else "OFF"
        tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        unzip(self, filename=os.path.join(self.source_folder, self.source_zip_filename), destination=self.build_folder, keep_permissions=True)
        replace_in_file(self, os.path.join(self.source_subfolder, "bzip2.c"), r"<sys\stat.h>", "<sys/stat.h>")
        shutil.copy(os.path.join(self.source_folder, "CMakeLists.txt"), os.path.join(self.build_folder, "CMakeLists.txt"))
        shutil.move(self.source_subfolder, "source_subfolder")
        cmake = self._configure_cmake()
        cmake.build()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, pattern="LICENSE", dst="licenses", src=self.source_subfolder)
        cmake = self._configure_cmake()
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `build_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.name = "BZip2"
        self.cpp_info.libs = ['bz2']
