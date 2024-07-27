import glob
import os
import textwrap

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import cross_building, check_min_cppstd, valid_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import ( apply_conandata_patches, copy, export_conandata_patches,
                                    get, replace_in_file, rm, rmdir, save, unzip
                              )
from conan.tools.scm import Version

required_conan_version = ">=1.54.0"

# This recipe builds, installs and tests the HDF5 format support library.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class Hdf5Conan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "hdf5"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.14.1"
    # A brief description of what the dependency does.
    description = "HDF5 is a data model, library, and file format for storing and managing data."
    # Declare the license of the dependency.
    license = "BSD-3-Clause"
    # State some keywords describing what the dependency does.
    topics = "hdf", "data"
    # Declare the home page of the dependency's project.
    homepage = "https://portal.hdfgroup.org/display/HDF5/HDF5"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "enable_cxx": [True, False],
        "hl": [True, False],
        "threadsafe": [True, False],
        "with_zlib": [True, False],
        "szip_support": [None, "with_libaec", "with_szip"],
        "szip_encoding": [True, False],
        "parallel": [True, False],
        "enable_unsupported": [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "enable_cxx": True,
        "hl": True,
        "threadsafe": False,
        "with_zlib": True,
        "szip_support": None,
        "szip_encoding": False,
        "parallel": False,
        "enable_unsupported": False
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "hdf5-{}.tar.gz".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1" #, "cmake_installer/3.29.3" --> see build_requirements() callback

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _min_cppstd(self):
        if Version(self.version) < "1.14.0":
            return "98"
        return "11"

    def _inject_stdlib_flag(self, tc):
        if self.settings.os in ["Linux", "FreeBSD"] and self.settings.compiler == "clang":
            cpp_stdlib = f" -stdlib={self.settings.compiler.libcxx}".rstrip("1")  # strip 11 from stdlibc++11
            tc.variables["CMAKE_CXX_FLAGS"] = tc.variables.get("CMAKE_CXX_FLAGS", "") + cpp_stdlib
        return tc

    def _components(self):
        hdf5_requirements = []
        if self.options.with_zlib:
            hdf5_requirements.append("zlib::zlib")
        if self.options.szip_support == "with_libaec":
            hdf5_requirements.append("libaec::libaec")
        elif self.options.szip_support == "with_szip":
            hdf5_requirements.append("szip::szip")
        if self.options.parallel:
            hdf5_requirements.append("openmpi::openmpi")

        return {
            "hdf5_c": {"component": "C", "alias_target": "hdf5", "requirements": hdf5_requirements},
            "hdf5_hl": {"component": "HL", "alias_target": "hdf5_hl", "requirements": ["hdf5_c"]},
            "hdf5_cpp": {"component": "CXX", "alias_target": "hdf5_cpp", "requirements": ["hdf5_c"]},
            "hdf5_hl_cpp": {"component": "HL_CXX", "alias_target": "hdf5_hl_cpp", "requirements": ["hdf5_c", "hdf5_cpp", "hdf5_hl"]},
        }

    def _create_cmake_module_alias_targets(self, module_file, targets):
        content = ""
        for alias, aliased in targets.items():
            content += textwrap.dedent(f"""\
                if(TARGET {aliased} AND NOT TARGET {alias})
                    add_library({alias} INTERFACE IMPORTED)
                    set_property(TARGET {alias} PROPERTY INTERFACE_LINK_LIBRARIES {aliased})
                endif()
            """)

        # add the additional hdf5_hl_cxx target when both CXX and HL components are specified
        content += textwrap.dedent("""\
                if(TARGET HDF5::HL AND TARGET HDF5::CXX AND NOT TARGET hdf5::hdf5_hl_cpp)
                    add_library(hdf5::hdf5_hl_cpp INTERFACE IMPORTED)
                    set_property(TARGET hdf5::hdf5_hl_cpp PROPERTY INTERFACE_LINK_LIBRARIES HDF5::HL_CXX)
                endif()
            """)
        save(self, module_file, content)

    def _create_cmake_module_variables(self, module_file, is_parallel):
        content = "set(HDF5_IS_PARALLEL {})".format("ON" if is_parallel else "OFF")
        save(self, module_file, content)

    @property
    def _module_targets_file_rel_path(self):
        return os.path.join("lib", "cmake",
                            f"conan-official-{self.name}-targets.cmake")

    @property
    def _module_variables_file_rel_path(self):
        return os.path.join("lib", "cmake",
                            f"conan-official-{self.name}-variables.cmake")

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
        if not self.options.enable_cxx:
            self.settings.rm_safe("compiler.cppstd")
            self.settings.rm_safe("compiler.libcxx")
        if (not self.options.enable_unsupported and (self.options.enable_cxx or self.options.hl))\
            or (self.settings.os == "Windows" and not self.options.shared):
            del self.options.threadsafe
        if not bool(self.options.szip_support):
            del self.options.szip_encoding

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        cmake_layout(self, src_folder="src")

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    def requirements(self):
        if self.options.with_zlib:
            self.requires("zlib/[>=1.2.11 <2]")
        if self.options.szip_support == "with_libaec":
            self.requires("libaec/1.0.6")
        elif self.options.szip_support == "with_szip":
            self.requires("szip/2.1.1")
        if self.options.parallel:
            self.requires("openmpi/4.1.0")

    # This callback is evoked to perform some validations (e.g. unsupported operating system) before building.
    # The detection of an invalid configuration must be signaled by raising an exception.
    def validate(self):
        if self.options.parallel and not self.options.enable_unsupported:
            if self.options.enable_cxx:
                raise ConanInvalidConfiguration("Parallel and C++ options are mutually exclusive, forcefully allow with enable_unsupported=True")
            if self.options.get_safe("threadsafe", False):
                raise ConanInvalidConfiguration("Parallel and Threadsafe options are mutually exclusive, forcefully allow with enable_unsupported=True")
        if self.options.szip_support == "with_szip" and \
                self.options.szip_encoding and \
                not self.dependencies["szip"].options.enable_encoding:
            raise ConanInvalidConfiguration("encoding must be enabled in szip dependency (szip:enable_encoding=True)")
        if self.settings.get_safe("compiler.cppstd"):
            check_min_cppstd(self, self._min_cppstd)

    # This callback is evoked to perform some validations whether build is possible with current configuration.
    # The detection of an inviable configuration must be signaled by raising an exception.
    def validate_build(self):
        if cross_building(self) and Version(self.version) < "1.14.4.3":
            # While building it runs some executables like H5detect
            raise ConanInvalidConfiguration("Current recipe doesn't support cross-building (yet)")

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        if Version(self.version) >= "1.14.0":
            self.tool_requires("cmake_installer/[>=3.18 <4]")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "hdf5-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "hdf5-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        cmakedeps = CMakeDeps(self)
        cmakedeps.generate()

        tc = CMakeToolchain(self)
        if not valid_min_cppstd(self, self._min_cppstd):
            tc.variables["CMAKE_CXX_STANDARD"] = self._min_cppstd
        if self.settings.get_safe("compiler.libcxx"):
            tc = self._inject_stdlib_flag(tc)
        if self.options.szip_support == "with_libaec":
            tc.variables["USE_LIBAEC"] = True
        tc.variables["HDF5_EXTERNALLY_CONFIGURED"] = True
        tc.variables["HDF5_EXTERNAL_LIB_PREFIX"] = ""
        tc.variables["HDF5_USE_FOLDERS"] = False
        tc.variables["HDF5_NO_PACKAGES"] = True
        tc.variables["ALLOW_UNSUPPORTED"] = False
        if Version(self.version) >= "1.10.6":
            tc.variables["ONLY_SHARED_LIBS"] = self.options.shared
        tc.variables["BUILD_STATIC_LIBS"] = not self.options.shared
        tc.variables["BUILD_STATIC_EXECS"] = False
        tc.variables["HDF5_ENABLE_COVERAGE"] = False
        tc.variables["HDF5_ENABLE_USING_MEMCHECKER"] = False
        if Version(self.version) >= "1.10.0":
            tc.variables["HDF5_MEMORY_ALLOC_SANITY_CHECK"] = False
        if Version(self.version) >= "1.10.5":
            tc.variables["HDF5_ENABLE_PREADWRITE"] = True
        tc.variables["HDF5_ENABLE_DEPRECATED_SYMBOLS"] = True
        tc.variables["HDF5_BUILD_GENERATORS"] = False
        tc.variables["HDF5_ENABLE_TRACE"] = False
        if self.settings.build_type == "Debug":
            tc.variables["HDF5_ENABLE_INSTRUMENT"] = False  # Option?
        tc.variables["HDF5_ENABLE_PARALLEL"] = self.options.parallel
        tc.variables["HDF5_ENABLE_Z_LIB_SUPPORT"] = self.options.with_zlib
        tc.variables["HDF5_ENABLE_SZIP_SUPPORT"] = bool(self.options.szip_support)
        tc.variables["HDF5_ENABLE_SZIP_ENCODING"] = self.options.get_safe("szip_encoding", False)
        tc.variables["HDF5_PACKAGE_EXTLIBS"] = False
        tc.variables["HDF5_ENABLE_THREADSAFE"] = self.options.get_safe("threadsafe", False)
        tc.variables["HDF5_ENABLE_DEBUG_APIS"] = False # Option?
        tc.variables["BUILD_TESTING"] = False

        tc.variables["HDF5_INSTALL_INCLUDE_DIR"] = "include/hdf5"

        tc.variables["HDF5_BUILD_TOOLS"] = False
        tc.variables["HDF5_BUILD_EXAMPLES"] = False
        tc.variables["HDF5_BUILD_HL_LIB"] = self.options.hl
        tc.variables["HDF5_BUILD_FORTRAN"] = False
        tc.variables["HDF5_BUILD_CPP_LIB"] = self.options.enable_cxx
        if Version(self.version) >= "1.10.0":
            tc.variables["HDF5_BUILD_JAVA"] = False
        tc.variables["ALLOW_UNSUPPORTED"] = self.options.enable_unsupported
        tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        apply_conandata_patches(self)
        # Do not force PIC
        replace_in_file(self, os.path.join(self.source_folder, "CMakeLists.txt"),
                "set (CMAKE_POSITION_INDEPENDENT_CODE ON)", "")
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))

        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rm(self, "libhdf5.settings", os.path.join(self.package_folder, "lib"))
        rm(self, "*.pdb", os.path.join(self.package_folder, "bin"))

        # remove extra libs... building 1.8.21 as shared also outputs static libs on Linux.
        if self.options.shared:
            for lib in glob.glob(os.path.join(self.package_folder, "lib", "*.a")):
                if not lib.endswith(".dll.a"):
                    os.remove(lib)

        # Mimic the official CMake FindHDF5 targets. HDF5::HDF5 refers to the global target as per conan,
        # but component targets have a lower case namespace prefix. hdf5::hdf5 refers to the C library only
        components = self._components()
        self._create_cmake_module_alias_targets(
            os.path.join(self.package_folder, self._module_targets_file_rel_path),
            {f"hdf5::{component['alias_target']}": f"HDF5::{component['component']}" for component in components.values()}
        )
        self._create_cmake_module_variables(
            os.path.join(self.package_folder, self._module_variables_file_rel_path),
            self.options.get_safe("parallel", False)
        )

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        def add_component(component_name, component, alias_target, requirements):
            def _config_libname(lib):
                if self.settings.os == "Windows" and self.settings.compiler != "gcc" and not self.options.shared:
                    lib = "lib" + lib
                if self.settings.build_type == "Debug":
                    debug_postfix = "_D" if self.settings.os == "Windows" else "_debug"
                    return lib + debug_postfix
                # See config/cmake_ext_mod/HDFMacros.cmake
                return lib

            self.cpp_info.components[component_name].set_property("cmake_target_name", f"hdf5::{alias_target}")
            self.cpp_info.components[component_name].set_property("pkg_config_name", alias_target)
            self.cpp_info.components[component_name].libs = [_config_libname(alias_target)]
            self.cpp_info.components[component_name].requires = requirements
            self.cpp_info.components[component_name].includedirs.append(os.path.join("include", "hdf5"))

            # TODO: to remove in conan v2 once cmake_find_package_* generators removed
            self.cpp_info.components[component_name].names["cmake_find_package"] = component
            self.cpp_info.components[component_name].names["cmake_find_package_multi"] = component
            self.cpp_info.components[component_name].build_modules["cmake_find_package"] = [self._module_targets_file_rel_path, self._module_variables_file_rel_path]
            self.cpp_info.components[component_name].build_modules["cmake_find_package_multi"] = [self._module_targets_file_rel_path, self._module_variables_file_rel_path]

        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "HDF5")
        self.cpp_info.set_property("cmake_target_name", "HDF5::HDF5")
        self.cpp_info.set_property("pkg_config_name", "hdf5-all-do-not-use") # to avoid conflict with hdf5_c component
        self.cpp_info.set_property("cmake_build_modules", [self._module_variables_file_rel_path])

        components = self._components()
        add_component("hdf5_c", **components["hdf5_c"])
        self.cpp_info.components["hdf5_c"].includedirs.append(os.path.join("include", "hdf5"))
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.components["hdf5_c"].system_libs.extend(["dl", "m"])
            if self.options.get_safe("threadsafe"):
                self.cpp_info.components["hdf5_c"].system_libs.append("pthread")
        elif self.settings.os == "Windows":
            self.cpp_info.components["hdf5_c"].system_libs.append("Shlwapi")

        if self.options.shared:
            self.cpp_info.components["hdf5_c"].defines.append("H5_BUILT_AS_DYNAMIC_LIB")
        if self.options.get_safe("enable_cxx"):
            add_component("hdf5_cpp", **components["hdf5_cpp"])
        if self.options.get_safe("hl"):
            add_component("hdf5_hl", **components["hdf5_hl"])
            if self.options.get_safe("enable_cxx"):
                add_component("hdf5_hl_cpp", **components["hdf5_hl_cpp"])

        # TODO: to remove in conan v2 once cmake_find_package_* generators removed
        self.cpp_info.names["cmake_find_package"] = "HDF5"
        self.cpp_info.names["cmake_find_package_multi"] = "HDF5"
