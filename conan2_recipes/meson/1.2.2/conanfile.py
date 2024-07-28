import os
import textwrap

from conan import ConanFile, conan_version
from conan.tools.files import copy, get, rmdir, save, unzip
from conan.tools.layout import basic_layout
from conan.tools.scm import Version

required_conan_version = ">=1.52.0"

# This recipe installs and tests the Meson utils (a build system written in Python).
# NOTE: this recipe does not actually build Meson as it is a Python application.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class MesonConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "meson"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.2.2"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "application"
    # A brief description of what the dependency does.
    description = "Meson is a project to create the best possible next-generation build system"
    # State some keywords describing what the dependency does.
    topics = ("meson", "mesonbuild", "build-system")
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://github.com/mesonbuild/meson"
    # Declare the license of the dependency.
    license = "Apache-2.0"
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "meson-{}.tar.gz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @staticmethod
    def _chmod_plus_x(filename):
        if os.name == "posix":
            os.chmod(filename, os.stat(filename).st_mode | 0o111)

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self, src_folder="src")

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    # Python should be a requirement, but since Conan is already in Python, there is no need for it.
    def requirements(self):
        if self.conf.get("tools.meson.mesontoolchain:backend", default="ninja", check_type=str) == "ninja":
            # self.requires("ninja/1.11.1") --> from the original recipe
            self.requires("ninja/1.12.1")

    # This is used to define which settings and options parcticipate in the hash calculation
    # for the package in Conan's cache.
    # In this case, we only want package name and version take part in the package's hash calculation.
    def package_id(self):
        self.info.clear()

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # in the original recipe, the following was used to download the file with the URL in conandata.yml.
        #get(self, **self.conan_data["sources"][self.version], destination=self.source_folder, strip_root=True) 
        unzip(self, filename=os.path.join(self.source_folder, "..", "meson-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "meson-{}".format(self.version)), dst=".")

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    # In this case, as a Python application, nothing is actually build.
    def build(self):
        pass

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        copy(self, "*", src=self.source_folder, dst=os.path.join(self.package_folder, "bin"))
        rmdir(self, os.path.join(self.package_folder, "bin", "test cases"))

        # create wrapper scripts
        save(self, os.path.join(self.package_folder, "bin", "meson.cmd"), textwrap.dedent("""\
            @echo off
            set PYTHONDONTWRITEBYTECODE=1
            CALL python %~dp0/meson.py %*
        """))
        save(self, os.path.join(self.package_folder, "bin", "meson"), textwrap.dedent("""\
            #!/usr/bin/env bash
            meson_dir=$(dirname "$0")
            export PYTHONDONTWRITEBYTECODE=1
            exec "$meson_dir/meson.py" "$@"
        """))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        meson_root = os.path.join(self.package_folder, "bin")
        self._chmod_plus_x(os.path.join(meson_root, "meson"))
        self._chmod_plus_x(os.path.join(meson_root, "meson.py"))

        self.cpp_info.builddirs = [os.path.join("bin", "mesonbuild", "cmake", "data")]

        self.cpp_info.includedirs = []
        self.cpp_info.libdirs = []

        if Version(conan_version).major < 2:
            self.env_info.PATH.append(meson_root)
