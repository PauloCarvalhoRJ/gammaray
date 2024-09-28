from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import ( apply_conandata_patches, export_conandata_patches, 
                                get, load, replace_in_file, save, copy, unzip )
from conan.tools.scm import Version
import os

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests Zlib 1.3.1 (a popular compression library).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class ZlibConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "zlib"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.3.1"
    # Hints Conan of what kind of information to propagate to consumer packages (e.g. header dir, lib dir, etc.).
    package_type = "library"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://zlib.net"
    # Declare the license of the dependency.
    license = "Zlib"
    # A brief description of what this dependency does.
    description = ("A Massively Spiffy Yet Delicately Unobtrusive Compression Library "
                   "(Also Free, Not to Mention Unencumbered by Patents)")
    # State some keywords describing what the dependency does.
    topics = ("zlib", "compression")
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # Declare any information in addition to the ones in the standard ones in 'settings' attribute.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    # Set the default values for the options in the 'options' attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "zlib-{}.tar.gz".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _is_mingw(self):
        return self.settings.os == "Windows" and self.settings.compiler == "gcc"

    def _patch_sources(self):
        apply_conandata_patches(self)

        is_apple_clang12 = self.settings.compiler == "apple-clang" and Version(self.settings.compiler.version) >= "12.0"
        if not is_apple_clang12:
            for filename in ['zconf.h', 'zconf.h.cmakein', 'zconf.h.in']:
                filepath = os.path.join(self.source_folder, filename)
                replace_in_file(self, filepath,
                                      '#ifdef HAVE_UNISTD_H    '
                                      '/* may be set to #if 1 by ./configure */',
                                      '#if defined(HAVE_UNISTD_H) && (1-HAVE_UNISTD_H-1 != 0)')
                replace_in_file(self, filepath,
                                      '#ifdef HAVE_STDARG_H    '
                                      '/* may be set to #if 1 by ./configure */',
                                      '#if defined(HAVE_STDARG_H) && (1-HAVE_STDARG_H-1 != 0)')

    def _extract_license(self):
        tmp = load(self, os.path.join(self.source_folder, "zlib.h"))
        license_contents = tmp[2:tmp.find("*/", 1)]
        return license_contents

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Does the same as the export_sources attribute, but in method form.
    def export_sources(self):
        export_conandata_patches(self)

    # Modifies any settings and options (e.g. some of them may not make sense in a given OS).
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    # Makes some adjustments to the configuration before building (e.g. remove or change 
    # conflicting options).
    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    # Customize the default Conan workspace before building (e.g. define the name for the source directory).
    def layout(self):
        cmake_layout(self, src_folder="src")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], destination=self.source_folder, strip_root=True) --> archive is locally present, no need to download it
        unzip(self, filename=os.path.join(self.source_folder, "..", "zlib-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "zlib-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["SKIP_INSTALL_ALL"] = False
        tc.variables["SKIP_INSTALL_LIBRARIES"] = False
        tc.variables["SKIP_INSTALL_HEADERS"] = False
        tc.variables["SKIP_INSTALL_FILES"] = True
        # Correct for misuse of "${CMAKE_INSTALL_PREFIX}/" in CMakeLists.txt
        tc.variables["INSTALL_LIB_DIR"] = "lib"
        tc.variables["INSTALL_INC_DIR"] = "include"
        tc.variables["ZLIB_BUILD_EXAMPLES"] = False
        tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        self._patch_sources()
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        save(self, os.path.join(self.package_folder, "licenses", "LICENSE"), self._extract_license())
        cmake = CMake(self)
        cmake.install()

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "ZLIB")
        self.cpp_info.set_property("cmake_target_name", "ZLIB::ZLIB")
        self.cpp_info.set_property("pkg_config_name", "zlib")
        if self.settings.os == "Windows" and not self._is_mingw:
            libname = "zdll" if self.options.shared else "zlib"
        else:
            libname = "z"
        self.cpp_info.libs = [libname]

        self.cpp_info.names["cmake_find_package"] = "ZLIB"
        self.cpp_info.names["cmake_find_package_multi"] = "ZLIB"
