from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, get, rmdir, unzip
from conan.tools.microsoft import is_msvc
from conan.tools.scm import Version
import os

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests the libwebp library (codec for the WebP image format).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class LibwebpConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "libwebp"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.3.2"
    # A brief description of what the dependency does.
    description = "Library to encode and decode images in WebP format"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://chromium.googlesource.com/webm/libwebp"
    # State some keywords describing what the dependency does.
    topics = ("image", "libwebp", "webp", "decoding", "encoding")
    # Declare the license of the dependency.
    license = "BSD-3-Clause"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_simd": [True, False],
        "near_lossless": [True, False],
        "swap_16bit_csp": [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_simd": True,
        "near_lossless": True,
        "swap_16bit_csp": False,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "libwebp-{}.tar.gz".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

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
        unzip(self, filename=os.path.join(self.source_folder, "..", "libwebp-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "libwebp-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        # should be an option but it doesn't work yet
        tc.variables["WEBP_ENABLE_SIMD"] = self.options.with_simd
        tc.variables["WEBP_NEAR_LOSSLESS"] = self.options.near_lossless
        tc.variables["WEBP_ENABLE_SWAP_16BIT_CSP"] = self.options.swap_16bit_csp
        # avoid finding system libs
        tc.variables["CMAKE_DISABLE_FIND_PACKAGE_GIF"] = True
        tc.variables["CMAKE_DISABLE_FIND_PACKAGE_PNG"] = True
        tc.variables["CMAKE_DISABLE_FIND_PACKAGE_TIFF"] = True
        tc.variables["CMAKE_DISABLE_FIND_PACKAGE_JPEG"] = True
        tc.variables["WEBP_BUILD_ANIM_UTILS"] = False
        tc.variables["WEBP_BUILD_CWEBP"] = False
        tc.variables["WEBP_BUILD_DWEBP"] = False
        tc.variables["WEBP_BUILD_IMG2WEBP"] = False
        tc.variables["WEBP_BUILD_GIF2WEBP"] = False
        tc.variables["WEBP_BUILD_VWEBP"] = False
        tc.variables["WEBP_BUILD_EXTRAS"] = False
        tc.variables["WEBP_BUILD_WEBPINFO"] = False
        if Version(self.version) >= "1.2.1":
            tc.variables["WEBP_BUILD_LIBWEBPMUX"] = True
        tc.variables["WEBP_BUILD_WEBPMUX"] = False
        if self.options.shared and is_msvc(self):
            # Building a dll (see fix-dll-export patch)
            tc.preprocessor_definitions["WEBP_DLL"] = 1
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
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "share"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "WebP")
        self.cpp_info.set_property("pkg_config_name", "libwebp-all-do-not-use")

        # webpdecoder
        self.cpp_info.components["webpdecoder"].set_property("cmake_target_name", "WebP::webpdecoder")
        self.cpp_info.components["webpdecoder"].set_property("pkg_config_name", "libwebpdecoder")
        self.cpp_info.components["webpdecoder"].libs = ["webpdecoder"]
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.components["webpdecoder"].system_libs = ["m", "pthread"]

        # webp
        self.cpp_info.components["webp"].set_property("cmake_target_name", "WebP::webp")
        self.cpp_info.components["webp"].set_property("pkg_config_name", "libwebp")
        self.cpp_info.components["webp"].libs = ["webp"]
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.components["webp"].system_libs = ["m", "pthread"]

        if Version(self.version) >= "1.3.0":
            # sharpyuv
            self.cpp_info.components["sharpyuv"].set_property("cmake_target_name", "WebP::sharpyuv")
            self.cpp_info.components["sharpyuv"].set_property("pkg_config_name", "libsharpyuv")
            self.cpp_info.components["sharpyuv"].libs = ["sharpyuv"]
            if self.settings.os in ["Linux", "FreeBSD"]:
                self.cpp_info.components["sharpyuv"].system_libs = ["m", "pthread"]
            # note: webp now depends on sharpyuv
            self.cpp_info.components["webp"].requires = ["sharpyuv"]

        # webpdemux
        self.cpp_info.components["webpdemux"].set_property("cmake_target_name", "WebP::webpdemux")
        self.cpp_info.components["webpdemux"].set_property("pkg_config_name", "libwebpdemux")
        self.cpp_info.components["webpdemux"].libs = ["webpdemux"]
        self.cpp_info.components["webpdemux"].requires = ["webp"]

        # webpmux
        self.cpp_info.components["webpmux"].set_property("cmake_target_name", "WebP::libwebpmux")
        self.cpp_info.components["webpmux"].set_property("pkg_config_name", "libwebpmux")
        self.cpp_info.components["webpmux"].libs = ["webpmux"]
        self.cpp_info.components["webpmux"].requires = ["webp"]
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.components["webpmux"].system_libs = ["m"]

        # TODO: to remove in conan v2 once cmake_find_package_* generators removed
        self.cpp_info.names["cmake_find_package"] = "WebP"
        self.cpp_info.names["cmake_find_package_multi"] = "WebP"
        self.cpp_info.names["pkg_config"] = "libwebp-all-do-not-use"
        self.cpp_info.components["webpmux"].names["cmake_find_package"] = "libwebpmux"
        self.cpp_info.components["webpmux"].names["cmake_find_package_multi"] = "libwebpmux"
