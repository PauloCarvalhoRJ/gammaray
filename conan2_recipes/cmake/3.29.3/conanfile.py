# -*- coding: utf-8 -*-
import os
from conan import ConanFile
from conan.tools.files import unzip, copy
from conan.errors import ConanException

# This recipe doesn't actually build CMake.  Instead, it only extracts either the Linux or Windows
# executables from the accompaining archives (one for Windows and the other for Linux) file 
# according to the operating system.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class CMakeInstallerConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "cmake_installer"
    # A brief description of what this recipe does.
    description = "This recipe extracts the CMake binaries appropriate to the current OS."
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "3.29.3"
    # Declare the license of the dependency.
    license = "BSD-3-clause"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-community/conan-cmake-installer"
    # Mention authorship.
    author = "Conan Community"
    # Declare the home page of the dependency's project.
    homepage = "https://github.com/Kitware/CMake"
    # State some keywords describing what the dependency does.
    topics = ("conan", "cmake", "build", "installer", "configure")
    # Declare what information are needed to build this package.
    # In this case: only operating system.
    #settings = "os", "compiler", "build_type", "arch"
    settings = "os"
    # Copy/distribute the LICENSE file.
    exports = "LICENSE"
    # Do not copy the source files to the build directory in order to build. This speeds up build time by 
    # avoiding unnecessary copies but the you HAVE to make sure no configure or build script/process change any 
    # of the source files (e.g. apply a patch).
    no_copy_source = True
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = ["cmake-3.29.3-Linux-x86_64.tar.gz",
                       "cmake-3.29.3-win64-x64.zip"]
    # Declares this dependeny's dependencies.
    # This does not actually require Ninja, but this serves as a reminder that building other packages will be done
    # with Ninja. Hence, CMake can generate Ninja makefiles.
    requires = "ninja/1.12.1"

    # Returns the correct subfolder name found in the CMake archive according to the operating system.
    # NOTE: This method is not part of the ConanFile interface.
    @property
    def source_archive_subfolder(self):
        if self.settings.os == "Linux":
            return "cmake-3.29.3-Linux-x86_64"
        elif self.settings.os == "Windows":
            return "cmake-3.29.3-win64-x64"
        else:
            raise NameError("Unsupported operating system: " + self.settings.os + ".  Check the Conan profile file you are using." )

    # Returns the correct CMake archive (.zip for Windows and .tar.gz for Linux) according to the operating system.
    # NOTE: This method is not part of the ConanFile interface.
    @property
    def source_archive_filename(self):
        if self.settings.os == "Linux":
            return "cmake-3.29.3-Linux-x86_64.tar.gz"
        elif self.settings.os == "Windows":
            return "cmake-3.29.3-win64-x64.zip"
        else:
            raise NameError("Unsupported operating system: " + self.settings.os + ".  Check the Conan profile file you are using." )

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        # Extracts everything from the CMake archive, which is in the source directory, into the build directory.
        unzip(self, filename=os.path.join(self.source_folder, self.source_archive_filename), destination=self.build_folder, keep_permissions=True)

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    def package(self):
        # Copy everything from the build directory into the 
        # package directory, that is, will be made available for use.
        copy(self, pattern="*", src=self.source_archive_subfolder, dst=self.package_folder)
        # Sets the adequate permissions for executables in Linux
        if self.settings.os == "Linux": 
            subprocess.call(['chmod', 'a+rwx', os.path.join(self.package_folder, "bin", "*")])

    # This is called when running recipes for packages which are dependant of this one.
    def package_info(self):
        if self.package_folder is not None:
            # Add all CMake binaries to the PATH environmnet variable in the first position.
            self.buildenv_info.prepend_path("PATH", os.path.join(self.package_folder, "bin"))
            # Sets the CMAKE_ROOT environment variable.
            self.buildenv_info.define("CMAKE_ROOT", self.package_folder)
            # Sets the CMAKE_MODULE_PATH environment variable.
            mod_path = os.path.join(self.package_folder, "share", "cmake-{}".format(self.version[0:4]), "Modules")
            self.buildenv_info.define("CMAKE_MODULE_PATH", mod_path)
            # Sanity check.
            if not os.path.exists(mod_path):
                raise ConanException("CMake module path not found: %s" % mod_path)
        else:
            self.output.warn("No package folder have been created.")
