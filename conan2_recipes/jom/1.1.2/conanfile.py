#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conan import ConanFile
from conan.tools.files import unzip, copy
import os

# This recipe doesn't actually build JOM.  Instead, it only extracts and installs (packages) the Windows
# executable from the accompaining archive.  JOM is not necessary in Linux.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class JomInstallerConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "jom_installer"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.1.2"
    # A brief description of what this recipe does.
    description = "jom is a clone of nmake to support the execution of multiple independent commands in parallel"
    # Declare where the content was obtained from.
    url = "https://github.com/bincrafters/conan-jom_installer"
    # Declare the home page of the dependency's project.
    homepage = "http://wiki.qt.io/Jom"
    # Mention authorship.
    author = "Bincrafters <bincrafters@gmail.com>"
    # Declare the license of the dependency.
    license = "GPL-3.0"
    # A list of files that should be exported (copied) alongside the recipe (conanfile.py) so the recipe can work.
    # For example, other Python scripts that the recipe may depend on.
    exports = ["LICENSE.md"]
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os"
    # Do not copy the source files to the build directory in order to build. This speeds up build time by 
    # avoiding unnecessary copies but the you HAVE to make sure no configure or build script/process change any 
    # of the source files (e.g. apply a patch).
    no_copy_source = True
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "jom-{}.tar.xz".format(version)
    # Declare where the archive with the dependency's source code was downloaded from.
    source_url = "http://download.qt.io/official_releases/jom/jom_{}.zip".format(version.replace('.', '_'))

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    # Returns the name of the JOM source sub-folder varying with the JOM version.
    @property
    def source_subfolder(self):
        return "jom-{}".format(self.version)

    # Returns the path to the tarball containing the source code of eigen3.
    @property
    def source_zip_filename(self):
        return "{}.tar.xz".format(self.source_subfolder)

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        if self.settings.os != "Windows":
            raise ConanException(f"JOM is not supposed to be required in non-Windows OSes.  Check the recipes of packages that depend on JOM in Windows (e.g. Qt).")

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        unzip(self, filename=os.path.join(self.source_folder, self.source_zip_filename))

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, pattern="*.exe", dst=os.path.join(self.package_folder, "bin"), src=os.path.join(self.build_folder, self.source_subfolder))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.buildenv_info.prepend_path("PATH", os.path.join(self.package_folder, "bin"))
