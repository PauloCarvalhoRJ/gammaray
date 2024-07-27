from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import is_apple_os
from conan.tools.build import check_min_cppstd, valid_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, copy, rm, rmdir, save, unzip
from conan.tools.microsoft import is_msvc_static_runtime
from conan.tools.scm import Version
import os
import textwrap


required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests the GDCM library (I/O with DICOM medical image format files).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class GDCMConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "gdcm"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "3.0.21"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "library"
    # A brief description of what the dependency does.
    description = "C++ library for DICOM medical files"
    # Declare the license of the dependency.
    license = "BSD-3-Clause"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "http://gdcm.sourceforge.net/"
    # State some keywords describing what the dependency does.
    topics = ("dicom", "images", "medical-imaging")
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_json": [True, False],
        "with_openssl": [True, False],
        "with_zlibng": [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_json": True,
        "with_openssl": True,
        "with_zlibng": False, # --> if this is enabled, zlib-ng will be required
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "gdcm-{}.tar.bz2".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _min_cppstd(self):
        return 14

    def _patch_sources(self):
        apply_conandata_patches(self)
        rm(self, "Find*.cmake", os.path.join(self.source_folder, "CMake"))

    def _create_cmake_variables(self, variables_file):
        v = Version(self.version)
        content = textwrap.dedent(f"""\
            # The GDCM version number.
            set(GDCM_MAJOR_VERSION "{v.major}")
            set(GDCM_MINOR_VERSION "{v.minor}")
            set(GDCM_BUILD_VERSION "{v.patch}")

            get_filename_component(SELF_DIR "${{CMAKE_CURRENT_LIST_FILE}}" PATH)

            # The libraries.
            set(GDCM_LIBRARIES "")

            # The CMake macros dir.
            set(GDCM_CMAKE_DIR "")

            # The configuration options.
            set(GDCM_BUILD_SHARED_LIBS "{"ON" if self.options.shared else "OFF"}")

            set(GDCM_USE_VTK "OFF")

            # The "use" file.
            set(GDCM_USE_FILE ${{SELF_DIR}}/UseGDCM.cmake)

            # The VTK options.
            set(GDCM_VTK_DIR "")

            get_filename_component(GDCM_INCLUDE_ROOT "${{SELF_DIR}}/../../include/{self._gdcm_subdir}" ABSOLUTE)
            set(GDCM_INCLUDE_DIRS ${{GDCM_INCLUDE_ROOT}})
            get_filename_component(GDCM_LIB_ROOT "${{SELF_DIR}}/../../lib" ABSOLUTE)
            set(GDCM_LIBRARY_DIRS ${{GDCM_LIB_ROOT}})
        """)
        save(self, variables_file, content)

    def _create_cmake_module_alias_targets(self):
        module_file = os.path.join(self.package_folder, self._gdcm_cmake_module_aliases_path)
        targets = {library: f"GDCM::{library}" for library in self._gdcm_libraries}
        content = ""
        for alias, aliased in targets.items():
            content += textwrap.dedent(f"""\
                if(TARGET {aliased} AND NOT TARGET {alias})
                    add_library({alias} INTERFACE IMPORTED)
                    set_property(TARGET {alias} PROPERTY INTERFACE_LINK_LIBRARIES {aliased})
                endif()
            """)
        save(self, module_file, content)

    @property
    def _gdcm_builddir(self):
        return os.path.join("lib", self._gdcm_subdir)

    @property
    def _gdcm_cmake_module_aliases_path(self):
        return os.path.join("lib", self._gdcm_subdir, "conan-official-gdcm-targets.cmake")

    @property
    def _gdcm_cmake_variables_path(self):
        return os.path.join("lib", self._gdcm_subdir, "conan-official-gdcm-variables.cmake")

    @property
    def _gdcm_build_modules(self):
        return [self._gdcm_cmake_module_aliases_path, self._gdcm_cmake_variables_path]

    @property
    def _gdcm_libraries(self):
        gdcm_libs = ["gdcmCommon",
                     "gdcmDICT",
                     "gdcmDSED",
                     "gdcmIOD",
                     "gdcmjpeg12",
                     "gdcmjpeg16",
                     "gdcmjpeg8",
                     "gdcmMEXD",
                     "gdcmMSFF",
                     "socketxx"]
        if self.settings.os == "Windows":
            gdcm_libs.append("gdcmgetopt")
        return gdcm_libs

    @property
    def _gdcm_subdir(self):
        v = Version(self.version)
        return f"gdcm-{v.major}.{v.minor}"

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

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        cmake_layout(self, src_folder="src")

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    def requirements(self):
        self.requires("charls/2.4.2")
        self.requires("expat/[>=2.6.2 <3]")
        self.requires("openjpeg/2.5.2")
        if self.options.with_zlibng:
            self.requires("zlib-ng/2.1.6")
        else:
            self.requires("zlib/[>=1.2.11 <2]")
        if self.settings.os != "Windows":
            self.requires("util-linux-libuuid/2.39.2")
            if Version(self.version) >= Version("3.0.20"):
                self.requires("libiconv/1.17")
        if self.options.with_json:
            self.requires("json-c/0.17")
        if self.options.with_openssl:
            self.requires("openssl/[>=1.1 <4]")

    # This callback is evoked to perform some validations (e.g. unsupported operating system) before building.
    # The detection of an invalid configuration must be signaled by raising an exception.
    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, self._min_cppstd)
        if is_msvc_static_runtime(self) and self.options.shared:
            raise ConanInvalidConfiguration(f"{self.ref} does not support shared and static runtime together.")
        if self.info.options.with_zlibng:
            zlib_ng = self.dependencies["zlib-ng"]
            if not zlib_ng.options.zlib_compat:
                raise ConanInvalidConfiguration(f"{self.ref} requires the dependency option zlib-ng:zlib_compat=True")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "gdcm-{}.tar.bz2".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "gdcm-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["GDCM_BUILD_DOCBOOK_MANPAGES"] = False
        tc.variables["GDCM_BUILD_SHARED_LIBS"] = bool(self.options.shared)
        # FIXME: unvendor deps https://github.com/conan-io/conan-center-index/pull/5705#discussion_r647224146
        tc.variables["GDCM_USE_SYSTEM_CHARLS"] = True
        tc.variables["GDCM_USE_SYSTEM_EXPAT"] = True
        tc.variables["GDCM_USE_SYSTEM_JSON"] = self.options.with_json
        tc.variables["GDCM_USE_SYSTEM_OPENJPEG"] = True
        tc.variables["GDCM_USE_SYSTEM_OPENSSL"] = self.options.with_openssl
        tc.variables["GDCM_USE_SYSTEM_UUID"] = self.settings.os != "Windows"
        tc.variables["GDCM_USE_SYSTEM_ZLIB"] = True

        if not valid_min_cppstd(self, self._min_cppstd):
            tc.variables["CMAKE_CXX_STANDARD"] = self._min_cppstd

        # https://sourceforge.net/p/gdcm/bugs/548/
        tc.preprocessor_definitions["CHARLS_NO_DEPRECATED_WARNING"] = "1"

        #gdcm currently uses functionality that is deprecated since OpenSSL 1.1.0
        tc.preprocessor_definitions["OPENSSL_API_COMPAT"] = "0x10000000L"

        tc.generate()
        deps = CMakeDeps(self)
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
        copy(self, pattern="Copyright.txt", dst=os.path.join(self.package_folder, "licenses"), src=self.source_folder)
        cmake = CMake(self)
        cmake.install()

        if self.settings.os == "Windows":
            bin_dir = os.path.join(self.package_folder, "bin")
            rm(self, "[!gs]*.dll", bin_dir)
            rm(self, "*.pdb", bin_dir)

        rm(self, "[!U]*.cmake", os.path.join(self.package_folder, "lib", self._gdcm_subdir)) #leave UseGDCM.cmake untouched
        rmdir(self, os.path.join(self.package_folder, "share"))
        self._create_cmake_variables(os.path.join(self.package_folder, self._gdcm_cmake_variables_path))

        # TODO: to remove in conan v2 once cmake_find_package* generators removed
        self._create_cmake_module_alias_targets()

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "GDCM")
        self.cpp_info.set_property("cmake_build_modules", [self._gdcm_cmake_variables_path])

        for lib in self._gdcm_libraries:
            self.cpp_info.components[lib].set_property("cmake_target_name", lib)
            self.cpp_info.components[lib].libs = [lib]
            self.cpp_info.components[lib].includedirs = [os.path.join("include", self._gdcm_subdir)]
            self.cpp_info.components[lib].builddirs.append(self._gdcm_builddir)

            # TODO: to remove in conan v2 once cmake_find_package* generators removed
            self.cpp_info.components[lib].build_modules["cmake"] = [self._gdcm_cmake_module_aliases_path]
            self.cpp_info.components[lib].build_modules["cmake_find_package"] = self._gdcm_build_modules
            self.cpp_info.components[lib].build_modules["cmake_find_package_multi"] = self._gdcm_build_modules

        if self.options.with_openssl:
            self.cpp_info.components["gdcmCommon"].requires.append("openssl::openssl")

        def zlib():
            return "zlib-ng::zlib-ng" if self.options.with_zlibng else "zlib::zlib"

        self.cpp_info.components["gdcmDSED"].requires.extend(["gdcmCommon", zlib()])
        self.cpp_info.components["gdcmIOD"].requires.extend(["gdcmDSED", "gdcmCommon", "expat::expat"])
        self.cpp_info.components["gdcmMSFF"].requires.extend(["gdcmIOD", "gdcmDSED", "gdcmDICT", "charls::charls", "openjpeg::openjpeg"])
        if self.options.with_json:
            self.cpp_info.components["gdcmMSFF"].requires.append("json-c::json-c")
        if self.settings.os != "Windows":
            self.cpp_info.components["gdcmMSFF"].requires.append("util-linux-libuuid::util-linux-libuuid")
            if Version(self.version) >= Version("3.0.20"):
                self.cpp_info.components["gdcmMSFF"].requires.append("libiconv::libiconv")
        if not self.options.shared:
            self.cpp_info.components["gdcmDICT"].requires.extend(["gdcmDSED", "gdcmIOD"])
            self.cpp_info.components["gdcmMEXD"].requires.extend(["gdcmMSFF", "gdcmDICT", "gdcmDSED", "gdcmIOD", "socketxx"])
            self.cpp_info.components["gdcmMSFF"].requires.extend(["gdcmjpeg8", "gdcmjpeg12", "gdcmjpeg16"])

            if self.settings.os == "Windows":
                self.cpp_info.components["gdcmCommon"].system_libs = ["ws2_32", "crypt32"]
                self.cpp_info.components["gdcmMSFF"].system_libs = ["rpcrt4"]
                self.cpp_info.components["socketxx"].system_libs = ["ws2_32"]
            else:
                self.cpp_info.components["gdcmCommon"].system_libs = ["dl"]
                if is_apple_os(self):
                    self.cpp_info.components["gdcmCommon"].frameworks = ["CoreFoundation"]

        # TODO: to remove in conan v2 once cmake_find_package* generators removed
        self.cpp_info.names["cmake_find_package"] = "GDCM"
        self.cpp_info.names["cmake_find_package_multi"] = "GDCM"
