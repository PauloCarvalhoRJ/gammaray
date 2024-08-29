from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import can_run
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.files import copy, get, replace_in_file, rmdir, unzip
from conan.tools.gnu import PkgConfigDeps
from conan.tools.layout import basic_layout
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.scm import Version
import os

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests Wayland 1.22.0 (a compsite/compositor library).
# Qt for Linux, for example, uses this to implement Composite pattern objects to represent
# GUI hierarchies of objects (e.g. QLabel inside a QFrame inside a QDialog).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class WaylandConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "wayland"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.22.0"
    # A brief description of what this dependency does.
    description = (
        "Wayland is a project to define a protocol for a compositor to talk to "
        "its clients as well as a library implementation of the protocol"
    )
    # State some keywords describing what the dependency does.
    topics = "protocol", "compositor", "display"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://wayland.freedesktop.org"
    # Declare the license of the dependency.
    license = "MIT"
    # Hints Conan of what information should be propagated to consumer packages (e.g. include and library directories).
    package_type = "library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # Declare additional configurations needed to define a build. 
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "enable_libraries": [True, False],
        "enable_dtd_validation": [True, False],
    }
    # Sets the default values to the additional configurations declared in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
        "enable_libraries": True,
        "enable_dtd_validation": True,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "wayland-{}.tar.xz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _patch_sources(self):
        replace_in_file(self, os.path.join(self.source_folder, "meson.build"),
                        "subdir('tests')", "#subdir('tests')")

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

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
        if self.options.enable_libraries:
            self.requires("libffi/3.4.4")
        if self.options.enable_dtd_validation:
            self.requires("libxml2/[>=2.12.5 <3]")
        self.requires("expat/[>=2.6.2 <3]")

    # Tests whether the current configuration is valid.
    def validate(self):
        if self.settings.os != "Linux":
            raise ConanInvalidConfiguration(f"{self.ref} only supports Linux")

    # Does the same as tool_requirements/build_requirements (requirements during build, e.g. CMake and Ninja) 
    # attribute, but in method form.
    def build_requirements(self):
        #self.tool_requires("meson/1.4.0") #---> of the original recipe
        self.tool_requires("meson/1.2.2") # we hope this version of Wayland works with Meson 1.2.2.
        # Ninja was not amongst the original recipe's dependencies, so the build fails if Ninja is not installed.
        # Therefore, we added its package as a build requirement instead of requiring the user to install it.
        self.tool_requires("ninja/1.12.1") 
        if not self.conf.get("tools.gnu:pkg_config", default=False, check_type=str):
            self.tool_requires("pkgconf/2.1.0")
        if not can_run(self):
            self.tool_requires(str(self.ref))

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> conan_data.yml was removed (archive is locally present, no need to download it)
        unzip(self, filename=os.path.join(self.source_folder, "..", "wayland-{}.tar.xz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "wayland-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()
        if can_run(self):
            env = VirtualRunEnv(self)
            env.generate(scope="build")

        pkg_config_deps = PkgConfigDeps(self)
        if not can_run(self):
            pkg_config_deps.build_context_activated = ["wayland"]
        elif self.dependencies["expat"].is_build_context:  # wayland is being built as build_require
            # If wayland is the build_require, all its dependencies are treated as build_requires
            pkg_config_deps.build_context_activated = [dep.ref.name for _, dep in self.dependencies.host.items()]
        pkg_config_deps.generate()
        tc = MesonToolchain(self)
        tc.project_options["libdir"] = "lib"
        tc.project_options["datadir"] = "res"
        tc.project_options["libraries"] = self.options.enable_libraries
        tc.project_options["dtd_validation"] = self.options.enable_dtd_validation
        tc.project_options["documentation"] = False
        if not can_run(self):
            tc.project_options["build.pkg_config_path"] = self.generators_folder
        if Version(self.version) >= "1.18.91":
            tc.project_options["scanner"] = True
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
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        meson = Meson(self)
        meson.install()
        pkg_config_dir = os.path.join(self.package_folder, "lib", "pkgconfig")
        rmdir(self, pkg_config_dir)

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.components["wayland-scanner"].set_property("pkg_config_name", "wayland-scanner")
        self.cpp_info.components["wayland-scanner"].resdirs = ["res"]
        self.cpp_info.components["wayland-scanner"].includedirs = []
        self.cpp_info.components["wayland-scanner"].libdirs = []
        self.cpp_info.components["wayland-scanner"].set_property("component_version", self.version)
        self.cpp_info.components["wayland-scanner"].requires = ["expat::expat"]
        if self.options.enable_dtd_validation:
            self.cpp_info.components["wayland-scanner"].requires.append("libxml2::libxml2")
        pkgconfig_variables = {
            'datarootdir': '${prefix}/res',
            'pkgdatadir': '${datarootdir}/wayland',
            'bindir': '${prefix}/bin',
            'wayland_scanner': '${bindir}/wayland-scanner',
        }
        self.cpp_info.components["wayland-scanner"].set_property(
            "pkg_config_custom_content",
            "\n".join(f"{key}={value}" for key,value in pkgconfig_variables.items()))

        if self.options.enable_libraries:
            self.cpp_info.components["wayland-server"].libs = ["wayland-server"]
            self.cpp_info.components["wayland-server"].set_property("pkg_config_name", "wayland-server")
            self.cpp_info.components["wayland-server"].requires = ["libffi::libffi"]
            self.cpp_info.components["wayland-server"].system_libs = ["pthread", "m"]
            self.cpp_info.components["wayland-server"].resdirs = ["res"]
            if self.version >= Version("1.21.0") and self.settings.os == "Linux":
                self.cpp_info.components["wayland-server"].system_libs += ["rt"]
            self.cpp_info.components["wayland-server"].set_property("component_version", self.version)
            pkgconfig_variables = {
                'datarootdir': '${prefix}/res',
                'pkgdatadir': '${datarootdir}/wayland',
            }
            self.cpp_info.components["wayland-server"].set_property(
                "pkg_config_custom_content",
                "\n".join(f"{key}={value}" for key, value in pkgconfig_variables.items()))

            self.cpp_info.components["wayland-client"].libs = ["wayland-client"]
            self.cpp_info.components["wayland-client"].set_property("pkg_config_name", "wayland-client")
            self.cpp_info.components["wayland-client"].requires = ["libffi::libffi"]
            self.cpp_info.components["wayland-client"].system_libs = ["pthread", "m"]
            self.cpp_info.components["wayland-client"].resdirs = ["res"]
            if self.version >= Version("1.21.0") and self.settings.os == "Linux":
                self.cpp_info.components["wayland-client"].system_libs += ["rt"]
            self.cpp_info.components["wayland-client"].set_property("component_version", self.version)
            pkgconfig_variables = {
                'datarootdir': '${prefix}/res',
                'pkgdatadir': '${datarootdir}/wayland',
            }
            self.cpp_info.components["wayland-client"].set_property(
                "pkg_config_custom_content",
                "\n".join(f"{key}={value}" for key, value in pkgconfig_variables.items()))

            self.cpp_info.components["wayland-cursor"].libs = ["wayland-cursor"]
            self.cpp_info.components["wayland-cursor"].set_property("pkg_config_name", "wayland-cursor")
            self.cpp_info.components["wayland-cursor"].requires = ["wayland-client"]
            self.cpp_info.components["wayland-cursor"].set_property("component_version", self.version)

            self.cpp_info.components["wayland-egl"].libs = ["wayland-egl"]
            self.cpp_info.components["wayland-egl"].set_property("pkg_config_name", "wayland-egl")
            self.cpp_info.components["wayland-egl"].requires = ["wayland-client"]
            self.cpp_info.components["wayland-egl"].set_property("component_version", "18.1.0")

            self.cpp_info.components["wayland-egl-backend"].set_property("pkg_config_name", "wayland-egl-backend")
            self.cpp_info.components["wayland-egl-backend"].set_property("component_version", "3")

            # TODO: to remove in conan v2
            self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
