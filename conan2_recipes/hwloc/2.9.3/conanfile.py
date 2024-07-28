from conan import ConanFile
from conan.tools.apple import is_apple_os, fix_apple_shared_install_name
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, get, rm, rmdir, unzip
from conan.tools.gnu import Autotools, AutotoolsToolchain, PkgConfigDeps
from conan.tools.layout import basic_layout
import os

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests the hwloc library (hardware elements locality info for HPC optimization).
# INFO: In order to prevent OneTBB missing package error, we build only shared library for hwloc.
#       In Windows, it uses CMake to configure and Autotools otherwise.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class HwlocConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "hwloc"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2.9.3"
    # A brief description of what the dependency does.
    description = "Portable Hardware Locality (hwloc)"
    # State some keywords describing what the dependency does.
    topics = ("hardware", "topology")
    # Declare the license of the dependency.
    license = "BSD-3-Clause"
    # Declare the home page of the dependency's project.
    homepage = "https://www.open-mpi.org/projects/hwloc/"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "shared-library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "with_libxml2": [True, False]
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "with_libxml2": False
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "hwloc-{}.tar.bz2".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1"#, "cmake_installer/3.29.3" --> see build_requirements() callback

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        self.settings.rm_safe("compiler.cppstd")
        self.settings.rm_safe("compiler.libcxx")

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        if self.settings.os == "Windows":
            cmake_layout(self, src_folder="src")
        else:
            basic_layout(self, src_folder="src")

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    def requirements(self):
        if self.options.with_libxml2:
            self.requires("libxml2/[>=2.12.5 <3]")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "hwloc-{}.tar.bz2".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "hwloc-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        if self.settings.os == "Windows":
            deps = CMakeDeps(self)
            deps.generate()
            tc = CMakeToolchain(self)
            tc.cache_variables["HWLOC_ENABLE_TESTING"] = 'OFF'
            tc.cache_variables["HWLOC_SKIP_LSTOPO"] = 'ON'
            tc.cache_variables["HWLOC_SKIP_TOOLS"] = 'ON'
            tc.cache_variables["HWLOC_SKIP_INCLUDES"] = 'OFF'
            tc.cache_variables["HWLOC_WITH_OPENCL"] = 'OFF'
            tc.cache_variables["HWLOC_WITH_CUDA"] = 'OFF'
            tc.cache_variables["HWLOC_BUILD_SHARED_LIBS"] = True
            tc.cache_variables["HWLOC_WITH_LIBXML2"] = self.options.with_libxml2
            tc.generate()
        else:
            deps = PkgConfigDeps(self)
            deps.generate()
            tc = AutotoolsToolchain(self)
            if not self.options.with_libxml2:
                tc.configure_args.extend(["--disable-libxml2"])
            tc.configure_args.extend(["--disable-io", "--disable-cairo"])
            tc.configure_args.extend(["--enable-shared", "--disable-static"])
            tc.generate()

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        if self.settings.os == "Windows":
            self.tool_requires("cmake_installer/[>=3.18 <4]")

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        if self.settings.os == "Windows":
            cmake = CMake(self)
            cmake.configure(build_script_folder=os.path.join("contrib", "windows-cmake"))
            cmake.build()
        else:
            autotools = Autotools(self)
            autotools.configure()
            autotools.make()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING", self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        if self.settings.os == "Windows":
            cmake = CMake(self)
            cmake.install()
            # remove PDB files
            rm(self, "*.pdb", os.path.join(self.package_folder, "bin"))
        else:
            autotools = Autotools(self)
            autotools.install()
            fix_apple_shared_install_name(self)
            # remove tools
            rmdir(self, os.path.join(self.package_folder, "bin"))

        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rm(self, "*.la", os.path.join(self.package_folder, "lib"))
        rmdir(self, os.path.join(self.package_folder, "share"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("pkg_config_name", "hwloc")
        self.cpp_info.libs = ["hwloc"]
        if is_apple_os(self):
            self.cpp_info.frameworks = ['IOKit', 'Foundation', 'CoreFoundation']
