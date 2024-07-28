import os

from conan import ConanFile
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import ( apply_conandata_patches, copy, export_conandata_patches,
                                get, rename, rm, rmdir, replace_in_file, unzip )
from conan.tools.layout import basic_layout
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.microsoft import is_msvc, unix_path_package_info_legacy
from conan.tools.scm import Version


required_conan_version = ">=1.57.0"

# This recipe builds, installs and tests the PkgConf utils (a compiler and linker flags configurer for building packages).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class PkgConfConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "pkgconf"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2.1.0"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # State some keywords describing what the dependency does.
    topics = ("build", "configuration")
    # Declare the home page of the dependency's project.
    homepage = "https://git.sr.ht/~kaniini/pkgconf"
    # Declare the license of the dependency.
    license = "ISC"
    # A brief description of what the dependency does.
    description = "package compiler and linker metadata toolkit"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "enable_lib": [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "enable_lib": False,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "pkgconf-{}.tar.xz".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja is required by Mason build system to work.
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _patch_sources(self):
        apply_conandata_patches(self)

        if not self.options.get_safe("shared", False):
            replace_in_file(self, os.path.join(self.source_folder, "meson.build"),
                                  "'-DLIBPKGCONF_EXPORT'",
                                  "'-DPKGCONFIG_IS_STATIC'")
            replace_in_file(self, os.path.join(self.source_folder, "meson.build"),
            "project('pkgconf', 'c',",
            "project('pkgconf', 'c',\ndefault_options : ['c_std=gnu99'],")

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self, src_folder="src")

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
        if not self.options.enable_lib:
            self.options.rm_safe("fPIC")
            self.options.rm_safe("shared")
        elif self.options.shared:
            self.options.rm_safe("fPIC")

        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    # This is used to define which settings and options parcticipate in the hash calculation
    # for the package in Conan's cache.
    def package_id(self):
        if not self.info.options.enable_lib:
            del self.info.settings.compiler # compiler info does not take part in the package's id hash.

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        self.tool_requires("meson/1.2.2")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "pkgconf-{}.tar.xz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "pkgconf-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        tc = MesonToolchain(self)
        if Version(self.version) >= "1.9.4":
            tc.project_options["tests"] = "disabled"
        else:
            tc.project_options["tests"] = False

        if not self.options.enable_lib:
            tc.project_options["default_library"] = "static"
        tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        self._patch_sources()
        meson = Meson(self)
        meson.configure()
        meson.build()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder,"licenses"))

        meson = Meson(self)
        meson.install()

        if is_msvc(self):
            rm(self, "*.pdb", os.path.join(self.package_folder, "bin"))
            if self.options.enable_lib and not self.options.shared:
                rename(self, os.path.join(self.package_folder, "lib", "libpkgconf.a"),
                          os.path.join(self.package_folder, "lib", "pkgconf.lib"),)

        if not self.options.enable_lib:
            rmdir(self, os.path.join(self.package_folder, "lib"))
            rmdir(self, os.path.join(self.package_folder, "include"))

        rmdir(self, os.path.join(self.package_folder, "share", "man"))
        rename(self, os.path.join(self.package_folder, "share", "aclocal"),
                  os.path.join(self.package_folder, "bin", "aclocal"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        if self.options.enable_lib:
            self.cpp_info.set_property("pkg_config_name", "libpkgconf")
            if Version(self.version) >= "1.7.4":
                self.cpp_info.includedirs.append(os.path.join("include", "pkgconf"))
            self.cpp_info.libs = ["pkgconf"]
            if not self.options.shared:
                self.cpp_info.defines = ["PKGCONFIG_IS_STATIC"]
        else:
            self.cpp_info.includedirs = []
            self.cpp_info.libdirs = []

        bindir = os.path.join(self.package_folder, "bin")
        self.env_info.PATH.append(bindir)

        exesuffix = ".exe" if self.settings.os == "Windows" else ""
        pkg_config = os.path.join(bindir, "pkgconf" + exesuffix).replace("\\", "/")
        self.buildenv_info.define_path("PKG_CONFIG", pkg_config)

        pkgconf_aclocal = os.path.join(self.package_folder, "bin", "aclocal")
        self.buildenv_info.prepend_path("ACLOCAL_PATH", pkgconf_aclocal)
        # TODO: evaluate if `ACLOCAL_PATH` is enough and we can stop using `AUTOMAKE_CONAN_INCLUDES`
        self.buildenv_info.prepend_path("AUTOMAKE_CONAN_INCLUDES", pkgconf_aclocal)

        # TODO: remove in conanv2
        automake_extra_includes = unix_path_package_info_legacy(self, pkgconf_aclocal.replace("\\", "/"))
        self.env_info.PKG_CONFIG = pkg_config
        self.env_info.AUTOMAKE_CONAN_INCLUDES.append(automake_extra_includes)
