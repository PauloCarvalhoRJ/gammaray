from conan import ConanFile
from conan.tools.apple import fix_apple_shared_install_name
from conan.tools.build import cross_building, build_jobs
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.files import copy, get, rename, rm, rmdir, replace_in_file, save, chdir, mkdir, unzip
from conan.tools.gnu import Autotools, AutotoolsToolchain, AutotoolsDeps, PkgConfigDeps
from conan.tools.layout import basic_layout
from conan.tools.microsoft import is_msvc, msvc_runtime_flag, unix_path, NMakeDeps, NMakeToolchain
from conan.tools.scm import Version
import os

import itertools
import textwrap

required_conan_version = ">=1.55.0"

# This recipe builds, installs and tests libxml2 2.12.7 (a XML parser).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class Libxml2Conan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "libxml2"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2.12.7"
    # Hints Conan of what information should be propagated to consumer packages (e.g. include and library directories).
    package_type = "library"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # A brief description of what this recipe does.
    description = "libxml2 is a software library for parsing XML documents"
    # State some keywords describing what the dependency does.
    topics = "xml", "parser", "validation"
    # Declare the home page of the dependency's project.
    homepage = "https://gitlab.gnome.org/GNOME/libxml2/-/wikis/"
    # Declare the license of the dependency.
    license = "MIT"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # from ./configure and ./win32/configure.js
    default_options = {
        "shared": False,
        "fPIC": True,
        "include_utils": True,
        "c14n": True,
        "catalog": True,
        "docbook": True,    # dropped after 2.10.3
        "ftp": True,
        "http": True,
        "html": True,
        "iconv": True,
        "icu": False,
        "iso8859x": True,
        "legacy": True,
        "mem-debug": False,
        "output": True,
        "pattern": True,
        "push": True,
        "python": False,
        "reader": True,
        "regexps": True,
        "run-debug": False,
        "sax1": True,
        "schemas": True,
        "schematron": True,
        "threads": True,
        "tree": True,
        "valid": True,
        "writer": True,
        "xinclude": True,
        "xpath": True,
        "xptr": True,
        "zlib": True,
        "lzma": False,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "libxml2-{}.tar.xz".format(version)

    #---------------------------------------------Class/non-Conan attributes------------------------------------------------

    options = {name: [True, False] for name in default_options.keys()}

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _configure_option_names(self):
        return [name for name in self.default_options.keys() if (name in self.options)
                and (name not in ["shared", "fPIC", "include_utils"])]

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

    @property
    def _is_mingw_windows(self):
        return self.settings.compiler == "gcc" and self.settings.os == "Windows" and self._settings_build.os == "Windows"




    def _build_msvc(self):
        with chdir(self, os.path.join(self.source_folder, 'win32')):
            debug = "yes" if self.settings.build_type == "Debug" else "no"
            static = "no" if self.options.shared else "yes"

            args = [
                "cscript",
                "configure.js",
                "compiler=msvc",
                f"prefix={self.package_folder}",
                f"cruntime=/{msvc_runtime_flag(self)}",
                f"debug={debug}",
                f"static={static}",
            ]

            incdirs = [incdir for dep in self.dependencies.values() for incdir in dep.cpp_info.includedirs]
            libdirs = [libdir for dep in self.dependencies.values() for libdir in dep.cpp_info.libdirs]
            args.append(f"include=\"{';'.join(incdirs)}\"")
            args.append(f"lib=\"{';'.join(libdirs)}\"")

            for name in self._configure_option_names:
                cname = {"mem-debug": "mem_debug",
                         "run-debug": "run_debug",
                         "docbook": "docb"}.get(name, name)
                value = getattr(self.options, name)
                value = "yes" if value else "no"
                args.append(f"{cname}={value}")

            configure_command = ' '.join(args)
            self.output.info(configure_command)
            self.run(configure_command)

            # Fix library names because they can be not just zlib.lib
            def fix_library(option, package, old_libname):
                if option:
                    libs = []
                    aggregated_cpp_info = self.dependencies[package].cpp_info.aggregated_components()
                    for lib in itertools.chain(aggregated_cpp_info.libs, aggregated_cpp_info.system_libs):
                        libname = lib
                        if not libname.endswith('.lib'):
                            libname += '.lib'
                        libs.append(libname)
                    replace_in_file(self, "Makefile.msvc",
                                          f"LIBS = $(LIBS) {old_libname}",
                                          f"LIBS = $(LIBS) {' '.join(libs)}")

            fix_library(self.options.zlib, 'zlib', 'zlib.lib')
            fix_library(self.options.lzma, "xz_utils", "liblzma.lib")
            fix_library(self.options.iconv, 'libiconv', 'iconv.lib')
            fix_library(self.options.icu, 'icu', 'advapi32.lib sicuuc.lib sicuin.lib sicudt.lib')
            fix_library(self.options.icu, 'icu', 'icuuc.lib icuin.lib icudt.lib')

            self.run("nmake /f Makefile.msvc libxml libxmla libxmladll")

            if self.options.include_utils:
                self.run("nmake /f Makefile.msvc utils")

    def _package_msvc(self):
        with chdir(self, os.path.join(self.source_folder, 'win32')):
            self.run("nmake /f Makefile.msvc install-libs")

            if self.options.include_utils:
                self.run("nmake /f Makefile.msvc install-dist")

    def _build_mingw(self):
        with chdir(self, os.path.join(self.source_folder, "win32")):
            # configuration
            yes_no = lambda v: "yes" if v else "no"
            args = [
                "cscript",
                "configure.js",
                "compiler=mingw",
                f"prefix={self.package_folder}",
                f"debug={yes_no(self.settings.build_type == 'Debug')}",
                f"static={yes_no(not self.options.shared)}",
            ]

            incdirs = [incdir for dep in self.dependencies.values() for incdir in dep.cpp_info.includedirs]
            libdirs = [libdir for dep in self.dependencies.values() for libdir in dep.cpp_info.libdirs]
            args.append(f"include=\"{' -I'.join(incdirs)}\"")
            args.append(f"lib=\"{' -L'.join(libdirs)}\"")

            for name in self._configure_option_names:
                cname = {
                    "mem-debug": "mem_debug",
                    "run-debug": "run_debug",
                    "docbook": "docb",
                }.get(name, name)
                args.append(f"{cname}={yes_no(getattr(self.options, name))}")
            configure_command = " ".join(args)
            self.output.info(configure_command)
            self.run(configure_command)

            # build
            def fix_library(option, package, old_libname):
                if option:
                    aggregated_cpp_info = self.dependencies[package].cpp_info.aggregated_components()
                    replace_in_file(self,
                        "Makefile.mingw",
                        f"LIBS += -l{old_libname}",
                        f"LIBS += -l{' -l'.join(aggregated_cpp_info.libs)}",
                    )

            fix_library(self.options.iconv, "libiconv", "iconv")
            fix_library(self.options.zlib, "zlib", "z")
            fix_library(self.options.lzma, "xz_utils", "lzma")

            self.run(f"mingw32-make -j{build_jobs(self)} -f Makefile.mingw libxml libxmla")
            if self.options.include_utils:
                self.run(f"mingw32-make -j{build_jobs(self)} -f Makefile.mingw utils")

    def _package_mingw(self):
        with chdir(self, os.path.join(self.source_folder, "win32")):
            mkdir(self, os.path.join(self.package_folder, "include", "libxml2"))
            self.run("mingw32-make -f Makefile.mingw install-libs")
            if self.options.include_utils:
                self.run("mingw32-make -f Makefile.mingw install-dist")

    def _patch_sources(self):
        # Break dependency of install on build
        for makefile in ("Makefile.mingw", "Makefile.msvc"):
            replace_in_file(self, os.path.join(self.source_folder, "win32", makefile),
                                               "install-libs : all",
                                               "install-libs :")
    def _create_cmake_module_variables(self, module_file):
        # FIXME: also define LIBXML2_XMLLINT_EXECUTABLE variable
        content = textwrap.dedent(f"""\
            set(LibXml2_FOUND TRUE)
            set(LIBXML2_FOUND TRUE)
            if(DEFINED LibXml2_INCLUDE_DIRS)
                set(LIBXML2_INCLUDE_DIR ${{LibXml2_INCLUDE_DIRS}})
                set(LIBXML2_INCLUDE_DIRS ${{LibXml2_INCLUDE_DIRS}})
            elseif(DEFINED libxml2_INCLUDE_DIRS)
                set(LIBXML2_INCLUDE_DIR ${{libxml2_INCLUDE_DIRS}})
                set(LIBXML2_INCLUDE_DIRS ${{libxml2_INCLUDE_DIRS}})
            endif()
            if(DEFINED LibXml2_LIBRARIES)
                set(LIBXML2_LIBRARIES ${{LibXml2_LIBRARIES}})
                set(LIBXML2_LIBRARY ${{LibXml2_LIBRARIES}})
            elseif(DEFINED libxml2_LIBRARIES)
                set(LIBXML2_LIBRARIES ${{libxml2_LIBRARIES}})
                set(LIBXML2_LIBRARY ${{libxml2_LIBRARIES}})
            endif()
            if(DEFINED LibXml2_DEFINITIONS)
                set(LIBXML2_DEFINITIONS ${{LibXml2_DEFINITIONS}})
            elseif(DEFINED libxml2_DEFINITIONS)
                set(LIBXML2_DEFINITIONS ${{libxml2_DEFINITIONS}})
            else()
                set(LIBXML2_DEFINITIONS "")
            endif()
            set(LIBXML2_VERSION_STRING "{self.version}")
        """)
        save(self, module_file, content)

    @property
    def _module_file_rel_path(self):
        return os.path.join("lib", "cmake", f"conan-official-{self.name}-variables.cmake")

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Configure or constrain the available options in a package before assigning them a value. A typical use case is
    # to remove an option in a given platform. For example, the SSE2 flag doesn’t exist in architectures different 
    # than 32 bits, so it should be removed in this method.
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
        if Version(self.version) >= "2.10.3":
            del self.options.docbook
        if Version(self.version) >= "2.11.0":
            self.options.rm_safe("run-debug")

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
        basic_layout(self, src_folder="src")

    # Does the same as requirements (set dependencies) attribute, but in method form.
    def requirements(self):
        if self.options.zlib:
            self.requires("zlib/[>=1.2.11 <2]")
        if self.options.lzma:
            self.requires("xz_utils/5.4.5")
        if self.options.iconv:
            self.requires("libiconv/1.17", transitive_headers=True, transitive_libs=True)
        if self.options.icu:
            self.requires("icu/73.2")

    # Does the same as tool_requirements/build_requirements (requirements during build, e.g. CMake and Ninja) 
    # attribute, but in method form.
    def build_requirements(self):
        if not (is_msvc(self) or self._is_mingw_windows):
            if self.options.zlib or self.options.lzma or self.options.icu:
                if not self.conf.get("tools.gnu:pkg_config", check_type=str):
                    self.tool_requires("pkgconf/2.1.0")
            if self._settings_build.os == "Windows":
                self.win_bash = True
                if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                    self.tool_requires("msys2/cci.latest")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> conan_data.yml was removed (archive is locally present, no need to download it)
        unzip(self, filename=os.path.join(self.source_folder, "..", "libxml2-{}.tar.xz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "libxml2-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        if is_msvc(self):
            tc = NMakeToolchain(self)
            tc.generate()
            deps = NMakeDeps(self)
            deps.generate()
        elif self._is_mingw_windows:
            pass # nothing to do for mingw?  it calls mingw-make directly
        else:
            env = VirtualBuildEnv(self)
            env.generate()

            if not cross_building(self):
                env = VirtualRunEnv(self)
                env.generate(scope="build")

            tc = AutotoolsToolchain(self)

            yes_no = lambda v: "yes" if v else "no"
            tc.configure_args.extend([
                f"--enable-shared={yes_no(self.options.shared)}",
                f"--enable-static={yes_no(not self.options.shared)}",
            ])
            for option_name in self._configure_option_names:
                option_value = getattr(self.options, option_name)
                tc.configure_args.append(f"--with-{option_name}={yes_no(option_value)}")

            tc.generate()

            tc = PkgConfigDeps(self)
            tc.generate()

            tc = AutotoolsDeps(self)
            tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        self._patch_sources()
        if is_msvc(self):
            self._build_msvc()
        elif self._is_mingw_windows:
            self._build_mingw()
        else:
            autotools = Autotools(self)
            autotools.configure()
            autotools.make("libxml2.la")

            if self.options.include_utils:
                for target in ["xmllint", "xmlcatalog", "xml2-config"]:
                    autotools.make(target)

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"), ignore_case=True, keep_path=False)
        copy(self, "Copyright", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"), ignore_case=True, keep_path=False)
        if is_msvc(self):
            self._package_msvc()
            # remove redundant libraries to avoid confusion
            if not self.options.shared:
                rm(self, "libxml2.dll", os.path.join(self.package_folder, "bin"))
            rm(self, "libxml2_a_dll.lib", os.path.join(self.package_folder, "lib"))
            rm(self, "libxml2_a.lib" if self.options.shared else "libxml2.lib", os.path.join(self.package_folder, "lib"))
            rm(self, "*.pdb", os.path.join(self.package_folder, "bin"))
        elif self._is_mingw_windows:
            self._package_mingw()
            if self.options.shared:
                rm(self, "libxml2.a", os.path.join(self.package_folder, "lib"))
                rename(self, os.path.join(self.package_folder, "lib", "libxml2.lib"),
                             os.path.join(self.package_folder, "lib", "libxml2.dll.a"))
            else:
                rm(self, "libxml2.dll", os.path.join(self.package_folder, "bin"))
                rm(self, "libxml2.lib", os.path.join(self.package_folder, "lib"))
        else:
            autotools = Autotools(self)

            for target in ["install-libLTLIBRARIES", "install-data"]:
                autotools.make(target=target, args=[f"DESTDIR={unix_path(self, self.package_folder)}"])

            if self.options.include_utils:
                autotools.install()

            rm(self, "*.la", os.path.join(self.package_folder, "lib"))
            rm(self, "*.sh", os.path.join(self.package_folder, "lib"))
            rm(self, "run*", os.path.join(self.package_folder, "bin"))
            rm(self, "test*", os.path.join(self.package_folder, "bin"))
            rmdir(self, os.path.join(self.package_folder, "share"))
            rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
            rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
            fix_apple_shared_install_name(self)

        for header in ["win32config.h", "wsockcompat.h"]:
            copy(self, pattern=header, src=os.path.join(self.source_folder, "include"),
                      dst=os.path.join(self.package_folder, "include", "libxml2"), keep_path=False)

        self._create_cmake_module_variables(
            os.path.join(self.package_folder, self._module_file_rel_path)
        )

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        # FIXME: Provide LibXml2::xmllint & LibXml2::xmlcatalog imported target for executables
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_module_file_name", "LibXml2")
        self.cpp_info.set_property("cmake_file_name", "libxml2")
        self.cpp_info.set_property("cmake_target_name", "LibXml2::LibXml2")
        self.cpp_info.set_property("cmake_build_modules", [self._module_file_rel_path])
        self.cpp_info.set_property("pkg_config_name", "libxml-2.0")
        prefix = "lib" if is_msvc(self) else ""
        suffix = "_a" if is_msvc(self) and not self.options.shared else ""
        self.cpp_info.libs = [f"{prefix}xml2{suffix}"]
        self.cpp_info.includedirs.append(os.path.join("include", "libxml2"))
        if not self.options.shared:
            self.cpp_info.defines = ["LIBXML_STATIC"]
        if self.options.include_utils:
            bindir = os.path.join(self.package_folder, "bin")
            self.output.info(f"Appending PATH environment variable: {bindir}")
            self.env_info.PATH.append(bindir)
        if self.settings.os in ["Linux", "FreeBSD", "Android"]:
            self.cpp_info.system_libs.append("m")
            if self.options.threads and self.settings.os in ["Linux", "FreeBSD"]:
                self.cpp_info.system_libs.append("pthread")
            if self.settings.os in ["Linux", "FreeBSD"]:
                self.cpp_info.system_libs.append("dl")
        elif self.settings.os == "Windows":
            if self.options.ftp or self.options.http:
                self.cpp_info.system_libs.extend(["ws2_32", "wsock32"])

        # TODO: to remove in conan v2 once cmake_find_package* & pkg_config generators removed
        self.cpp_info.filenames["cmake_find_package"] = "LibXml2"
        self.cpp_info.filenames["cmake_find_package_multi"] = "libxml2"
        self.cpp_info.names["cmake_find_package"] = "LibXml2"
        self.cpp_info.names["cmake_find_package_multi"] = "LibXml2"
        self.cpp_info.build_modules["cmake_find_package"] = [self._module_file_rel_path]
        self.cpp_info.names["pkg_config"] = "libxml-2.0"
