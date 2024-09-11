from conan import ConanFile, conan_version
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import copy, get, replace_in_file, rmdir, unzip
from conan.tools.layout import basic_layout
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.scm import Version
from conan.errors import ConanInvalidConfiguration
import os

required_conan_version = ">=1.64.0 <2 || >=2.2.0"

# This recipe builds, installs and tests Wayland Protocols 1.33 (a protocol for Wayland compositor communication with clients).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class WaylandProtocolsConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "wayland-protocols"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.33"
    # A brief description of what this dependency does.
    description = "Wayland is a project to define a protocol for a compositor to talk to its clients as well as a library implementation of the protocol"
    # State some keywords describing what the dependency does.
    topics = ("wayland",)
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://gitlab.freedesktop.org/wayland/wayland-protocols"
    # Declare the license of the dependency.
    license = "MIT"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "wayland-protocols-{}.tar.xz".format(version)
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _patch_sources(self):
        if Version(self.version) <= "1.23":
            # fixed upstream in https://gitlab.freedesktop.org/wayland/wayland-protocols/-/merge_requests/113
            replace_in_file(self, os.path.join(self.source_folder, "meson.build"),
                            "dep_scanner = dependency('wayland-scanner', native: true)",
                            "#dep_scanner = dependency('wayland-scanner', native: true)")

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Determine which information, in addition to name, version and those in the settings attribute
    # take part in the hash calculation for the package in Conan cache.
    def package_id(self):
        self.info.clear()

    # Tests whether the current configuration is valid.
    def validate(self):
        if self.settings.os != "Linux":
            raise ConanInvalidConfiguration(f"{self.ref} only supports Linux")

    # Does the same as tool_requirements/build_requirements (requirements during build, e.g. CMake and Ninja) 
    # attribute, but in method form.
    def build_requirements(self):
        # self.tool_requires("meson/1.3.0") --> from the original Wayland Protocols 1.33 recipe
        self.tool_requires("meson/1.2.2") # --> let's hope this works with Meson 1.2.2

    # Customize the default Conan workspace before building (e.g. define the name for the source directory).
    def layout(self):
        basic_layout(self, src_folder="src")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], destination=self.source_folder, strip_root=True) --> archive is locally present, no need to download it
        unzip(self, filename=os.path.join(self.source_folder, "..", "wayland-protocols-{}.tar.xz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "wayland-protocols-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = MesonToolchain(self)
        # Using relative folder because of this https://github.com/conan-io/conan/pull/15706
        tc.project_options["datadir"] = "res"
        tc.project_options["tests"] = "false"
        tc.generate()
        virtual_build_env = VirtualBuildEnv(self)
        virtual_build_env.generate()

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
        copy(self, "COPYING", self.source_folder, os.path.join(self.package_folder, "licenses"))
        meson = Meson(self)
        meson.install()
        rmdir(self, os.path.join(self.package_folder, "res", "pkgconfig"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        pkgconfig_variables = {
            'datarootdir': '${prefix}/res',
            'pkgdatadir': '${datarootdir}/wayland-protocols',
        }
        # TODO: Remove when Conan 1.x not supported
        pkgconfig_variables = pkgconfig_variables if conan_version.major >= 2 \
            else "\n".join(f"{key}={value}" for key, value in pkgconfig_variables.items())
        self.cpp_info.set_property("pkg_config_custom_content", pkgconfig_variables)
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = []
        self.cpp_info.bindirs = []
