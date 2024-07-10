# -*- coding: utf-8 -*-
import os
from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import unzip, copy, rmdir

# This recipe doesn't actually build Straberry Pearl.  Instead, it only extracts and installs (packages) the Windows
# binaries and headers from the accompaining archive.  Straberry Pearl is not necessary in Linux.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class StrawberryperlConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "strawberryperl"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "5.30.0.1"
    # A brief description of what this recipe does.
    description = "Strawbery Perl for Windows. Useful as build_require"
    # Declare the license of the dependency.
    license = "GNU Public License or the Artistic License"
    # Declare the home page of the dependency's project.
    homepage = "http://strawberryperl.com"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-community/conan-strawberryperl"
    # Mention authorship.
    author = "Conan Community"
    # State some keywords describing what the dependency does.
    topics = ("conan", "installer", "perl", "windows")
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os"
    # Do not copy the source files to the build directory in order to build. This speeds up build time by 
    # avoiding unnecessary copies but the you HAVE to make sure no configure or build script/process change any 
    # of the source files (e.g. apply a patch).
    no_copy_source = True
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "strawberryperl-{}.tar.xz".format(version)

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        if self.settings.os != "Windows":
            raise ConanInvalidConfiguration("Only Windows supported for Strawberry Perl.")

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        self.output.info("Deflating archive with Strawberry Pearl binaries and headers...")
        unzip(self, filename=os.path.join(self.source_folder, "strawberryperl-{}.tar.xz".format(self.version)))

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, pattern="*", src=self.build_folder, dst=self.package_folder, keep_path=True)
        rmdir(self, os.path.join(self.package_folder, "c", "lib", "pkgconfig"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        perl_bin = os.path.join(self.package_folder, "perl", "bin")
        self.output.info('Inserting in front of PATH environment variable: %s' % perl_bin)
        self.buildenv_info.prepend_path("PATH", perl_bin)
        c_bin = os.path.join(self.package_folder, "c", "bin")
        self.output.info('Inserting in front of PATH environment variable: %s' % c_bin)
        self.buildenv_info.prepend_path("PATH", c_bin)
