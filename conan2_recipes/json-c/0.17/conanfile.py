from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import collect_libs, copy, get, rmdir, unzip
from conan.tools.scm import Version
import os

required_conan_version = ">=1.54.0"

# This recipe builds, installs and tests the json-C library (json parser for C/C++ applications).


# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class JSONCConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "json-c"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "0.17"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "library"
    # A brief description of what the dependency does.
    description = "JSON-C - A JSON implementation in C"
    # State some keywords describing what the dependency does.
    topics = ("json", "encoding", "decoding", "manipulation")
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://github.com/json-c/json-c"
    # A brief description of what the dependency does.
    license = "MIT"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "json-c-{}.tar.gz".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

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
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.settings.rm_safe("compiler.cppstd")
        self.settings.rm_safe("compiler.libcxx")

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        cmake_layout(self, src_folder="src")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "json-c-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "json-c-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        if Version(self.version) >= "0.15":
            tc.variables["BUILD_STATIC_LIBS"] = not self.options.shared
            tc.variables["DISABLE_STATIC_FPIC"] = not self.options.get_safe("fPIC", True)
        # To install relocatable shared libs on Macos
        tc.cache_variables["CMAKE_POLICY_DEFAULT_CMP0042"] = "NEW"
        tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "json-c")
        self.cpp_info.set_property("cmake_target_name", "json-c::json-c")
        self.cpp_info.set_property("pkg_config_name", "json-c")
        self.cpp_info.libs = collect_libs(self)
        self.cpp_info.includedirs = ["include", "include/json-c"]
