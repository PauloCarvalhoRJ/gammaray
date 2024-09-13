from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, get, rmdir, unzip
from conan.tools.scm import Version
import os

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests Brotli 1.1.0 (a compression format).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class BrotliConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "brotli"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.1.0"
    # A brief description of what this dependency does.
    description = "Brotli compression format"
    # Declare the license of the dependency.
    license = "MIT",
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://github.com/google/brotli"
    # State some keywords describing what the dependency does.
    topics = ("brotli", "compression")
    # Hints Conan of what information should be propagated to consumer packages (e.g. include and library directories).
    package_type = "library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # Declare additional configurations needed to define a build. 
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "target_bits": [64, 32, None],
        "endianness": ["big", "little", "neutral", None],
        "enable_portable": [True, False],
        "enable_rbit": [True, False],
        "enable_debug": [True, False],
        "enable_log": [True, False],
    }
    # Sets the default values to the additional configurations declared in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "target_bits": None,
        "endianness": None,
        "enable_portable": False,
        "enable_rbit": True,
        "enable_debug": False,
        "enable_log": False,
    }
    # Declares this dependency's dependencies needed for building.  This
    # allows not polluting the system's PATH environment variable with tools only needed
    # to build software.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "brotli-{}.tar.gz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _get_decorated_lib(self, name):
        libname = name
        if Version(self.version) < "1.1.0" and not self.options.shared:
            libname += "-static"
        return libname

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # This is called when files need to be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.  This is the method form of the export_sources attribute.
    def export_sources(self):
        export_conandata_patches(self)

    # Does the same as the options attribute, but in method form.
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
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> archive is locally present, no need to download it
        unzip(self, filename=os.path.join(self.source_folder, "..", "brotli-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "brotli-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BROTLI_BUNDLED_MODE"] = False
        tc.variables["BROTLI_DISABLE_TESTS"] = True
        if self.options.get_safe("target_bits") == 32:
            tc.preprocessor_definitions["BROTLI_BUILD_32_BIT"] = 1
        elif self.options.get_safe("target_bits") == 64:
            tc.preprocessor_definitions["BROTLI_BUILD_64_BIT"] = 1
        if self.options.get_safe("endianness") == "big":
            tc.preprocessor_definitions["BROTLI_BUILD_BIG_ENDIAN"] = 1
        elif self.options.get_safe("endianness") == "neutral":
            tc.preprocessor_definitions["BROTLI_BUILD_ENDIAN_NEUTRAL"] = 1
        elif self.options.get_safe("endianness") == "little":
            tc.preprocessor_definitions["BROTLI_BUILD_LITTLE_ENDIAN"] = 1
        if self.options.enable_portable:
            tc.preprocessor_definitions["BROTLI_BUILD_PORTABLE"] = 1
        if not self.options.enable_rbit:
            tc.preprocessor_definitions["BROTLI_BUILD_NO_RBIT"] = 1
        if self.options.enable_debug:
            tc.preprocessor_definitions["BROTLI_DEBUG"] = 1
        if self.options.enable_log:
            tc.preprocessor_definitions["BROTLI_ENABLE_LOG"] = 1
        if Version(self.version) < "1.1.0":
            # To install relocatable shared libs on Macos
            tc.cache_variables["CMAKE_POLICY_DEFAULT_CMP0042"] = "NEW"
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
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        includedir = os.path.join("include", "brotli")
        # brotlicommon
        self.cpp_info.components["brotlicommon"].set_property("pkg_config_name", "libbrotlicommon")
        self.cpp_info.components["brotlicommon"].includedirs.append(includedir)
        self.cpp_info.components["brotlicommon"].libs = [self._get_decorated_lib("brotlicommon")]
        if self.settings.os == "Windows" and self.options.shared:
            self.cpp_info.components["brotlicommon"].defines.append("BROTLI_SHARED_COMPILATION")
        # brotlidec
        self.cpp_info.components["brotlidec"].set_property("pkg_config_name", "libbrotlidec")
        self.cpp_info.components["brotlidec"].includedirs.append(includedir)
        self.cpp_info.components["brotlidec"].libs = [self._get_decorated_lib("brotlidec")]
        self.cpp_info.components["brotlidec"].requires = ["brotlicommon"]
        # brotlienc
        self.cpp_info.components["brotlienc"].set_property("pkg_config_name", "libbrotlienc")
        self.cpp_info.components["brotlienc"].includedirs.append(includedir)
        self.cpp_info.components["brotlienc"].libs = [self._get_decorated_lib("brotlienc")]
        self.cpp_info.components["brotlienc"].requires = ["brotlicommon"]
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.components["brotlienc"].system_libs = ["m"]

        # TODO: to remove in conan v2 once cmake_find_package* & pkg_config generators removed.
        #       do not set this target in CMakeDeps, it was a mistake, there is no official brotil config file, nor Find module file
        self.cpp_info.names["cmake_find_package"] = "Brotli"
        self.cpp_info.names["cmake_find_package_multi"] = "Brotli"
        self.cpp_info.components["brotlicommon"].names["pkg_config"] = "libbrotlicommon"
        self.cpp_info.components["brotlidec"].names["pkg_config"] = "libbrotlidec"
        self.cpp_info.components["brotlienc"].names["pkg_config"] = "libbrotlienc"
