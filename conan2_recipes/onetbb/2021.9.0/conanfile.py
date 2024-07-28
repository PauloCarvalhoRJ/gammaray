from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import cross_building
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import ( apply_conandata_patches, export_conandata_patches,
                                 copy, get, load, rmdir, rm, unzip )
from conan.tools.gnu import PkgConfigDeps
from conan.tools.scm import Version
import os
import re

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests the OneTBB library (an API for Threading Building Blocks).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class OneTBBConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "onetbb"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2021.9.0"
    # Declare the license of the dependency.
    license = "Apache-2.0"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://github.com/oneapi-src/oneTBB"
    # A brief description of what the dependency does.
    description = (
        "oneAPI Threading Building Blocks (oneTBB) lets you easily write parallel C++"
        " programs that take full advantage of multicore performance, that are portable, composable"
        " and have future-proof scalability.")
    # State some keywords describing what the dependency does.
    topics = ("tbb", "threading", "parallelism", "tbbmalloc")
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "shared-library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "tbbmalloc": [True, False],
        "tbbproxy": [True, False],
        "tbbbind": [True, False],
        "interprocedural_optimization": [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "tbbmalloc": True,
        "tbbproxy": True,
        "tbbbind": True,
        "interprocedural_optimization": True,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "onetbb-{}.tar.gz".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _has_tbbmalloc(self):
        return Version(self.version) < "2021.5.0" or self.options.get_safe("tbbmalloc")

    @property
    def _has_tbbproxy(self):
        return Version(self.version) < "2021.6.0" or self.options.get_safe("tbbproxy")

    @property
    def _tbbbind_hwloc_version(self):
        # TBB expects different variables depending on the version
        return "2_5" if Version(self.version) >= "2021.4.0" else "2_4"

    @property
    def _tbbbind_supported(self):
        return self.settings.os != "Macos" or Version(self.version) >= "2021.11.0"

    @property
    def _tbbbind_build(self):
        return self.options.get_safe("tbbbind", False) and self._tbbbind_supported

    @property
    def _tbbbind_explicit_hwloc(self):
        # during cross-compilation, oneTBB does not search for HWLOC and we need to specify it explicitly
        # but then oneTBB creates an imported SHARED target from provided paths, so we have to set shared=True
        return self._tbbbind_build and cross_building(self)

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Has the same effect as the export_sources attribute, but in method form. Hence, this callback is called when
    # exporting (copying) files from the source repository to the source directory in Conan's workplace.
    def export_sources(self):
        export_conandata_patches(self)

    # Configure or constrain the available options in a package before assigning them a value. A typical use case is
    # to remove an option in a given platform. For example, the SSE2 flag doesn’t exist in architectures different 
    # than 32 bits, so it should be removed in this method.
    def config_options(self):
        if Version(self.version) < "2021.5.0":
            del self.options.tbbmalloc
        if Version(self.version) < "2021.6.0":
            del self.options.tbbproxy
        if not self._tbbbind_supported:
            del self.options.tbbbind
        if Version(self.version) < "2021.6.0" or self.settings.os == "Android":
            del self.options.interprocedural_optimization

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        if Version(self.version) >= "2021.6.0" and not self.options.tbbmalloc:
            self.options.rm_safe("tbbproxy")

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        cmake_layout(self, src_folder="src")

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    def requirements(self):
        if self._tbbbind_build:
            self.requires("hwloc/2.9.3")

    # This callback is evoked to perform some validations (e.g. unsupported operating system) before building.
    # The detection of an invalid configuration must be signaled by raising an exception.
    def validate(self):
        if self.settings.compiler == "apple-clang" and Version(self.settings.compiler.version) < "11.0":
            raise ConanInvalidConfiguration(f"{self.ref} couldn't be built by apple-clang < 11.0")

        # Old versions used to have shared option before hwloc dependency was moved to shared only
        if self._tbbbind_explicit_hwloc and not self.dependencies["hwloc"].options.get_safe("shared", True):
            raise ConanInvalidConfiguration(f"{self.ref} requires hwloc:shared=True to be built.")

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        if self._tbbbind_build and not self._tbbbind_explicit_hwloc:
            if not self.conf.get("tools.gnu:pkg_config", check_type=str):
                self.tool_requires("pkgconf/2.1.0")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "onetbb-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "onetbb-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        toolchain = CMakeToolchain(self)
        toolchain.variables["TBB_TEST"] = False
        toolchain.variables["TBB_STRICT"] = False
        if Version(self.version) >= "2021.5.0":
            toolchain.variables["TBBMALLOC_BUILD"] = self.options.tbbmalloc
        if self.options.get_safe("interprocedural_optimization"):
            toolchain.variables["TBB_ENABLE_IPO"] = self.options.interprocedural_optimization
        if Version(self.version) >= "2021.6.0" and self.options.get_safe("tbbmalloc"):
            toolchain.variables["TBBMALLOC_PROXY_BUILD"] = self.options.tbbproxy
        toolchain.variables["TBB_DISABLE_HWLOC_AUTOMATIC_SEARCH"] = not self._tbbbind_build
        if self._tbbbind_explicit_hwloc:
            hwloc_package_folder = self.dependencies["hwloc"].package_folder
            hwloc_lib_name = ("hwloc.lib" if self.settings.os == "Windows" else
                              "libhwloc.dylib" if self.settings.os == "Macos" else
                              "libhwloc.so")
            toolchain.variables[f"CMAKE_HWLOC_{self._tbbbind_hwloc_version}_LIBRARY_PATH"] = \
                os.path.join(hwloc_package_folder, "lib", hwloc_lib_name).replace("\\", "/")
            toolchain.variables[f"CMAKE_HWLOC_{self._tbbbind_hwloc_version}_INCLUDE_PATH"] = \
                os.path.join(hwloc_package_folder, "include").replace("\\", "/")
            if self.settings.os == "Windows":
                toolchain.variables[f"CMAKE_HWLOC_{self._tbbbind_hwloc_version}_DLL_PATH"] = \
                    os.path.join(hwloc_package_folder, "bin", "hwloc.dll").replace("\\", "/")
        toolchain.generate()

        if self._tbbbind_build and not self._tbbbind_explicit_hwloc:
            deps = PkgConfigDeps(self)
            deps.generate()

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
        copy(self, "LICENSE.txt", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        rm(self, "*.pdb", os.path.join(self.package_folder, "bin"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "TBB")
        self.cpp_info.set_property("pkg_config_name", "tbb")
        self.cpp_info.set_property("cmake_config_version_compat", "AnyNewerVersion")

        def lib_name(name):
            if self.settings.build_type == "Debug":
                return name + "_debug"
            return name

        # tbb
        tbb = self.cpp_info.components["libtbb"]

        tbb.set_property("cmake_target_name", "TBB::tbb")
        tbb.libs = [lib_name("tbb")]
        if self.settings.os == "Windows":
            version_info = load(self,
                os.path.join(self.package_folder, "include", "oneapi", "tbb",
                             "version.h"))
            binary_version = re.sub(
                r".*" + re.escape("#define __TBB_BINARY_VERSION ") +
                r"(\d+).*",
                r"\1",
                version_info,
                flags=re.MULTILINE | re.DOTALL,
            )
            tbb.libs.append(lib_name(f"tbb{binary_version}"))
        if self.settings.os in ["Linux", "FreeBSD"]:
            tbb.system_libs = ["m", "dl", "rt", "pthread"]

        # tbbmalloc
        if self._has_tbbmalloc:
            tbbmalloc = self.cpp_info.components["tbbmalloc"]

            tbbmalloc.set_property("cmake_target_name", "TBB::tbbmalloc")
            tbbmalloc.libs = [lib_name("tbbmalloc")]
            if self.settings.os in ["Linux", "FreeBSD"]:
                tbbmalloc.system_libs = ["dl", "pthread"]

            # tbbmalloc_proxy
            if self._has_tbbproxy:
                tbbproxy = self.cpp_info.components["tbbmalloc_proxy"]

                tbbproxy.set_property("cmake_target_name", "TBB::tbbmalloc_proxy")
                tbbproxy.libs = [lib_name("tbbmalloc_proxy")]
                tbbproxy.requires = ["tbbmalloc"]
                if self.settings.os in ["Linux", "FreeBSD"]:
                    tbbproxy.system_libs = ["m", "dl", "pthread"]

        # TODO: to remove in conan v2 once cmake_find_package* & pkg_config generators removed
        self.cpp_info.names["cmake_find_package"] = "TBB"
        self.cpp_info.names["cmake_find_package_multi"] = "TBB"
        self.cpp_info.names["pkg_config"] = "tbb"
        tbb.names["cmake_find_package"] = "tbb"
        tbb.names["cmake_find_package_multi"] = "tbb"
