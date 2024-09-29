import os

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import fix_apple_shared_install_name
from conan.tools.build import cross_building
from conan.tools.files import get, rmdir, copy, rm, export_conandata_patches, apply_conandata_patches, unzip
from conan.tools.gnu import AutotoolsToolchain, Autotools
from conan.tools.layout import basic_layout

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests Flex 2.6.4 (lexical analyzer generator).
# NOTE: Flex is not buildable in Windows.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class FlexConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "flex"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2.6.4"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://github.com/westes/flex"
    # A brief description of what this dependency does.
    description = "Flex, the fast lexical analyzer generator"
    # State some keywords describing what the dependency does.
    topics = ("lex", "lexer", "lexical analyzer generator")
    # Declare the license of the dependency.
    license = "BSD-2-Clause"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # Declare additional configurations needed to define a build. 
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    # Sets the default values to the additional configurations declared in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "flex-{}.tar.gz".format(version)

    # Customize the default Conan workspace before building (e.g. define the name for the source directory).
    def layout(self):
        basic_layout(self)

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    # Does the same as requirements (set dependencies) attribute, but in method form.
    def requirements(self):
        # Flex requires M4 to be compiled. If consumer does not have M4
        # installed, Conan will need to know that Flex requires it.
        self.requires("m4/1.4.19")

    # Tests whether the current configuration is valid.
    def validate(self):
        if self.settings.os == "Windows":
            raise ConanInvalidConfiguration("Flex package is not compatible with Windows. "
                                            "Consider using winflexbison instead.")

    # Does the same as tool_requirements/build_requirements (requirements during build, e.g. CMake and Ninja) 
    # attribute, but in method form.
    def build_requirements(self):
        self.tool_requires("m4/1.4.19")
        if hasattr(self, "settings_build") and cross_building(self):
            self.tool_requires(f"{self.name}/{self.version}")

    # Copy (exports in Conan jargon) necessary files in addition to source files (e.g. patches) to
    # the source directory in Conan build workspace directory.
    def export_sources(self):
        export_conandata_patches(self)

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> conan_data.yml was removed (archive is locally present, no need to download it)
        unzip(self, filename=os.path.join(self.source_folder, "flex-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "flex-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        at = AutotoolsToolchain(self)
        at.configure_args.extend([
            "--disable-nls",
            "--disable-bootstrap",
            "HELP2MAN=/bin/true",
            "M4=m4",
            # https://github.com/westes/flex/issues/247
            "ac_cv_func_malloc_0_nonnull=yes", "ac_cv_func_realloc_0_nonnull=yes",
            # https://github.com/easybuilders/easybuild-easyconfigs/pull/5792
            "ac_cv_func_reallocarray=no",
        ])
        at.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        apply_conandata_patches(self)
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
        rm(self, "*.la", os.path.join(self.package_folder, "lib"))
        fix_apple_shared_install_name(self)

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.libs = ["fl"]
        self.cpp_info.system_libs = ["m"]
        # Avoid CMakeDeps messing with Conan targets
        self.cpp_info.set_property("cmake_find_mode", "none")

        bindir = os.path.join(self.package_folder, "bin")
        self.output.info("Appending PATH environment variable: {}".format(bindir))
        self.env_info.PATH.append(bindir)

        lex_path = os.path.join(bindir, "flex").replace("\\", "/")
        self.output.info("Setting LEX environment variable: {}".format(lex_path))
        self.env_info.LEX = lex_path
