from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, get, rename, replace_in_file, unzip
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.layout import basic_layout
from conan.tools.microsoft import is_msvc, unix_path
import os

required_conan_version = ">=1.54.0"

# This recipe builds, installs and tests Bison 3.8.2 (a language parser generator).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class BisonConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "bison"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "3.8.2"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://www.gnu.org/software/bison/"
    # A brief description of what this dependency does.
    description = "Bison is a general-purpose parser generator"
    # State some keywords describing what the dependency does.
    topics = ("bison", "parser")
    # Declare the license of the dependency.
    license = "GPL-3.0-or-later"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # Declare additional configurations needed to define a build. 
    options = {
        "fPIC": [True, False],
    }
    # Sets the default values to the additional configurations declared in the options attribute.
    default_options = {
        "fPIC": True,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "bison-{}.tar.gz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

    def _patch_sources(self):
        apply_conandata_patches(self)

        makefile = os.path.join(self.source_folder, "Makefile.in")
        yacc = os.path.join(self.source_folder, "src", "yacc.in")

        if self.settings.os == "Windows":
            # replace embedded unix paths by windows paths
            replace_in_file(self, makefile,
                                  "echo '#define BINDIR \"$(bindir)\"';",
                                  "echo '#define BINDIR \"$(shell cygpath -m \"$(bindir)\")\"';")
            replace_in_file(self, makefile,
                                  "echo '#define PKGDATADIR \"$(pkgdatadir)\"';",
                                  "echo '#define PKGDATADIR \"$(shell cygpath -m \"$(pkgdatadir)\")\"';")
            replace_in_file(self, makefile,
                                  "echo '#define DATADIR \"$(datadir)\"';",
                                  "echo '#define DATADIR \"$(shell cygpath -m \"$(datadir)\")\"';")
            replace_in_file(self, makefile,
                                  "echo '#define DATAROOTDIR \"$(datarootdir)\"';",
                                  "echo '#define DATAROOTDIR \"$(shell cygpath -m \"$(datarootdir)\")\"';")

        replace_in_file(self, makefile,
                              "dist_man_MANS = $(top_srcdir)/doc/bison.1",
                              "dist_man_MANS =")
        replace_in_file(self, yacc, "@prefix@", "$CONAN_BISON_ROOT")
        replace_in_file(self, yacc, "@bindir@", "$CONAN_BISON_ROOT/bin")

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Copy (exports in Conan jargon) necessary files in addition to source files (e.g. patches) to
    # the source directory in Conan build workspace directory.
    def export_sources(self):
        export_conandata_patches(self)

    # Adjust configuration options (e.g. enable/disable certain options depending on OS).
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        self.settings.rm_safe("compiler.cppstd")
        self.settings.rm_safe("compiler.libcxx")

    # Customize the build workspace layout (e.g. define which directories contain source files).
    def layout(self):
        basic_layout(self, src_folder="src")

    # Does the same as requirements (set dependencies) attribute, but in method form.
    def requirements(self):
        self.requires("m4/1.4.19")

    # Tests whether the current configuration is valid.
    def validate(self):
        if is_msvc(self) and self.version == "3.8.2":
            raise ConanInvalidConfiguration(
                f"{self.ref} is not yet ready for Visual Studio, use previous version "
                "or open a pull request on https://github.com/conan-io/conan-center-index/pulls"
            )

    # Does the same as tool_requirements/build_requirements (requirements during build, e.g. CMake and Ninja) 
    # attribute, but in method form.
    def build_requirements(self):
        if self._settings_build.os == "Windows":
            self.win_bash = True
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")
        if is_msvc(self):
            self.tool_requires("automake/1.16.5")
        if self.settings.os != "Windows":
            self.tool_requires("flex/2.6.4")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> conan_data.yml was removed (archive is locally present, no need to download it)
        unzip(self, filename=os.path.join(self.source_folder, "..", "bison-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "bison-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        tc = AutotoolsToolchain(self)
        tc.configure_args.extend([
            "--enable-relocatable",
            "--disable-nls",
            "--datarootdir=${prefix}/res",
        ])
        if self.settings.compiler == "apple-clang":
            tc.configure_args.append("gl_cv_compiler_check_decl_option=")
        if is_msvc(self):
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
            tc.extra_cflags.append("-FS")
        env = tc.environment()
        if is_msvc(self):
            automake_conf = self.dependencies.build["automake"].conf_info
            compile_wrapper = unix_path(self, automake_conf.get("user.automake:compile-wrapper", check_type=str))
            ar_wrapper = unix_path(self, automake_conf.get("user.automake:lib-wrapper", check_type=str))
            env.define("CC", f"{compile_wrapper} cl -nologo")
            env.define("CXX", f"{compile_wrapper} cl -nologo")
            env.define("LD", "link -nologo")
            env.define("AR", f"{ar_wrapper} lib")
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
        autotools.install()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        autotools = Autotools(self)
        autotools.install()
        if is_msvc(self):
            rename(self, os.path.join(self.package_folder, "lib", "liby.a"),
                         os.path.join(self.package_folder, "lib", "y.lib"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.includedirs = []
        self.cpp_info.libs = ["y"]
        self.cpp_info.resdirs = ["res"]

        bison_root = self.package_folder.replace("\\", "/")
        self.buildenv_info.define_path("CONAN_BISON_ROOT", bison_root)

        pkgdir = os.path.join(self.package_folder, "res", "bison")
        self.buildenv_info.define_path("BISON_PKGDATADIR", pkgdir)

        # yacc is a shell script, so requires a shell (such as bash)
        yacc = os.path.join(self.package_folder, "bin", "yacc").replace("\\", "/")
        self.conf_info.define("user.bison:yacc", yacc)

        # TODO: to remove in conan v2
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
        self.env_info.CONAN_BISON_ROOT = self.package_folder.replace("\\", "/")
        self.env_info.BISON_PKGDATADIR = pkgdir
        self.user_info.YACC = yacc
