from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import is_apple_os
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import ( 
    apply_conandata_patches, copy, export_conandata_patches, 
    get, replace_in_file, rm, rmdir, unzip, chdir
)
from conan.tools.microsoft import is_msvc
from conan.tools.scm import Version
import os

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests libPNG 1.6.42.


# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class LibpngConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "libpng"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.6.42"
    # A brief description of what this recipe does.
    description = "libpng is the official PNG file format reference library."
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "http://www.libpng.org"
    # Declare the license of the dependency.
    license = "libpng-2.0"
    # State some keywords describing what the dependency does.
    topics = ("png", "graphics", "image")
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "neon": [True, "check", False],
        "msa": [True, False],
        "sse": [True, False],
        "vsx": [True, False],
        "api_prefix": ["ANY"],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "neon": True,
        "msa": True,
        "sse": True,
        "vsx": True,
        "api_prefix": "",
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "libpng-{}.tar.xz".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    # Returns whether we're using CLang compiler in Windows.
    @property
    def _is_clang_cl(self):
        return self.settings.os == "Windows" and self.settings.compiler == "clang" and \
               self.settings.compiler.get_safe("runtime")

    # Return whether there is Neon support.
    @property
    def _has_neon_support(self):
        return "arm" in self.settings.arch

    # Return whether there is MSA support.
    @property
    def _has_msa_support(self):
        return "mips" in self.settings.arch

    # Return whether there is SSE support.
    @property
    def _has_sse_support(self):
        return self.settings.arch in ["x86", "x86_64"]

    # Return whether there is VSX support.
    @property
    def _has_vsx_support(self):
        return "ppc" in self.settings.arch

    # Used to redefine values in some CMake variables.
    @property
    def _neon_msa_sse_vsx_mapping(self):
        return {
            "True": "on",
            "False": "off",
            "check": "check",
        }

    # If there's a conan_data.yml accompaining this recipe, applies the source code patches declared there.
    # This is currently not being used.
    def _patch_sources(self):
        apply_conandata_patches(self)

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Has the same effect as the export_sources attribute, but in method form. Hence, this callback is called when
    # exporting (copying) files from the source repository to the source directory in Conan's workplace.
    def export_sources(self):
        # export_conandata_patches(self) --> conandata.yml was removed (no patches actually)
        copy(self, "conan_cmake_project_include.cmake", self.recipe_folder, os.path.join(self.export_sources_folder, "src"))

    # Configure or constrain the available options in a package before assigning them a value. A typical use case is
    # to remove an option in a given platform. For example, the SSE2 flag doesn’t exist in architectures different 
    # than 32 bits, so it should be removed in this method.
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
        if not self._has_neon_support:
            del self.options.neon
        if not self._has_msa_support:
            del self.options.msa
        if not self._has_sse_support:
            del self.options.sse
        if not self._has_vsx_support:
            del self.options.vsx

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        cmake_layout(self, src_folder="src")

    # Does the same as the requires attribute, but in method form (e.g. can use an if: statement).
    def requirements(self):
        self.requires("zlib/[>=1.2.11 <2]")

    # This callback is evoked to perform some validations (e.g. unsupported operating system) before building.
    # The detection of an invalid configuration must be signaled by raising an exception.
    def validate(self):
        if Version(self.version) < "1.6" and self.settings.arch == "armv8" and is_apple_os(self):
            raise ConanInvalidConfiguration(f"{self.ref} currently does not building for {self.settings.os} {self.settings.arch}. Contributions are welcomed")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> conan_data.yml was removed (archive is locally present, no need to download it)
        unzip(self, filename=os.path.join(self.source_folder, "..", "libpng-{}.tar.xz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "libpng-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["PNG_TESTS"] = False
        tc.cache_variables["PNG_SHARED"] = self.options.shared
        tc.cache_variables["PNG_STATIC"] = not self.options.shared
        tc.cache_variables["PNG_DEBUG"] = self.settings.build_type == "Debug"
        tc.cache_variables["PNG_PREFIX"] = self.options.api_prefix
        if Version(self.version) < "1.6.38":
            tc.cache_variables["CMAKE_PROJECT_libpng_INCLUDE"] = os.path.join(self.source_folder, "conan_cmake_project_include.cmake")
        if self._has_neon_support:
            tc.variables["PNG_ARM_NEON"] = self._neon_msa_sse_vsx_mapping[str(self.options.neon)]
        if self._has_msa_support:
            tc.variables["PNG_MIPS_MSA"] = self._neon_msa_sse_vsx_mapping[str(self.options.msa)]
        if self._has_sse_support:
            tc.variables["PNG_INTEL_SSE"] = self._neon_msa_sse_vsx_mapping[str(self.options.sse)]
        if self._has_vsx_support:
            tc.variables["PNG_POWERPC_VSX"] = self._neon_msa_sse_vsx_mapping[str(self.options.vsx)]
        if Version(self.version) >= "1.6.41":
            tc.variables["PNG_FRAMEWORK"] = False  # changed from False to True by default in PNG 1.6.41
            tc.variables["PNG_TOOLS"] = False
        elif Version(self.version) >= "1.6.38":
            tc.variables["PNG_EXECUTABLES"] = False

        # Make sure we are not using a system-installed zlib (e.g. common in Linux), which can 
        # be of a version different from the desired one installed in Conan cache.
        # The -I of GCC takes precedence over system-wide include directories.
        include_path_prefix = "/I" if is_msvc(self) else "-I"
        cmake_cxx_flags = ""
        cmake_c_flags = ""
        for zlib_include_dir in self.dependencies["zlib"].cpp_info.includedirs:
            #CMake compiler flags are cumulative (no need to do something like a = a + new_stuff),
            #but Conan does not do like that.
            cmake_cxx_flags = cmake_cxx_flags + include_path_prefix + zlib_include_dir + " "
            cmake_c_flags = cmake_c_flags + include_path_prefix + zlib_include_dir + " "
        tc.cache_variables["CMAKE_CXX_FLAGS"] = cmake_cxx_flags
        tc.cache_variables["CMAKE_C_FLAGS"] = cmake_c_flags

        tc.cache_variables["CMAKE_MACOSX_BUNDLE"] = False
        tc.generate()
        tc = CMakeDeps(self)
        tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        # self._patch_sources() --> conandata.yml was removed (no patches actually)
        with chdir(self, self.source_folder):
            replace_in_file(self, 'CMakeLists.txt',
                                  'ZLIB::ZLIB', # --> How zlib's CMake target is expected in libpng's original source.
                                  'zlib::zlib') # --> GammaRay's zlib's .cmake file declares itself in lower case.

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
        if self.options.shared:
            rm(self, "*[!.dll]", os.path.join(self.package_folder, "bin"))
        else:
            rmdir(self, os.path.join(self.package_folder, "bin"))
        rmdir(self, os.path.join(self.package_folder, "lib", "libpng"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "share"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        major_min_version = f"{Version(self.version).major}{Version(self.version).minor}"

        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "PNG")
        self.cpp_info.set_property("cmake_target_name", "PNG::PNG")
        self.cpp_info.set_property("pkg_config_name", "libpng")
        self.cpp_info.set_property("pkg_config_aliases", [f"libpng{major_min_version}"])

        prefix = "lib" if (is_msvc(self) or self._is_clang_cl) else ""
        suffix = major_min_version if self.settings.os == "Windows" else ""
        if is_msvc(self) or self._is_clang_cl:
            suffix += "_static" if not self.options.shared else ""
        suffix += "d" if self.settings.os == "Windows" and self.settings.build_type == "Debug" else ""
        self.cpp_info.libs = [f"{prefix}png{suffix}"]
        if self.settings.os in ["Linux", "Android", "FreeBSD", "SunOS", "AIX"]:
            self.cpp_info.system_libs.append("m")

        # TODO: Remove after Conan 2.0
        self.cpp_info.names["cmake_find_package"] = "PNG"
        self.cpp_info.names["cmake_find_package_multi"] = "PNG"
