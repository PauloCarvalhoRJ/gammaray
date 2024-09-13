from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import (
    apply_conandata_patches, collect_libs, copy, export_conandata_patches, load,
    get, rename, replace_in_file, rmdir, save, unzip
)
from conan.tools.scm import Version
import os
import re
import textwrap

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests FreeType 2.13.2 (freeware to render fonts).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class FreetypeConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "freetype"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2.13.2"
    # A brief description of what this dependency does.
    description = "FreeType is a freely available software library to render fonts."
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://www.freetype.org"
    # Declare the license of the dependency.
    license = "FTL"
    # State some keywords describing what the dependency does.
    topics = ("freetype", "fonts")
    # Hints Conan of what information should be propagated to consumer packages (e.g. include and library directories).
    package_type = "library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # Declare additional configurations needed to define a build. 
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_png": [True, False],
        "with_zlib": [True, False],
        "with_bzip2": [True, False],
        "with_brotli": [True, False],
        "subpixel": [True, False],
    }
    # Sets the default values to the additional configurations declared in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_png": True,
        "with_zlib": True,
        "with_bzip2": True,
        "with_brotli": True,
        "subpixel": False,
    }
    # Declares this dependency's dependencies needed for building.  This
    # allows not polluting the system's PATH environment variable with tools only needed
    # to build software.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "freetype-{}.tar.xz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _patch_sources(self):
        apply_conandata_patches(self)
        # Do not accidentally enable dependencies we have disabled
        cmakelists = os.path.join(self.source_folder, "CMakeLists.txt")
        if_harfbuzz_found = "if ({})".format("HARFBUZZ_FOUND" if Version(self.version) < "2.11.0" else "HarfBuzz_FOUND")
        replace_in_file(self, cmakelists, "find_package(HarfBuzz ${HARFBUZZ_MIN_VERSION})", "")
        replace_in_file(self, cmakelists, if_harfbuzz_found, "if(0)")
        if not self.options.with_png:
            replace_in_file(self, cmakelists, "find_package(PNG)", "")
            replace_in_file(self, cmakelists, "if (PNG_FOUND)", "if(0)")
        if not self.options.with_zlib:
            replace_in_file(self, cmakelists, "find_package(ZLIB)", "")
            replace_in_file(self, cmakelists, "if (ZLIB_FOUND)", "if(0)")
        if not self.options.with_bzip2:
            replace_in_file(self, cmakelists, "find_package(BZip2)", "")
            replace_in_file(self, cmakelists, "if (BZIP2_FOUND)", "if(0)")
        # the custom FindBrotliDec of upstream is too fragile
        replace_in_file(self, cmakelists,
                              "find_package(BrotliDec REQUIRED)",
                              "find_package(Brotli REQUIRED)\n"
                              "set(BROTLIDEC_FOUND 1)\n"
                              "set(BROTLIDEC_LIBRARIES \"brotli::brotli\")")
        if not self.options.with_brotli:
            replace_in_file(self, cmakelists, "find_package(BrotliDec)", "")
            replace_in_file(self, cmakelists, "if (BROTLIDEC_FOUND)", "if(0)")

        config_h = os.path.join(self.source_folder, "include", "freetype", "config", "ftoption.h")
        if self.options.subpixel:
            replace_in_file(self, config_h, "/* #define FT_CONFIG_OPTION_SUBPIXEL_RENDERING */", "#define FT_CONFIG_OPTION_SUBPIXEL_RENDERING")

    def _make_freetype_config(self, version):
        freetype_config_in = os.path.join(self.source_folder, "builds", "unix", "freetype-config.in")
        if not os.path.isdir(os.path.join(self.package_folder, "bin")):
            os.makedirs(os.path.join(self.package_folder, "bin"))
        freetype_config = os.path.join(self.package_folder, "bin", "freetype-config")
        rename(self, freetype_config_in, freetype_config)
        libs = "-lfreetyped" if self.settings.build_type == "Debug" else "-lfreetype"
        staticlibs = f"-lm {libs}" if self.settings.os == "Linux" else libs
        replace_in_file(self, freetype_config, r"%PKG_CONFIG%", r"/bin/false")  # never use pkg-config
        replace_in_file(self, freetype_config, r"%prefix%", r"$conan_prefix")
        replace_in_file(self, freetype_config, r"%exec_prefix%", r"$conan_exec_prefix")
        replace_in_file(self, freetype_config, r"%includedir%", r"$conan_includedir")
        replace_in_file(self, freetype_config, r"%libdir%", r"$conan_libdir")
        replace_in_file(self, freetype_config, r"%ft_version%", r"$conan_ftversion")
        replace_in_file(self, freetype_config, r"%LIBSSTATIC_CONFIG%", r"$conan_staticlibs")
        replace_in_file(self, freetype_config, r"-lfreetype", libs)
        replace_in_file(self, freetype_config, r"export LC_ALL", textwrap.dedent("""\
            export LC_ALL
            BINDIR=$(dirname $0)
            conan_prefix=$(dirname $BINDIR)
            conan_exec_prefix=${{conan_prefix}}/bin
            conan_includedir=${{conan_prefix}}/include
            conan_libdir=${{conan_prefix}}/lib
            conan_ftversion={version}
            conan_staticlibs="{staticlibs}"
        """).format(version=version, staticlibs=staticlibs))

    def _extract_libtool_version(self):
        conf_raw = load(self, os.path.join(self.source_folder, "builds", "unix", "configure.raw"))
        return next(re.finditer(r"^version_info='([0-9:]+)'", conf_raw, flags=re.M)).group(1).replace(":", ".")

    @property
    def _libtool_version_txt(self):
        return os.path.join(self.package_folder, "res", "freetype-libtool-version.txt")

    def _create_cmake_module_variables(self, module_file):
        content = textwrap.dedent(f"""\
            set(FREETYPE_FOUND TRUE)
            if(DEFINED Freetype_INCLUDE_DIRS)
                set(FREETYPE_INCLUDE_DIRS ${{Freetype_INCLUDE_DIRS}})
            endif()
            if(DEFINED Freetype_LIBRARIES)
                set(FREETYPE_LIBRARIES ${{Freetype_LIBRARIES}})
            endif()
            set(FREETYPE_VERSION_STRING "{self.version}")
        """)
        save(self, module_file, content)

    def _create_cmake_module_alias_targets(self, module_file, targets):
        content = ""
        for alias, aliased in targets.items():
            content += textwrap.dedent("""\
                if(TARGET {aliased} AND NOT TARGET {alias})
                    add_library({alias} INTERFACE IMPORTED)
                    set_property(TARGET {alias} PROPERTY INTERFACE_LINK_LIBRARIES {aliased})
                endif()
            """.format(alias=alias, aliased=aliased))
        save(self, module_file, content)

    @property
    def _module_vars_rel_path(self):
        return os.path.join("lib", "cmake", f"conan-official-{self.name}-variables.cmake")

    @property
    def _module_target_rel_path(self):
        return os.path.join("lib", "cmake", f"conan-official-{self.name}-targets.cmake")

    @staticmethod
    def _chmod_plus_x(filename):
        if os.name == "posix" and (os.stat(filename).st_mode & 0o111) != 0o111:
            os.chmod(filename, os.stat(filename).st_mode | 0o111)

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # This is called when files need to be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.  This is the method form of the export_sources attribute.
    def export_sources(self):
        export_conandata_patches(self) # --> as of version 2.13.2, there are no patches

    # Does the same as the options attribute, but in method form.
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

    # Does the same as the requires/tool_requires, but in method form.
    def requirements(self):
        if self.options.with_png:
            self.requires("libpng/[>=1.6 <2]")
        if self.options.with_zlib:
            self.requires("zlib/[>=1.2.10 <2]")
        if self.options.with_bzip2:
            # self.requires("bzip2/1.0.8") --> from the original recipe
            self.requires("bzip2/1.0.6") # --> let's hope our existing version suffices
        if self.options.with_brotli:
            self.requires("brotli/1.1.0")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> archive is locally present, no need to download it
        unzip(self, filename=os.path.join(self.source_folder, "..", "freetype-{}.tar.xz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "freetype-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        if Version(self.version) >= "2.11.0":
            tc.variables["FT_REQUIRE_ZLIB"] = self.options.with_zlib
            tc.variables["FT_DISABLE_ZLIB"] = not self.options.with_zlib
            tc.variables["FT_REQUIRE_PNG"] = self.options.with_png
            tc.variables["FT_DISABLE_PNG"] = not self.options.with_png
            tc.variables["FT_REQUIRE_BZIP2"] = self.options.with_bzip2
            tc.variables["FT_DISABLE_BZIP2"] = not self.options.with_bzip2
            # TODO: Harfbuzz can be added as an option as soon as it is available.
            tc.variables["FT_REQUIRE_HARFBUZZ"] = False
            tc.variables["FT_DISABLE_HARFBUZZ"] = True
            tc.variables["FT_REQUIRE_BROTLI"] = self.options.with_brotli
            tc.variables["FT_DISABLE_BROTLI"] = not self.options.with_brotli
        else:
            tc.variables["FT_WITH_ZLIB"] = self.options.with_zlib
            tc.variables["FT_WITH_PNG"] = self.options.with_png
            tc.variables["FT_WITH_BZIP2"] = self.options.with_bzip2
            # TODO: Harfbuzz can be added as an option as soon as it is available.
            tc.variables["FT_WITH_HARFBUZZ"] = False
            tc.variables["FT_WITH_BROTLI"] = self.options.with_brotli
        # Generate a relocatable shared lib on Macos
        tc.cache_variables["CMAKE_POLICY_DEFAULT_CMP0042"] = "NEW"
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
        cmake = CMake(self)
        cmake.install()

        libtool_version = self._extract_libtool_version()
        save(self, self._libtool_version_txt, libtool_version)
        self._make_freetype_config(libtool_version)

        doc_folder = os.path.join(self.source_folder, "docs")
        license_folder = os.path.join(self.package_folder, "licenses")
        copy(self, "FTL.TXT", doc_folder, license_folder)
        copy(self, "GPLv2.TXT", doc_folder, license_folder)
        copy(self, "LICENSE.TXT", doc_folder, license_folder)

        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        self._create_cmake_module_variables(
            os.path.join(self.package_folder, self._module_vars_rel_path)
        )
        self._create_cmake_module_alias_targets(
            os.path.join(self.package_folder, self._module_target_rel_path),
            {"freetype": "Freetype::Freetype"}
        )

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_module_file_name", "Freetype")
        self.cpp_info.set_property("cmake_file_name", "freetype")
        self.cpp_info.set_property("cmake_target_name", "Freetype::Freetype")
        self.cpp_info.set_property("cmake_target_aliases", ["freetype"]) # other possible target name in upstream config file
        self.cpp_info.set_property("cmake_build_modules", [self._module_vars_rel_path])
        self.cpp_info.set_property("pkg_config_name", "freetype2")
        self.cpp_info.libs = collect_libs(self)
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs.append("m")
        self.cpp_info.includedirs.append(os.path.join("include", "freetype2"))

        libtool_version = load(self, self._libtool_version_txt).strip()
        self.conf_info.define("user.freetype:libtool_version", libtool_version)
        self.cpp_info.set_property("system_package_version", libtool_version)

        # TODO: to remove in conan v2 once cmake_find_package* & pkg_config generators removed
        self.cpp_info.set_property("component_version", libtool_version)
        self.cpp_info.filenames["cmake_find_package"] = "Freetype"
        self.cpp_info.filenames["cmake_find_package_multi"] = "freetype"
        self.cpp_info.names["cmake_find_package"] = "Freetype"
        self.cpp_info.names["cmake_find_package_multi"] = "Freetype"
        self.cpp_info.build_modules["cmake_find_package"] = [self._module_vars_rel_path]
        self.cpp_info.build_modules["cmake_find_package_multi"] = [self._module_target_rel_path]
        self.cpp_info.names["pkg_config"] = "freetype2"
        freetype_config = os.path.join(self.package_folder, "bin", "freetype-config")
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
        self.env_info.FT2_CONFIG = freetype_config
        self._chmod_plus_x(freetype_config)
        self.user_info.LIBTOOL_VERSION = libtool_version
