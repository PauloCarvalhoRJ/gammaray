from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, get, rmdir, unzip
import os

required_conan_version = ">=1.54.0"

# This recipe builds, installs and tests the qwt library (Qt for technical applications).
# NOTE: Depending on which of the prcision_* settings are enabled, this recipe builds a maximum of four 
#       libraries (one for each floating point precision), so the build steps will seem to repeat several times.

SINGLE = 'single'
DOUBLE = 'double'
LONGDOUBLE = 'longdouble'
QUAD = 'quad'
ALL = [SINGLE, DOUBLE, LONGDOUBLE, QUAD]

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class FFTWConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "fftw"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "3.3.10"
    # A brief description of what the dependency does.
    description = "C subroutine library for computing the Discrete Fourier Transform (DFT) in one or more dimensions"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "http://www.fftw.org/"
    # Declare the license of the dependency.
    license = "GPL-2.0"
    # State some keywords describing what the dependency does.
    topics = ("fftw", "dft", "dct", "dst")
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "precision": ALL + ['deprecated'],
        'precision_single': [True, False],
        'precision_double': [True, False],
        'precision_longdouble': [True, False],
        'precision_quad': [True, False],
        "openmp": [True, False],
        "threads": [True, False],
        "combinedthreads": [True, False],
        "simd": ["sse", "sse2", "avx", "avx2", False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "precision": 'deprecated',
        'precision_single': True,
        'precision_double': True,
        'precision_longdouble': True,
        'precision_quad': False,
        "openmp": False,
        "threads": False,
        "combinedthreads": False,
        "simd": False,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "fftw-{}.tar.gz".format(version)
    # Declares this dependency's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _all_precisions(self):
        return [p for p in ALL if self.options.get_safe(f"precision_{p}")]

    @property
    def _prec_suffix(self):
        return {
            "double": "",
            "single": "f",
            "longdouble": "l",
            'quad': 'q'
        }

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Has the same effect as the export_sources attribute, but in method form. Hence, this callback is called when
    # exporting (copying) files from the source repository to the source directory in Conan's workplace.
    def export_sources(self):
        export_conandata_patches(self)

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
        if not self.options.threads:
            del self.options.combinedthreads
        if self.options.precision != "deprecated":
            self.output.warning("precision options is deprecated! use dedicated options 'precision_single', 'precision_double', 'precision_longdouble' and 'precision_quad' instead")

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        cmake_layout(self, src_folder="src")

    # This callback is evoked to perform some validations (e.g. unsupported operating system) before building.
    # The detection of an invalid configuration must be signaled by raising an exception.
    def validate(self):
        if self.settings.os == "Windows" and self.options.shared:
            if self.options.openmp:
                raise ConanInvalidConfiguration("Shared fftw with openmp can't be built on Windows")
            if self.options.threads and not self.options.combinedthreads:
                raise ConanInvalidConfiguration("Shared fftw with threads and not combinedthreads can't be built on Windows")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "fftw-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "fftw-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = False
        tc.variables["ENABLE_OPENMP"] = self.options.openmp
        tc.variables["ENABLE_THREADS"] = self.options.threads
        tc.variables["WITH_COMBINED_THREADS"] = self.options.get_safe("combinedthreads", False)
        tc.variables["ENABLE_SSE"] = self.options.simd == "sse"
        tc.variables["ENABLE_SSE2"] = self.options.simd == "sse2"
        tc.variables["ENABLE_AVX"] = self.options.simd == "avx"
        tc.variables["ENABLE_AVX2"] = self.options.simd == "avx2"
        tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        def on_off(value):
            return "ON" if value else 'OFF'

        apply_conandata_patches(self)
        for current_precision in self._all_precisions:
            cmake = CMake(self)
            variables = {
                "ENABLE_FLOAT": on_off(current_precision == SINGLE),
                "ENABLE_LONG_DOUBLE": on_off(current_precision == LONGDOUBLE),
                "ENABLE_QUAD_PRECISION": on_off(current_precision == QUAD)
            }
            cmake.configure(variables=variables)
            cmake.build()
            cmake.install()

    # This is used to define which settings and options parcticipate in the hash calculation
    # for the package in Conan's cache.
    # In this case, we don't want the floating point precision setting to change the hash.
    def package_id(self):
        del self.info.options.precision

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYRIGHT", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        cmake_config_name = cmake_namespace = "FFTW3"

        self.cpp_info.set_property("cmake_file_name", cmake_config_name)

        # TODO: to remove in conan v2 once cmake_find_package_* & pkg_config generators removed
        self.cpp_info.filenames["cmake_find_package"] = cmake_config_name
        self.cpp_info.filenames["cmake_find_package_multi"] = cmake_config_name
        self.cpp_info.names["cmake_find_package"] = cmake_namespace
        self.cpp_info.names["cmake_find_package_multi"] = cmake_namespace

        for precision in self._all_precisions:
            prec_suffix = self._prec_suffix[precision]
            cmake_target_name = pkgconfig_name = lib_name = "fftw3" + prec_suffix
            component_name = f"fftwlib_{precision}"
            component = self.cpp_info.components[component_name]

            # TODO: back to global scope in conan v2 once cmake_find_package_* & pkg_config generators removed
            if self.options.openmp:
                component.libs.append(lib_name + "_omp")
            if self.options.threads and not self.options.combinedthreads:
                component.libs.append(lib_name + "_threads")
            self.cpp_info.components[component_name].libs.append(lib_name)
            if self.settings.os in ["Linux", "FreeBSD"]:
                component.system_libs.append("m")
                if precision == QUAD:
                    component.system_libs.extend(['quadmath'])
                if self.options.threads:
                    component.system_libs.append("pthread")
            self.cpp_info.components[component_name].includedirs.append(os.path.join(self.package_folder, "include"))

            component.names["cmake_find_package"] = cmake_target_name
            component.names["cmake_find_package_multi"] = cmake_target_name
            component.set_property("cmake_target_name", f"{cmake_namespace}::{cmake_target_name}")
            component.set_property("pkg_config_name", pkgconfig_name)
