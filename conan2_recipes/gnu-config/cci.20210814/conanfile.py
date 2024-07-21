from conan import ConanFile
from conan.errors import ConanException
from conan.tools.files import copy, get, load, save, apply_conandata_patches, export_conandata_patches, unzip
from conan.tools.layout import basic_layout
import os

required_conan_version = ">=1.52.0"

# This installs in Conan's cache the GNU's config.gess and config.sub scripts that are needed by some builds with Autotools.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class GnuConfigConan(ConanFile):
    name = "gnu-config"
    # This is the name that will be used to locate the package/dependency in Conan repository.
    version = "cci.20210814"
    # A brief description of what the dependency does.
    description = "The GNU config.guess and config.sub scripts"
    # Declare the home page of the dependency's project.
    homepage = "https://savannah.gnu.org/projects/config/"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # State some keywords describing what the dependency does.
    topics = ("gnu", "config", "autotools", "canonical", "host", "build", "target", "triplet")
    # Declare the license of the dependency.
    license = "GPL-3.0-or-later", "autoconf-special-exception"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. bin dir).
    package_type = "build-scripts"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    os = "arch", "compiler", "build_type", "arch"
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "gnu-config-{}.tar.gz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _extract_license(self):
        txt_lines = load(self, os.path.join(self.source_folder, "config.guess")).splitlines()
        start_index = None
        end_index = None
        for line_i, line in enumerate(txt_lines):
            if start_index is None:
                if "This file is free" in line:
                    start_index = line_i
            if end_index is None:
                if "Please send patches" in line:
                    end_index = line_i
        if not all((start_index, end_index)):
            raise ConanException("Failed to extract the license")
        return "\n".join(txt_lines[start_index:end_index])

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Has the same effect as the export_sources attribute, but in method form. Hence, this callback is called when
    # exporting (copying) files from the source repository to the source directory in Conan's workplace.
    def export_sources(self):
        export_conandata_patches(self)

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self, src_folder="src")

    # This is used to define which settings and options parcticipate in the hash calculation
    # for the package in Conan's cache.
    # In this case, we only want package name and version take part in the package's hash calculation.
    def package_id(self):
        self.info.clear()

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "gnu-config-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "gnu-config-{}".format(self.version)), dst=".")

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        apply_conandata_patches(self)

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        save(self, os.path.join(self.package_folder, "licenses", "COPYING"), self._extract_license())
        copy(self, "config.guess", src=self.source_folder, dst=os.path.join(self.package_folder, "bin"))
        copy(self, "config.sub", src=self.source_folder, dst=os.path.join(self.package_folder, "bin"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.includedirs = []
        self.cpp_info.libdirs = []

        bin_path = os.path.join(self.package_folder, "bin")
        self.conf_info.define("user.gnu-config:config_guess", os.path.join(bin_path, "config.guess"))
        self.conf_info.define("user.gnu-config:config_sub", os.path.join(bin_path, "config.sub"))

        # TODO: to remove in conan v2
        self.user_info.CONFIG_GUESS = os.path.join(bin_path, "config.guess")
        self.user_info.CONFIG_SUB = os.path.join(bin_path, "config.sub")
        self.env_info.PATH.append(bin_path)
