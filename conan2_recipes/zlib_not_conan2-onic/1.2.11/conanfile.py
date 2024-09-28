import os
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.files import unzip, patch, chdir, replace_in_file, mkdir, load, save, copy
import shutil
import subprocess

# This recipe builds, installs (packages) and tests zlib and, optionally, an accompaining library called minizip.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class ZlibConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "zlib"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.2.11"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://zlib.net"
    # Declare the license of the dependency.
    license = "Zlib"
    # A brief description of what this recipe does.
    description = ("A Massively Spiffy Yet Delicately Unobtrusive Compression Library "
                   "(Also Free, Not to Mention Unencumbered by Patents)")
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {"shared": [True, False],
               "fPIC": [True, False],
               "minizip": [True, False],
               # FIXME: THe os_version seems redundant and unused, should be removed
               "os_version": ["linux", "windows", "other"],
               }
    # Declare which are the default values to the options in the options attribute.
    default_options = {"shared": False, "fPIC": True, "minizip": True}
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = ["zlib-{}.tar.gz".format(version),
                       "CMakeLists.txt",
                       "CMakeLists_minizip.txt",
                       "patches/**"]
    generators = "CMakeDeps"
    # Declare where the tarball with the dependency's source code was downloaded from.
    source_url = "https://zlib.net/zlib-1.2.11.tar.gz"
    _source_subfolder = "source_subfolder"  # TODO :This can be improved with "layout()" and "cmake_layout()"
    # State some keywords describing what the dependency does.
    topics = ("conan", "zlib", "compression")
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------ZlibConan methods (not Conan API)----------------------------------------

    # Called by build(). Returns the name of the archive containing the zlib source code.
    @property
    def _source_zip_filename(self):
        return "zlib-{}.tar.gz".format(self.version)

    # Called by _build_zlib().
    def _build_zlib_cmake(self):
        cmake = CMake(self)
        #cmake.configure(build_dir=".")
        cmake.configure()
        # we need to build only libraries without test example/example64 and minigzip/minigzip64
        make_target = "zlib" if self.options.shared else "zlibstatic"
        #cmake.build(build_dir=".", target=make_target)
        cmake.build(target=make_target)
        if self.options.minizip:
            cmake.build(target="minizip")

    # Called by build().
    def _build_zlib(self):

        minizip_dir = os.path.join(self._source_subfolder, 'contrib', 'minizip')
        os.rename(os.path.join(self.source_folder, "CMakeLists_minizip.txt"), os.path.join(minizip_dir, 'CMakeLists.txt'))
        # Apply patches.  These are changes to the source code applied in the build diretory 
        # (the original source files remain unchanged in the source repository).
        # See conandata.yml file that comes along this recipe in the source repository for info
        # on the patches to be applied.
        patches = self.conan_data["patches"][self.version]
        for p in patches:
            patch(self, base_path=p["base_path"], patch_file=p["patch_file"])

        with chdir(self, self._source_subfolder):
            # https://github.com/madler/zlib/issues/268
            replace_in_file(self, 'gzguts.h',
                                  '#if defined(_WIN32) || defined(__CYGWIN__)',
                                  '#if defined(_WIN32) || defined(__MINGW32__)')

            is_apple_clang12 = self.settings.compiler == "apple-clang" and tools.Version(self.settings.compiler.version) >= "12.0"
            if not is_apple_clang12:
                for filename in ['zconf.h', 'zconf.h.cmakein', 'zconf.h.in']:
                    replace_in_file(self, filename,
                                          '#ifdef HAVE_UNISTD_H    '
                                          '/* may be set to #if 1 by ./configure */',
                                          '#if defined(HAVE_UNISTD_H) && (1-HAVE_UNISTD_H-1 != 0)')
                    replace_in_file(self, filename,
                                          '#ifdef HAVE_STDARG_H    '
                                          '/* may be set to #if 1 by ./configure */',
                                          '#if defined(HAVE_STDARG_H) && (1-HAVE_STDARG_H-1 != 0)')
            #mkdir(self, "_build")
            #with chdir(self, "_build"): # this _build directory stays empty...
                #self._build_zlib_cmake()
            self._build_zlib_cmake()

    # Called by package().
    # The different toolsets create different library names, so this method standardizes them.
    def _rename_libraries(self):
        if self.settings.os == "Windows":
            lib_path = os.path.join(self.package_folder, "lib")
            suffix = "d" if self.settings.build_type == "Debug" else ""

            if self.options.shared:
                #if self.settings.compiler == "Visual Studio":
                if self.settings.compiler == "msvc":
                    current_lib = os.path.join(lib_path, "zlib%s.lib" % suffix)
                    os.rename(current_lib, os.path.join(lib_path, "zlib.lib"))
            else:  # static libaries enabled (self.options.shared==False)
                #if self.settings.compiler == "Visual Studio":
                if self.settings.compiler == "msvc":
                    current_lib = os.path.join(lib_path, "zlibstatic%s.lib" % suffix)
                    os.rename(current_lib, os.path.join(lib_path, "zlib.lib"))
                elif self.settings.compiler == "gcc":
                    if self.settings.os != "Windows" or not self.settings.os.subsystem:
                        current_lib = os.path.join(lib_path, "libzlibstatic.a")
                        os.rename(current_lib, os.path.join(lib_path, "libzlib.a"))
                elif self.settings.compiler == "clang":
                    current_lib = os.path.join(lib_path, "zlibstatic.lib")
                    os.rename(current_lib, os.path.join(lib_path, "zlib.lib"))

    #---------------------------------------Methods of ConanFile's interface----------------------------------------

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
        del self.settings.compiler.cppstd
        if self.settings.os == "Windows":
            self.options.os_version = "windows"
        elif self.settings.os == "Linux":
            del self.settings.compiler.libcxx
            self.options.os_version = "linux"
        else:
            del self.settings.compiler.libcxx
            self.options.os_version = "other"

    # Runs after the computation and installation of the dependency graph. This means that it will run after a conan
    # install command, or when a package is being built in the cache, it will be run before calling the build() method.
    # E.g. CMake is commanded to generate the Makefile or VS Solution files.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_MINIZIP"] = bool(self.options.minizip)
        tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        unzip(self, filename=os.path.join(self.source_folder, self._source_zip_filename), destination=self.build_folder, keep_permissions=True)
        shutil.move("zlib-{}".format(self.version), self._source_subfolder)
        self._build_zlib()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    def package(self):
        # Extract the License/s from the header to a file
        build_dir = self.build_folder
        lib = os.path.join(self.package_folder, "lib")
        bin = os.path.join(self.package_folder, "bin")
        include = os.path.join(self.package_folder, "include")
        with chdir(self, os.path.join(self.source_folder, self._source_subfolder)):
            tmp = load(self, "zlib.h")
            license_contents = tmp[2:tmp.find("*/", 1)]
            save(self, "LICENSE", license_contents)

        # Copy the license files
        copy(self, pattern="LICENSE", src=self._source_subfolder, dst=os.path.join(self.package_folder, "licenses"))

        # Copy headers
        for header in ["*zlib.h", "*zconf.h", "*zip.h", "*ioapi.h", "*minizip_extern.h", 
                       "*iowin32.h", "*mztools.h"]:
            copy(self, pattern=header, dst=include, src=self._source_subfolder, keep_path=False)
            copy(self, pattern=header, dst=include, src="_build", keep_path=False)

        # Copying static and dynamic libs
        if self.options.shared:
            copy(self, pattern="*.dylib*", dst=lib, src=build_dir, keep_path=False, symlinks=True)
            copy(self, pattern="*.so*", dst=lib, src=build_dir, keep_path=False, symlinks=True)
            copy(self, pattern="*.dll", dst=bin, src=build_dir, keep_path=False)
            copy(self, pattern="*.dll.a", dst=lib, src=build_dir, keep_path=False)
        else:
            copy(self, pattern="*.a", dst=lib, src=build_dir, keep_path=False)
        copy(self, pattern="*.lib", dst=lib, src=build_dir, keep_path=False)

        self._rename_libraries()

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `build_requires = ...` attributes.
    def package_info(self):
        # Attention: minizip *must* come *before* zlib in the libs list with GCC.
        #            order is important when linking against the static libraries.
        #            this behavior was observed with GCC 9.3.1
        if self.options.minizip:
            self.cpp_info.libs.append('minizip')
            if self.options.shared:
                self.cpp_info.defines.append('MINIZIP_DLL')
        self.cpp_info.libs.append("zlib" if self.settings.os == "Windows" and not self.settings.os.subsystem else "z")
        self.cpp_info.name = "zlib"

