from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import ( 
    apply_conandata_patches, copy, export_conandata_patches, 
    get, replace_in_file, rmdir, unzip
)

import os

required_conan_version = ">=1.54.0"

# This recipe builds, installs and tests the md4c library (a Markdown format parser for C/C++ applications).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class Md4cConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "md4c"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "0.4.8"
    # A brief description of what the dependency does.
    description = "C Markdown parser. Fast. SAX-like interface. Compliant to CommonMark specification."
    # Declare the license of the dependency.
    license = "MIT"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://github.com/mity/md4c"
    # State some keywords describing what the dependency does.
    topics = ("markdown-parser", "markdown")
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "md2html": [True, False],
        "encoding": ["utf-8", "utf-16", "ascii"],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "md2html": True,
        "encoding": "utf-8",
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "md4c-release-{}.tar.gz".format(version)
    # Declares this dependency's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _patch_sources(self):
        apply_conandata_patches(self)
        # Honor encoding option
        replace_in_file(
            self,
            os.path.join(self.source_folder, "src", "CMakeLists.txt"),
            "COMPILE_FLAGS \"-DMD4C_USE_UTF8\"",
            "",
        )

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

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        cmake_layout(self, src_folder="src")

    # This callback is evoked to perform some validations (e.g. unsupported operating system) before building.
    # The detection of an invalid configuration must be signaled by raising an exception.
    def validate(self):
        if self.settings.os != "Windows" and self.options.encoding == "utf-16":
            raise ConanInvalidConfiguration(f"{self.ref} doesn't support utf-16 options on non-Windows platforms")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True)
        unzip(self, filename=os.path.join(self.source_folder, "..", "md4c-release-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "md4c-release-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_MD2HTML_EXECUTABLE"] = self.options.md2html
        if self.options.encoding == "utf-8":
            tc.preprocessor_definitions["MD4C_USE_UTF8"] = "1"
        elif self.options.encoding == "utf-16":
            tc.preprocessor_definitions["MD4C_USE_UTF16"] = "1"
        elif self.options.encoding == "ascii":
            tc.preprocessor_definitions["MD4C_USE_ASCII"] = "1"
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
        copy(self, pattern="LICENSE.md", dst=os.path.join(self.package_folder, "licenses"), src=self.source_folder)
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "share"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "md4c")

        self.cpp_info.components["_md4c"].set_property("cmake_target_name", "md4c::md4c")
        self.cpp_info.components["_md4c"].set_property("pkg_config_name", "md4c")
        self.cpp_info.components["_md4c"].libs = ["md4c"]
        if self.settings.os == "Windows" and self.options.encoding == "utf-16":
            self.cpp_info.components["_md4c"].defines.append("MD4C_USE_UTF16")

        self.cpp_info.components["md4c_html"].set_property("cmake_target_name", "md4c::md4c-html")
        self.cpp_info.components["md4c_html"].set_property("pkg_config_name", "md4c-html")
        self.cpp_info.components["md4c_html"].libs = ["md4c-html"]
        self.cpp_info.components["md4c_html"].requires = ["_md4c"]

        # workaround so that global target & pkgconfig file have all components while avoiding
        # to create unofficial target or pkgconfig file
        self.cpp_info.set_property("cmake_target_name", "md4c::md4c-html")
        self.cpp_info.set_property("pkg_config_name", "md4c-html")

        # TODO: to remove in conan v2
        self.cpp_info.components["_md4c"].names["cmake_find_package"] = "md4c"
        self.cpp_info.components["_md4c"].names["cmake_find_package_multi"] = "md4c"
        self.cpp_info.components["md4c_html"].names["cmake_find_package"] = "md4c-html"
        self.cpp_info.components["md4c_html"].names["cmake_find_package_multi"] = "md4c-html"
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
