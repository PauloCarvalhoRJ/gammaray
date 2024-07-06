from conan import ConanFile
from conan.tools.files import unzip, copy
import subprocess
import os

# This recipe doesn't actually build Ninja.  Instead, it only extracts either the Linux or Windows
# executable from the accompaining ninja.zip file according to the operating system.
# Pre-built Ninja executables for various OSes can be obtained from https://github.com/ninja-build/ninja/releases

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class NinjaConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "ninja"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.12.1"
    # Declare the license of the dependency.
    license = "Apache License 2.0"
    # A brief description of what this recipe does.
    description = "This recipe extracts the correct ninja executable for the OS."
    # Declare what information are needed to build this package.
    # In this case: only operating system.
    #settings = "os", "compiler", "build_type", "arch"
    settings = "os"
    # Do not copy the source files to the build directory in order to build. This speeds up build time by 
    # avoiding unnecessary copies but the you HAVE to make sure no configure or build script/process change any 
    # of the source files (e.g. apply a patch).
    no_copy_source = True
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "ninja*"

    # Returns the correct file name for the Ninja executable according to the operating system.
    # NOTE: This method is not part of the ConanFile interface.
    @property
    def ninja_executable(self):
        if self.settings.os == "Linux":
            return "ninja"
        elif self.settings.os == "Windows":
            return "ninja.exe"
        else:
            raise NameError("Unsupported operating system: " + self.settings.os + ".  Check the Conan profile file you are using." )

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        # Extracts all the Ninja executables of ninja.zip, which is in the source directory, into the build directory.
        unzip(self, filename=os.path.join(self.source_folder, "ninja.zip"), destination=self.build_folder, keep_permissions=True)

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    def package(self):
        # Copy the adequate Ninja executable from the build directory into the 
        # package directory, that is, will be made available for use.
        copy(self, pattern=self.ninja_executable, src=self.build_folder, dst=os.path.join(self.package_folder, "bin"))
        # Sets the adequate permissions for executables in Linux
        if self.settings.os == "Linux": 
            subprocess.call(['chmod', 'a+rwx', os.path.join(self.package_folder, "bin", "ninja")])

    # This is called when running recipes for packages which are dependant of this one.
    def package_info(self):
        # Dependant packages of this package will have the ninja executable in the PATH environment
        # variable during their build time so their own build process can find and use Ninja.  
        # No need to clutter the env nor have difficult-to-manage/maintain bash/batch scripts.
        self.buildenv_info.prepend_path("PATH", os.path.join(self.package_folder, "bin"))
