from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout, CMakeToolchain
from conan.tools.files import apply_conandata_patches, export_conandata_patches, copy, get, rmdir, unzip
import os

required_conan_version = ">=1.52.0"


# This recipe installs the headers of Eigen (a header-only library that is not actually built).

# NOTE: GammaRay itself uses a different Eigen that is in its /thirdparty source subdirectory.  This is currently
#       a depedency to build ITK and possible other dependencies in the future (as of 2024-07-24).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class EigenConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "eigen"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "3.4.0"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "http://eigen.tuxfamily.org"
    # A brief description of what the dependency does.
    description = "Eigen is a C++ template library for linear algebra: matrices, vectors," \
                  " numerical solvers, and related algorithms."
    # State some keywords describing what the dependency does.
    topics = ("algebra", "linear-algebra", "matrix", "vector", "numerical", "header-only")
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "header-library"
    # Declare the license of the dependency.
    license = ("MPL-2.0", "LGPL-3.0-or-later")  # Taking into account the default value of MPL2_only option
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "MPL2_only": [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "MPL2_only": False,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "eigen-{}.tar.bz2".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    # Has the same effect as the export_sources attribute, but in method form. Hence, this callback is called when
    # exporting (copying) files from the source repository to the source directory in Conan's workplace.
    def export_sources(self):
        export_conandata_patches(self)

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        self.license = "MPL-2.0" if self.options.MPL2_only else ("MPL-2.0", "LGPL-3.0-or-later")

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        cmake_layout(self, src_folder="src")

    # This is used to define which settings and options parcticipate in the hash calculation
    # for the package in Conan's cache.
    # In this case, we only want package name and version take part in the package's hash calculation 
    # since Eigen is a header-only library.
    def package_id(self):
        self.info.clear()

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #get(self, **self.conan_data["sources"][self.version],
        #    destination=self.source_folder, strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "eigen-{}.tar.bz2".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "eigen-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["BUILD_TESTING"] = not self.conf.get("tools.build:skip_test", default=True, check_type=bool)
        tc.cache_variables["EIGEN_TEST_NOQT"] = True
        tc.generate()

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
        cmake = CMake(self)
        cmake.install()
        copy(self, "COPYING.*", self.source_folder, os.path.join(self.package_folder, "licenses"))
        rmdir(self, os.path.join(self.package_folder, "share"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "Eigen3")
        self.cpp_info.set_property("cmake_target_name", "Eigen3::Eigen")
        self.cpp_info.set_property("pkg_config_name", "eigen3")
        # TODO: back to global scope once cmake_find_package* generators removed
        self.cpp_info.components["eigen3"].bindirs = []
        self.cpp_info.components["eigen3"].libdirs = []
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.components["eigen3"].system_libs = ["m"]
        if self.options.MPL2_only:
            self.cpp_info.components["eigen3"].defines = ["EIGEN_MPL2_ONLY"]

        # TODO: to remove in conan v2 once cmake_find_package* & pkg_config generators removed
        self.cpp_info.names["cmake_find_package"] = "Eigen3"
        self.cpp_info.names["cmake_find_package_multi"] = "Eigen3"
        self.cpp_info.names["pkg_config"] = "eigen3"
        self.cpp_info.components["eigen3"].names["cmake_find_package"] = "Eigen"
        self.cpp_info.components["eigen3"].names["cmake_find_package_multi"] = "Eigen"
        self.cpp_info.components["eigen3"].set_property("cmake_target_name", "Eigen3::Eigen")
        self.cpp_info.components["eigen3"].includedirs = [os.path.join("include", "eigen3")]
