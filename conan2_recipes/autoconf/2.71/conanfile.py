from conan import ConanFile
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import copy, get, rmdir, apply_conandata_patches, replace_in_file, export_conandata_patches, unzip
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.layout import basic_layout
from conan.tools.microsoft import unix_path, is_msvc
import os

required_conan_version = ">=1.54.0"

# This recipe builds, installs and tests GNU's autoconf (automatic configuration of software builds).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class AutoconfConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "autoconf"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2.71"
    # A brief description of what the dependency does.
    description = (
        "Autoconf is an extensible package of M4 macros that produce shell "
        "scripts to automatically configure software source code packages"
    )
    # Declare the license of the dependency.
    license = ("GPL-2.0-or-later", "GPL-3.0-or-later")
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://www.gnu.org/software/autoconf/"
    # State some keywords describing what the dependency does.
    topics = ("configure", "build")
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. bin dir).
    package_type = "application"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "autoconf-{}.tar.xz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _settings_build(self):
        # TODO: Remove for Conan v2
        return getattr(self, "settings_build", self.settings)

    def _patch_sources(self):
        apply_conandata_patches(self)
        replace_in_file(self, os.path.join(self.source_folder, "Makefile.in"),
                        "M4 = /usr/bin/env m4", "#M4 = /usr/bin/env m4")
        if self._settings_build.os == "Windows":
            # Handle vagaries of Windows line endings
            replace_in_file(self, os.path.join(self.source_folder, "bin", "autom4te.in"),
                            "$result =~ s/^\\n//mg;", "$result =~ s/^\\R//mg;")

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

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    def requirements(self):
        self.requires("m4/1.4.19") # Needed at runtime by downstream clients as well

    # This is used to define which settings and options parcticipate in the hash calculation
    # for the package in Conan's cache.
    # In this case, we only want package name and version to take part in the package's hash calculation.
    def package_id(self):
        self.info.clear()

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        self.tool_requires("m4/1.4.19")
        if self._settings_build.os == "Windows":
            self.win_bash = True
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "autoconf-{}.tar.xz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "autoconf-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        tc = AutotoolsToolchain(self)
        tc.configure_args.extend([
            "--datarootdir=${prefix}/res",
        ])

        if self.settings.os == "Windows":
            if is_msvc(self):
                build = "{}-{}-{}".format(
                    "x86_64" if self._settings_build.arch == "x86_64" else "i686",
                    "pc" if self._settings_build.arch == "x86" else "win64",
                    "mingw32")
                host = "{}-{}-{}".format(
                    "x86_64" if self.settings.arch == "x86_64" else "i686",
                    "pc" if self.settings.arch == "x86" else "win64",
                    "mingw32")
                tc.configure_args.append(f"--build={build}")
                tc.configure_args.append(f"--host={host}")

        env = tc.environment()
        env.define_path("INSTALL", unix_path(self, os.path.join(self.source_folder, "build-aux", "install-sh")))
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
        autotools = Autotools(self)
        autotools.install()

        copy(self, "COPYING*", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        rmdir(self, os.path.join(self.package_folder, "res", "info"))
        rmdir(self, os.path.join(self.package_folder, "res", "man"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.frameworkdirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = []
        self.cpp_info.resdirs = ["res"]

        bin_path = os.path.join(self.package_folder, "bin")
        self.buildenv_info.define_path("AUTOCONF", os.path.join(bin_path, "autoconf"))
        self.buildenv_info.define_path("AUTORECONF", os.path.join(bin_path, "autoreconf"))
        self.buildenv_info.define_path("AUTOHEADER", os.path.join(bin_path, "autoheader"))
        self.buildenv_info.define_path("AUTOM4TE", os.path.join(bin_path, "autom4te"))

        # TODO: to remove in conan v2
        self.env_info.PATH.append(bin_path)
        self.env_info.AUTOCONF = "autoconf"
        self.env_info.AUTORECONF = "autoreconf"
        self.env_info.AUTOHEADER = "autoheader"
        self.env_info.AUTOM4TE = "autom4te"
