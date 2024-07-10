import os
from conan import ConanFile
from conan.tools.files import unzip, copy

# This recipe doesn't actually build nasm.  Instead, it only extracts and installs (packages) the Windows
# binaries and headers from the accompaining archive.  nasm is not necessary in Linux.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class NasmConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "nasm"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2.13.01"
    # Declare the license of the dependency.
    license = "BSD-2-Clause"
    # Declare where the content was obtained from.
    url = "https://github.com/lasote/conan-nasm-installer"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os"
    # A brief description of what this recipe does.
    description="Nasm for windows. Useful as a tool_require."
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "nasm-{}.tar.xz".format(version)
    # Do not copy the source files to the build directory in order to build. This speeds up build time by 
    # avoiding unnecessary copies but the you HAVE to make sure no configure or build script/process change any 
    # of the source files (e.g. apply a patch).
    no_copy_source = True

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def nasm_folder_name(self):
        return "nasm-%s" % self.version

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        if self.settings.os != "Windows":
            raise Exception("Only windows supported for nasm.")

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        unzip(self, filename=os.path.join(self.source_folder, "{}.tar.xz".format(self.nasm_folder_name)))

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        #self.copy("*", dst="", keep_path=True)
        #self.copy("license*", dst="", src=self.nasm_folder_name, keep_path=False, ignore_case=True)
        copy(self, pattern="*", src=self.build_folder, dst=self.package_folder, keep_path=True)
        copy(self, pattern="license*", dst=self.package_folder, src=self.nasm_folder_name, keep_path=False, ignore_case=True)

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.output.info("Using %s version " % self.nasm_folder_name)
        self.buildenv_info.prepend_path("PATH", os.path.join(self.package_folder, self.nasm_folder_name))
