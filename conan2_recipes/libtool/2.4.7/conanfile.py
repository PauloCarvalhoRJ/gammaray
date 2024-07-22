from conan import ConanFile
from conan.errors import ConanException
from conan.tools.apple import is_apple_os, fix_apple_shared_install_name
from conan.tools.env import Environment, VirtualBuildEnv, VirtualRunEnv
from conan.tools.files import ( apply_conandata_patches, export_conandata_patches, 
                                copy, get, rename, replace_in_file, rmdir, unzip
                            )
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.layout import basic_layout
from conan.tools.microsoft import check_min_vs, is_msvc, unix_path_package_info_legacy

import os
import re
import shutil

required_conan_version = ">=1.60.0 <2 || >=2.0.5"

# This recipe builds, installs and tests GNU's libtool.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class LibtoolConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "libtool"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2.4.7"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. bin dir).
    # most common use is as "application", but library traits
    # are a superset of application so this should cover all cases
    package_type = "library"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://www.gnu.org/software/libtool/"
    # A brief description of what the dependency does.
    description = "GNU libtool is a generic library support script. "
    # State some keywords describing what the dependency does.
    topics = ("configure", "library", "shared", "static")
    # Declare the license of the dependency.
    license = ("GPL-2.0-or-later", "GPL-3.0-or-later")
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    # "exclude_files" "packages" "additional_packages" values are a comma separated list
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "libtool-{}.tar.gz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _has_dual_profiles(self):
        return hasattr(self, "settings_build")

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

    @property
    def _datarootdir(self):
        return os.path.join(self.package_folder, "res")

    def _patch_sources(self):
        apply_conandata_patches(self)
        config_guess =  self.dependencies.build["gnu-config"].conf_info.get("user.gnu-config:config_guess")
        config_sub = self.dependencies.build["gnu-config"].conf_info.get("user.gnu-config:config_sub")
        shutil.copy(config_sub, os.path.join(self.source_folder, "build-aux", "config.sub"))
        shutil.copy(config_guess, os.path.join(self.source_folder, "build-aux", "config.guess"))

    @property
    def _shared_ext(self):
        if self.settings.os == "Windows":
            return "dll"
        elif is_apple_os(self):
            return "dylib"
        else:
            return "so"

    @property
    def _static_ext(self):
        if is_msvc(self):
            return "lib"
        else:
            return "a"

    def _rm_binlib_files_containing(self, ext_inclusive, ext_exclusive=None):
        regex_in = re.compile(r".*\.({})($|\..*)".format(ext_inclusive))
        if ext_exclusive:
            regex_out = re.compile(r".*\.({})($|\..*)".format(ext_exclusive))
        else:
            regex_out = re.compile("^$")
        for directory in (
                os.path.join(self.package_folder, "bin"),
                os.path.join(self.package_folder, "lib"),
        ):
            for file in os.listdir(directory):
                if regex_in.match(file) and not regex_out.match(file):
                    os.unlink(os.path.join(directory, file))

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
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self, src_folder="src")

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    def requirements(self):
        self.requires("automake/1.16.5")
        #TODO: consider adding m4 as direct dependency, perhaps when we start using version ranges.
        # https://github.com/conan-io/conan-center-index/pull/16248#discussion_r1116332095
        #self.requires("m4/1.4.19")

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        if self._has_dual_profiles:
            self.tool_requires("automake/<host_version>")
            self.tool_requires("m4/1.4.19")               # Needed by configure
        self.tool_requires("gnu-config/cci.20210814")
        if self._settings_build.os == "Windows":
            self.win_bash = True
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "libtool-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "libtool-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        VirtualBuildEnv(self).generate()
        if not self._has_dual_profiles:
            VirtualRunEnv(self).generate(scope="build")

        if is_msvc(self):
            # __VSCMD_ARG_NO_LOGO: this test_package has too many invocations,
            #                      this avoids printing the logo everywhere
            # VSCMD_SKIP_SENDTELEMETRY: avoid the telemetry process holding onto the directory
            #                           unnecessarily
            env = Environment()
            env.define("__VSCMD_ARG_NO_LOGO", "1")
            env.define("VSCMD_SKIP_SENDTELEMETRY", "1")
            env.vars(self, scope="build").save_script("conanbuild_vcvars_options.bat")

        tc = AutotoolsToolchain(self)

        if is_msvc(self) and check_min_vs(self, "180", raise_invalid=False):
            tc.extra_cflags.append("-FS")

        tc.configure_args.extend([
            "--datarootdir=${prefix}/res",
            "--enable-shared",
            "--enable-static",
            "--enable-ltdl-install",
        ])

        env = tc.environment()
        if is_msvc(self):
            env.define("CC", "cl -nologo")
            env.define("CXX", "cl -nologo")

            # Disable Fortran detection to handle issue with VS 2022
            # See: https://savannah.gnu.org/patch/?9313#comment1
            # In the future this could be removed if a new version fixes this
            # upstream
            env.define("F77", "no")
            env.define("FC", "no")
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
        copy(self, "COPYING*", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        autotools = Autotools(self)
        autotools.install()
        fix_apple_shared_install_name(self)

        rmdir(self, os.path.join(self._datarootdir, "info"))
        rmdir(self, os.path.join(self._datarootdir, "man"))

        os.unlink(os.path.join(self.package_folder, "lib", "libltdl.la"))
        if self.options.shared:
            self._rm_binlib_files_containing(self._static_ext, self._shared_ext)
        else:
            self._rm_binlib_files_containing(self._shared_ext)

        files = (
            os.path.join(self.package_folder, "bin", "libtool"),
            os.path.join(self.package_folder, "bin", "libtoolize"),
        )
        replaces = {
            "GREP": "/usr/bin/env grep",
            "EGREP": "/usr/bin/env grep -E",
            "FGREP": "/usr/bin/env grep -F",
            "SED": "/usr/bin/env sed",
        }
        for file in files:
            contents = open(file).read()
            for key, repl in replaces.items():
                contents, nb1 = re.subn("^{}=\"[^\"]*\"".format(key), "{}=\"{}\"".format(key, repl), contents, flags=re.MULTILINE)
                contents, nb2 = re.subn("^: \\$\\{{{}=\"[^$\"]*\"\\}}".format(key), ": ${{{}=\"{}\"}}".format(key, repl), contents, flags=re.MULTILINE)
                if nb1 + nb2 == 0:
                    raise ConanException("Failed to find {} in {}".format(key, repl))
            open(file, "w").write(contents)

        binpath = os.path.join(self.package_folder, "bin")
        if self.settings.os == "Windows":
            rename(self, os.path.join(binpath, "libtoolize"),
                         os.path.join(binpath, "libtoolize.exe"))
            rename(self, os.path.join(binpath, "libtool"),
                         os.path.join(binpath, "libtool.exe"))

        if is_msvc(self) and self.options.shared:
            rename(self, os.path.join(self.package_folder, "lib", "ltdl.dll.lib"),
                         os.path.join(self.package_folder, "lib", "ltdl.lib"))

        # allow libtool to link static libs into shared for more platforms
        libtool_m4 = os.path.join(self._datarootdir, "aclocal", "libtool.m4")
        method_pass_all = "lt_cv_deplibs_check_method=pass_all"
        replace_in_file(self, libtool_m4,
                              "lt_cv_deplibs_check_method='file_magic ^x86 archive import|^x86 DLL'",
                              method_pass_all)
        replace_in_file(self, libtool_m4,
                              "lt_cv_deplibs_check_method='file_magic file format (pei*-i386(.*architecture: i386)?|pe-arm-wince|pe-x86-64)'",
                              method_pass_all)

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.libs = ["ltdl"]

        if self.options.shared:
            if self.settings.os == "Windows":
                self.cpp_info.defines = ["LIBLTDL_DLL_IMPORT"]
        else:
            if self.settings.os == "Linux":
                self.cpp_info.system_libs = ["dl"]

        # Define environment variables such that libtool m4 files are seen by Automake
        libtool_aclocal_dir = os.path.join(self._datarootdir, "aclocal")

        self.buildenv_info.append_path("ACLOCAL_PATH", libtool_aclocal_dir)
        self.buildenv_info.append_path("AUTOMAKE_CONAN_INCLUDES", libtool_aclocal_dir)
        self.runenv_info.append_path("ACLOCAL_PATH", libtool_aclocal_dir)
        self.runenv_info.append_path("AUTOMAKE_CONAN_INCLUDES", libtool_aclocal_dir)

        # For Conan 1.x downstream consumers, can be removed once recipe is Conan 1.x only:
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
        self.env_info.ACLOCAL_PATH.append(unix_path_package_info_legacy(self, libtool_aclocal_dir))
        self.env_info.AUTOMAKE_CONAN_INCLUDES.append(unix_path_package_info_legacy(self, libtool_aclocal_dir))
