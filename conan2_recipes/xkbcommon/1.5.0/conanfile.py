import os
import textwrap

from conan import ConanFile
from conan.tools.apple import fix_apple_shared_install_name
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.files import copy, get, replace_in_file, rmdir, save, unzip
from conan.tools.gnu import PkgConfigDeps
from conan.tools.layout import basic_layout
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.scm import Version
from conan.errors import ConanInvalidConfiguration

required_conan_version = ">=1.60.0 <2 || >=2.0.5"

# This recipe builds, installs and tests xkbcommons 1.5.0 (keyboard keymaps for Unix/Linux software).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class XkbcommonConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "xkbcommon"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.5.0"
    # Hints Conan of what information should be propagated to consumer packages (e.g. include and library directories).
    package_type = "library"
    # A brief description of what this dependency does.
    description = "keymap handling library for toolkits and window systems"
    # State some keywords describing what the dependency does.
    topics = ("keyboard", "wayland", "x11", "xkb")
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://github.com/xkbcommon/libxkbcommon"
    # Declare the license of the dependency.
    license = "MIT"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # Declare additional configurations needed to define a build. 
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_x11": [True, False],
        "with_wayland": [True, False],
        "xkbregistry": [True, False],
    }
    # Sets the default values to the additional configurations declared in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_x11": True,
        "with_wayland": True,
        "xkbregistry": True,
    }
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1"
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "libxkbcommon-{}.tar.xz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _has_build_profile(self):
        return hasattr(self, "settings_build")

    @property
    def _has_xkbregistry_option(self):
        return Version(self.version) >= "1.0.0"

    def _patch_sources(self):
        if self.options.get_safe("with_wayland"):
            if self._has_build_profile:
                # Patch the build system to use the pkg-config files generated for the build context.
                meson_build_file = os.path.join(self.source_folder, "meson.build")
                replace_in_file(
                    self,
                    meson_build_file,
                    "wayland_scanner_dep = dependency('wayland-scanner', required: false, native: true)",
                    "wayland_scanner_dep = dependency('wayland-scanner_BUILD', required: false, native: true)",
                )

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Does the same as the options attribute, but in method form.
    def config_options(self):
        if not self._has_xkbregistry_option:
            del self.options.xkbregistry
        if self.settings.os != "Linux":
            del self.options.with_wayland

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
        basic_layout(self, src_folder="src")

    # Does the same as requirements (set dependencies) attribute, but in method form.
    def requirements(self):
        self.requires("xkeyboard-config/system")
        if self.options.with_x11:
            self.requires("xorg/system")
        if self.options.get_safe("xkbregistry"):
            self.requires("libxml2/[>=2.12.5 <3]")
        if self.options.get_safe("with_wayland"):
            self.requires("wayland/1.22.0")

    # Tests whether the current configuration is valid.
    def validate(self):
        if self.settings.os not in ["Linux", "FreeBSD"]:
            raise ConanInvalidConfiguration(f"{self.ref} is only compatible with Linux and FreeBSD")

    # Does the same as tool_requirements/build_requirements (requirements during build, e.g. CMake and Ninja) 
    # attribute, but in method form.
    def build_requirements(self):
        #self.tool_requires("meson/1.3.2") ---> from the original recipe
        self.tool_requires("meson/1.2.2") # let's hope this version works with meson 1.2.2
        self.tool_requires("bison/3.8.2")
        if not self.conf.get("tools.gnu:pkg_config", default=False, check_type=str):
            self.tool_requires("pkgconf/2.1.0")
        if self.options.get_safe("with_wayland"):
            if self._has_build_profile:
                self.tool_requires("wayland/<host_version>")
            self.tool_requires("wayland-protocols/1.33")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> archive is locally present, no need to download it
        unzip(self, filename=os.path.join(self.source_folder, "..", "libxkbcommon-{}.tar.xz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "libxkbcommon-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()
        if self.options.get_safe("with_wayland") and not self._has_build_profile:
            env = VirtualRunEnv(self)
            env.generate(scope="build")

        tc = MesonToolchain(self)
        if Version(self.version) >= "1.6":
            tc.project_options["enable-bash-completion"] = False
        tc.project_options["enable-docs"] = False
        tc.project_options["enable-wayland"] = self.options.get_safe("with_wayland", False)
        tc.project_options["enable-x11"] = self.options.with_x11
        if self._has_xkbregistry_option:
            tc.project_options["enable-xkbregistry"] = self.options.xkbregistry
        tc.project_options["build.pkg_config_path"] = self.generators_folder
        tc.generate()

        pkg_config_deps = PkgConfigDeps(self)
        if self.options.get_safe("with_wayland"):
            if self._has_build_profile:
                pkg_config_deps.build_context_activated = ["wayland", "wayland-protocols"]
                pkg_config_deps.build_context_suffix = {"wayland": "_BUILD"}
            else:
                # Manually generate pkgconfig file of wayland-protocols since
                # PkgConfigDeps.build_context_activated can't work with legacy 1 profile
                wp_prefix = self.dependencies.build["wayland-protocols"].package_folder
                wp_version = self.dependencies.build["wayland-protocols"].ref.version
                wp_pkg_content = textwrap.dedent(f"""\
                    prefix={wp_prefix}
                    datarootdir=${{prefix}}/res
                    pkgdatadir=${{datarootdir}}/wayland-protocols
                    Name: Wayland Protocols
                    Description: Wayland protocol files
                    Version: {wp_version}
                """)
                save(self, os.path.join(self.generators_folder, "wayland-protocols.pc"), wp_pkg_content)
        pkg_config_deps.generate()

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
        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))
        meson = Meson(self)
        meson.install()
        rmdir(self, os.path.join(self.package_folder, "share"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        fix_apple_shared_install_name(self)

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.components["libxkbcommon"].set_property("pkg_config_name", "xkbcommon")
        self.cpp_info.components["libxkbcommon"].libs = ["xkbcommon"]
        self.cpp_info.components["libxkbcommon"].requires = ["xkeyboard-config::xkeyboard-config"]
        self.cpp_info.components["libxkbcommon"].resdirs = ["res"]

        if self.options.with_x11:
            self.cpp_info.components["libxkbcommon-x11"].set_property("pkg_config_name", "xkbcommon-x11")
            self.cpp_info.components["libxkbcommon-x11"].libs = ["xkbcommon-x11"]
            self.cpp_info.components["libxkbcommon-x11"].requires = ["libxkbcommon", "xorg::xcb", "xorg::xcb-xkb"]
        if self.options.get_safe("xkbregistry"):
            self.cpp_info.components["libxkbregistry"].set_property("pkg_config_name", "xkbregistry")
            self.cpp_info.components["libxkbregistry"].libs = ["xkbregistry"]
            self.cpp_info.components["libxkbregistry"].requires = ["libxml2::libxml2"]
        if self.options.get_safe("with_wayland", False):
            self.cpp_info.components["xkbcli-interactive-wayland"].libs = []
            self.cpp_info.components["xkbcli-interactive-wayland"].includedirs = []
            self.cpp_info.components["xkbcli-interactive-wayland"].requires = ["wayland::wayland-client"]

        if Version(self.version) >= "1.0.0":
            self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))

        # unofficial, but required to avoid side effects (libxkbcommon component
        # "steals" the default global pkg_config name)
        self.cpp_info.set_property("pkg_config_name", "xkbcommon_all_do_not_use")
        self.cpp_info.names["pkg_config"] = "xkbcommon_all_do_not_use"
