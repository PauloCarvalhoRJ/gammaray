import os

from conan import ConanFile
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, get, replace_in_file, rmdir, unzip
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.layout import basic_layout
from conan.tools.scm import Version

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests GNU's automake (Autotools' makefile generator).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class AutomakeConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "automake"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.16.5"
    # A brief description of what the dependency does.
    description = (
        "Automake is a tool for automatically generating Makefile.in files"
        " compliant with the GNU Coding Standards."
    )
    # Declare the license of the dependency.
    license = ("GPL-2.0-or-later", "GPL-3.0-or-later")
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://www.gnu.org/software/automake/"
    # State some keywords describing what the dependency does.
    topics = ("autotools", "configure", "build")
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. bin dir).
    package_type = "application"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "automake-{}.tar.gz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

    def _patch_sources(self):
        apply_conandata_patches(self)
        if self.settings.os == "Windows":
            # tracing using m4 on Windows returns Windows paths => use cygpath to convert to unix paths
            ac_local_in = os.path.join(self.source_folder, "bin", "aclocal.in")
            replace_in_file(self, ac_local_in,
                                "          $map_traced_defs{$arg1} = $file;",
                                "          $file = `cygpath -u $file`;\n"
                                "          $file =~ s/^\\s+|\\s+$//g;\n"
                                "          $map_traced_defs{$arg1} = $file;")
            # handle relative paths during aclocal.m4 creation
            replace_in_file(self, ac_local_in, "$map{$m} eq $map_traced_defs{$m}",
                                "abs_path($map{$m}) eq abs_path($map_traced_defs{$m})")

    @property
    def _datarootdir(self):
        return os.path.join(self.package_folder, "res")

    @property
    def _automake_libdir(self):
        ver = Version(self.version)
        return os.path.join(self._datarootdir, f"automake-{ver.major}.{ver.minor}")

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Has the same effect as the export_sources attribute, but in method form. Hence, this callback is called when
    # exporting (copying) files from the source repository to the source directory in Conan's workplace.
    def export_sources(self):
        export_conandata_patches(self)

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        self.settings.rm_safe("compiler.cppstd")
        self.settings.rm_safe("compiler.libcxx")

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self, src_folder="src")

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    def requirements(self):
        self.requires("autoconf/2.71")
        # automake requires perl-Thread-Queue package

    # This is used to define which settings and options parcticipate in the hash calculation
    # for the package in Conan's cache.
    # In this case, we only want package name, version and OS to take part in the package's hash calculation.
    def package_id(self):
        del self.info.settings.arch
        del self.info.settings.compiler
        del self.info.settings.build_type

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        if hasattr(self, "settings_build"):
            self.tool_requires("autoconf/2.71")
        if self._settings_build.os == "Windows":
            self.win_bash = True
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "automake-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "automake-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        tc = AutotoolsToolchain(self)
        tc.configure_args.extend([
            "--datarootdir=${prefix}/res",
        ])
        tc.generate()

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
        autotools = Autotools(self)
        autotools.install()
        copy(self, "COPYING*", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))

        rmdir(self, os.path.join(self._datarootdir, "info"))
        rmdir(self, os.path.join(self._datarootdir, "man"))
        rmdir(self, os.path.join(self._datarootdir, "doc"))

        # TODO: consider whether the following is still necessary on Windows
        if self.settings.os == "Windows":
            binpath = os.path.join(self.package_folder, "bin")
            for filename in os.listdir(binpath):
                fullpath = os.path.join(binpath, filename)
                if not os.path.isfile(fullpath):
                    continue
                os.rename(fullpath, fullpath + ".exe")

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = []
        self.cpp_info.frameworkdirs = []
        self.cpp_info.resdirs = ["res"]

        # For consumers with new integrations (Conan 1 and 2 compatible):
        compile_wrapper = os.path.join(self._automake_libdir, "compile")
        lib_wrapper = os.path.join(self._automake_libdir, "ar-lib")
        self.conf_info.define("user.automake:compile-wrapper", compile_wrapper)
        self.conf_info.define("user.automake:lib-wrapper", lib_wrapper)

        # For legacy Conan 1.x consumers only:
        self.user_info.compile = compile_wrapper
        self.user_info.ar_lib = lib_wrapper
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
