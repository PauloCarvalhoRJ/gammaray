from conan import ConanFile
from conan.tools.build import cross_building
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, get, rmdir, save, unzip
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.layout import basic_layout
from conan.tools.microsoft import is_msvc, unix_path
from conan.tools.scm import Version
import os
import shutil

required_conan_version = ">=1.55.0"

# This recipe builds, installs and tests m4 (Unix macro processor).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class M4Conan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "m4"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.4.19"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. bin dir).
    package_type = "application"
    # A brief description of what the dependency does.
    description = "GNU M4 is an implementation of the traditional Unix macro processor"
    # State some keywords describing what the dependency does.
    topics = ("macro", "preprocessor")
    # Declare the home page of the dependency's project.
    homepage = "https://www.gnu.org/software/m4/"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the license of the dependency.
    license = "GPL-3.0-only"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "m4-{}.tar.xz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

    def _patch_sources(self):
        apply_conandata_patches(self)
        if shutil.which("help2man") == None:
            # dummy file for configure
            help2man = os.path.join(self.source_folder, "help2man")
            save(self, help2man, "#!/usr/bin/env bash\n:")
            if os.name == "posix":
                os.chmod(help2man, os.stat(help2man).st_mode | 0o111)

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Has the same effect as the export_sources attribute, but in method form. Hence, this callback is called when
    # exporting (copying) files from the source repository to the source directory in Conan's workplace.
    def export_sources(self):
        export_conandata_patches(self)

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self, src_folder="src")

    # This is used to define which settings and options parcticipate in the hash calculation
    # for the package in Conan's cache.
    # In this case, we don't want the hash to vary with the compiler.
    def package_id(self):
        del self.info.settings.compiler

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        if self._settings_build.os == "Windows":
            self.win_bash = True
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True)
        unzip(self, filename=os.path.join(self.source_folder, "..", "m4-{}.tar.xz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "m4-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        tc = AutotoolsToolchain(self)
        if is_msvc(self):
            tc.extra_cflags.append("-FS")
            # Avoid a `Assertion Failed Dialog Box` during configure with build_type=Debug
            # Visual Studio does not support the %n format flag:
            # https://docs.microsoft.com/en-us/cpp/c-runtime-library/format-specification-syntax-printf-and-wprintf-functions
            # Because the %n format is inherently insecure, it is disabled by default. If %n is encountered in a format string,
            # the invalid parameter handler is invoked, as described in Parameter Validation. To enable %n support, see _set_printf_count_output.
            tc.configure_args.extend([
                "gl_cv_func_printf_directive_n=no",
                "gl_cv_func_snprintf_directive_n=no",
                "gl_cv_func_snprintf_directive_n=no",
            ])
            if self.settings.build_type in ("Debug", "RelWithDebInfo"):
                tc.extra_ldflags.append("-PDB")
        elif self.settings.compiler == "clang" and Version(self.version) < "1.4.19":
            tc.extra_cflags.extend([
                "-rtlib=compiler-rt",
                "-Wno-unused-command-line-argument",
            ])
        if cross_building(self) and is_msvc(self):
            triplet_arch_windows = {"x86_64": "x86_64", "x86": "i686", "armv8": "aarch64"}
            
            host_arch = triplet_arch_windows.get(str(self.settings.arch))
            build_arch = triplet_arch_windows.get(str(self._settings_build.arch))

            if host_arch and build_arch:
                host = f"{host_arch}-w64-mingw32"
                build = f"{build_arch}-w64-mingw32"
                tc.configure_args.extend([
                    f"--host={host}",
                    f"--build={build}",
                ])
        if self.settings.os == "Windows":
            tc.configure_args.append("ac_cv_func__set_invalid_parameter_handler=yes")
        env = tc.environment()
        # help2man trick
        env.prepend_path("PATH", self.source_folder)
        # handle msvc
        if is_msvc(self):
            ar_wrapper = unix_path(self, os.path.join(self.source_folder, "build-aux", "ar-lib"))
            env.define("CC", "cl -nologo")
            env.define("CXX", "cl -nologo")
            env.define("AR", f"{ar_wrapper} lib")
            env.define("LD", "link")
            env.define("NM", "dumpbin -symbols")
            env.define("OBJDUMP", ":")
            env.define("RANLIB", ":")
            env.define("STRIP", ":")
        tc.generate(env)

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        self._patch_sources()
        autotools = Autotools(self)
        autotools.configure()
        autotools.make()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        autotools = Autotools(self)
        autotools.install()
        rmdir(self, os.path.join(self.package_folder, "share"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = []

        # M4 environment variable is used by a lot of scripts as a way to override a hard-coded embedded m4 path
        bin_ext = ".exe" if self.settings.os == "Windows" else ""
        m4_bin = os.path.join(self.package_folder, "bin", f"m4{bin_ext}").replace("\\", "/")
        self.runenv_info.define_path("M4", m4_bin)
        self.buildenv_info.define_path("M4", m4_bin)

        # TODO: to remove in conan v2
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
        self.env_info.M4 = m4_bin
