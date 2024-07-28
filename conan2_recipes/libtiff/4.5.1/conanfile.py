from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import ( apply_conandata_patches, copy, export_conandata_patches,
                                 get, replace_in_file, rm, rmdir, unzip
                              )
from conan.tools.microsoft import is_msvc
from conan.tools.scm import Version
import os

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests the TIFF image format support library.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class LibtiffConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "libtiff"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "4.5.1"
    # A brief description of what the dependency does.
    description = "Library for Tag Image File Format (TIFF)"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the license of the dependency.
    license = "libtiff"
    # Declare the home page of the dependency's project.
    homepage = "http://www.simplesystems.org/libtiff"
    # State some keywords describing what the dependency does.
    topics = ("tiff", "image", "bigtiff", "tagged-image-file-format")
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "lzma": [True, False],
        "jpeg": [False, "libjpeg", "libjpeg-turbo", "mozjpeg"],
        "zlib": [True, False],
        "libdeflate": [True, False],
        "zstd": [True, False],
        "jbig": [True, False],
        "webp": [True, False],
        "cxx":  [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "lzma": True,
        "jpeg": "libjpeg", #---> if you change this, check the possible additional requirements in the requirements() callback.
        "zlib": True,
        "libdeflate": True,
        "zstd": True,
        "jbig": True,
        "webp": True,
        "cxx":  True,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "libtiff-{}.tar.gz".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1"#, "cmake_installer/3.29.3" --> see build_requirements() callback

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _patch_sources(self):
        apply_conandata_patches(self)

        # remove FindXXXX for conan dependencies
        for module in ["Deflate", "JBIG", "JPEG", "LERC", "WebP", "ZSTD", "liblzma", "LibLZMA"]:
            rm(self, f"Find{module}.cmake", os.path.join(self.source_folder, "cmake"))

        # Export symbols of tiffxx for msvc shared
        replace_in_file(self, os.path.join(self.source_folder, "libtiff", "CMakeLists.txt"),
                              "set_target_properties(tiffxx PROPERTIES SOVERSION ${SO_COMPATVERSION})",
                              "set_target_properties(tiffxx PROPERTIES SOVERSION ${SO_COMPATVERSION} WINDOWS_EXPORT_ALL_SYMBOLS ON)")

        # Disable tools, test, contrib, man & html generation
        if Version(self.version) < "4.5.0":
            replace_in_file(self, os.path.join(self.source_folder, "CMakeLists.txt"),
                                  "add_subdirectory(tools)\nadd_subdirectory(test)\nadd_subdirectory(contrib)\nadd_subdirectory(build)\n"
                                  "add_subdirectory(man)\nadd_subdirectory(html)", "")

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
        if not self.options.cxx:
            self.settings.rm_safe("compiler.cppstd")
            self.settings.rm_safe("compiler.libcxx")

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        cmake_layout(self, src_folder="src")

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    def requirements(self):
        if self.options.zlib:
            self.requires("zlib/[>=1.2.11 <2]")
        if self.options.libdeflate:
            self.requires("libdeflate/1.19")
        if self.options.lzma:
            self.requires("xz_utils/5.4.5")
        if self.options.jpeg == "libjpeg":
            self.requires("libjpeg/9e")
        elif self.options.jpeg == "libjpeg-turbo":
            self.requires("libjpeg-turbo/3.0.2")
        elif self.options.jpeg == "mozjpeg":
            self.requires("mozjpeg/4.1.5")
        if self.options.jbig:
            self.requires("jbig/20160605")
        if self.options.zstd:
            self.requires("zstd/1.5.5")
        if self.options.webp:
            self.requires("libwebp/1.3.2")

    # This callback is evoked to perform some validations (e.g. unsupported operating system) before building.
    # The detection of an invalid configuration must be signaled by raising an exception.
    def validate(self):
        if self.options.libdeflate and not self.options.zlib:
            raise ConanInvalidConfiguration("libtiff:libdeflate=True requires libtiff:zlib=True")

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        if Version(self.version) >= "4.5.1":
            # https://github.com/conan-io/conan/issues/3482#issuecomment-662284561
            self.tool_requires("cmake_installer/[>=3.18 <4]")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "libtiff-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "libtiff-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["lzma"] = self.options.lzma
        tc.variables["jpeg"] = bool(self.options.jpeg)
        tc.variables["jpeg12"] = False
        tc.variables["jbig"] = self.options.jbig
        tc.variables["zlib"] = self.options.zlib
        tc.variables["libdeflate"] = self.options.libdeflate
        tc.variables["zstd"] = self.options.zstd
        tc.variables["webp"] = self.options.webp
        tc.variables["lerc"] = False # TODO: add lerc support for libtiff versions >= 4.3.0
        if Version(self.version) >= "4.5.0":
            # Disable tools, test, contrib, man & html generation
            tc.variables["tiff-tools"] = False
            tc.variables["tiff-tests"] = False
            tc.variables["tiff-contrib"] = False
            tc.variables["tiff-docs"] = False
        tc.variables["cxx"] = self.options.cxx
        # BUILD_SHARED_LIBS must be set in command line because defined upstream before project()
        tc.cache_variables["BUILD_SHARED_LIBS"] = bool(self.options.shared)
        tc.cache_variables["CMAKE_FIND_PACKAGE_PREFER_CONFIG"] = True
        tc.generate()
        deps = CMakeDeps(self)
        if Version(self.version) >= "4.5.1":
            deps.set_property("jbig", "cmake_target_name", "JBIG::JBIG")
            deps.set_property("xz_utils", "cmake_target_name", "liblzma::liblzma")
            deps.set_property("libdeflate", "cmake_file_name", "Deflate")
            deps.set_property("libdeflate", "cmake_target_name", "Deflate::Deflate")
        deps.generate()

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
        license_file = "COPYRIGHT" if Version(self.version) < "4.5.0" else "LICENSE.md"
        copy(self, license_file, src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"), ignore_case=True, keep_path=False)
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "TIFF")
        self.cpp_info.set_property("cmake_target_name", "TIFF::TIFF")
        self.cpp_info.set_property("pkg_config_name", f"libtiff-{Version(self.version).major}")
        suffix = "d" if is_msvc(self) and self.settings.build_type == "Debug" else ""
        if self.options.cxx:
            self.cpp_info.libs.append(f"tiffxx{suffix}")
        self.cpp_info.libs.append(f"tiff{suffix}")
        if self.settings.os in ["Linux", "Android", "FreeBSD", "SunOS", "AIX"]:
            self.cpp_info.system_libs.append("m")

        self.cpp_info.requires = []
        if self.options.zlib:
            self.cpp_info.requires.append("zlib::zlib")
        if self.options.libdeflate:
            self.cpp_info.requires.append("libdeflate::libdeflate")
        if self.options.lzma:
            self.cpp_info.requires.append("xz_utils::xz_utils")
        if self.options.jpeg == "libjpeg":
            self.cpp_info.requires.append("libjpeg::libjpeg")
        elif self.options.jpeg == "libjpeg-turbo":
            self.cpp_info.requires.append("libjpeg-turbo::jpeg")
        elif self.options.jpeg == "mozjpeg":
            self.cpp_info.requires.append("mozjpeg::libjpeg")
        if self.options.jbig:
            self.cpp_info.requires.append("jbig::jbig")
        if self.options.zstd:
            self.cpp_info.requires.append("zstd::zstd")
        if self.options.webp:
            self.cpp_info.requires.append("libwebp::libwebp")

        # TODO: to remove in conan v2 once cmake_find_package* & pkg_config generators removed
        self.cpp_info.names["cmake_find_package"] = "TIFF"
        self.cpp_info.names["cmake_find_package_multi"] = "TIFF"
