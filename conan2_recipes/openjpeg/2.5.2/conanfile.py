from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import (
                    apply_conandata_patches, copy, export_conandata_patches, 
                    get, rmdir, save, replace_in_file, unzip
                 )
from conan.tools.scm import Version
import os
import textwrap

required_conan_version = ">=1.54.0"

# This recipe builds, installs and tests the OpenJPEG library (JPEG 2000 codec).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class OpenjpegConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "openjpeg"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2.5.2"
    description = "OpenJPEG is an open-source JPEG 2000 codec written in C language."
    # A brief description of what the dependency does.
    license = "BSD-2-Clause"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://github.com/uclouvain/openjpeg"
    # State some keywords describing what the dependency does.
    topics = ("jpeg2000", "jp2", "openjpeg", "image", "multimedia", "format", "graphics")
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "build_codec": [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "build_codec": False,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "openjpeg-{}.tar.gz".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _patch_sources(self):
        apply_conandata_patches(self)
        # The finite-math-only optimization has no effect and can cause linking errors
        # when linked against glibc >= 2.31
        replace_in_file(self, os.path.join(self.source_folder, "CMakeLists.txt"),
                        "-ffast-math", "-ffast-math;-fno-finite-math-only")

    def _create_cmake_module_variables(self, module_file):
        content = textwrap.dedent(f"""\
            set(OPENJPEG_FOUND TRUE)
            if(DEFINED OpenJPEG_INCLUDE_DIRS)
                set(OPENJPEG_INCLUDE_DIRS ${{OpenJPEG_INCLUDE_DIRS}})
            endif()
            if(DEFINED OpenJPEG_LIBRARIES)
                set(OPENJPEG_LIBRARIES ${{OpenJPEG_LIBRARIES}})
            endif()
            set(OPENJPEG_MAJOR_VERSION "{Version(self.version).major}")
            set(OPENJPEG_MINOR_VERSION "{Version(self.version).minor}")
            set(OPENJPEG_BUILD_VERSION "{Version(self.version).patch}")
            set(OPENJPEG_BUILD_SHARED_LIBS {"TRUE" if self.options.shared else "FALSE"})
        """)
        save(self, module_file, content)

    def _create_cmake_module_alias_targets(self, module_file, targets):
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
    def _module_vars_rel_path(self):
        return os.path.join("lib", "cmake", f"conan-official-{self.name}-variables.cmake")

    @property
    def _module_target_rel_path(self):
        return os.path.join("lib", "cmake", f"conan-official-{self.name}-targets.cmake")

    @property
    def _openjpeg_subdir(self):
        openjpeg_version = Version(self.version)
        return f"openjpeg-{openjpeg_version.major}.{openjpeg_version.minor}"

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

    # This is used to define which settings and options parcticipate in the hash calculation
    # for the package in Conan's cache.
    # In this case, we only want package name and version take part in the package's hash calculation.
    def package_id(self):
        del self.info.options.build_codec # not used for the moment

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "openjpeg-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "openjpeg-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP"] = True
        tc.variables["BUILD_DOC"] = False
        tc.variables["BUILD_STATIC_LIBS"] = not self.options.shared
        tc.variables["BUILD_LUTS_GENERATOR"] = False
        tc.variables["BUILD_CODEC"] = False
        if Version(self.version) < "2.5.0":
            tc.variables["BUILD_MJ2"] = False
            tc.variables["BUILD_JPWL"] = False
            tc.variables["BUILD_JP3D"] = False
        tc.variables["BUILD_JPIP"] = False
        tc.variables["BUILD_VIEWER"] = False
        tc.variables["BUILD_JAVA"] = False
        tc.variables["BUILD_TESTING"] = False
        tc.variables["BUILD_PKGCONFIG_FILES"] = False
        tc.variables["OPJ_DISABLE_TPSOT_FIX"] = False
        tc.variables["OPJ_USE_THREAD"] = True
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
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", self._openjpeg_subdir))
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake", self._openjpeg_subdir))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        self._create_cmake_module_variables(
            os.path.join(self.package_folder, self._module_vars_rel_path)
        )
        # TODO: to remove in conan v2 once cmake_find_package* & pkg_config generators removed
        self._create_cmake_module_alias_targets(
            os.path.join(self.package_folder, self._module_target_rel_path),
            {"openjp2": "OpenJPEG::OpenJPEG"}
        )

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "OpenJPEG")
        self.cpp_info.set_property("cmake_target_name", "openjp2")
        self.cpp_info.set_property("cmake_build_modules", [self._module_vars_rel_path])
        self.cpp_info.set_property("pkg_config_name", "libopenjp2")
        self.cpp_info.includedirs.append(os.path.join("include", self._openjpeg_subdir))
        self.cpp_info.builddirs.append(os.path.join("lib", "cmake"))
        self.cpp_info.libs = ["openjp2"]
        if self.settings.os == "Windows" and not self.options.shared:
            self.cpp_info.defines.append("OPJ_STATIC")
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs = ["pthread", "m"]
        elif self.settings.os == "Android":
            self.cpp_info.system_libs = ["m"]

        # TODO: to remove in conan v2 once cmake_find_package* & pkg_config generators removed
        self.cpp_info.names["cmake_find_package"] = "OpenJPEG"
        self.cpp_info.names["cmake_find_package_multi"] = "OpenJPEG"
        self.cpp_info.build_modules["cmake_find_package"] = [self._module_target_rel_path]
        self.cpp_info.build_modules["cmake_find_package_multi"] = [self._module_target_rel_path]
        self.cpp_info.names["pkg_config"] = "libopenjp2"
