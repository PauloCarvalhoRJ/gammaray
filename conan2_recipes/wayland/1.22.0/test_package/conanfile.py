import os

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMake, cmake_layout, CMakeDeps, CMakeToolchain
from conan.tools.env import VirtualBuildEnv
from conan.tools.gnu import PkgConfig, PkgConfigDeps

# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.

class TestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _has_build_profile(self):
        return hasattr(self, "settings_build")

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Sets the requirements in method form (like the tool_requires attribute).
    # For test packages, of course it requires the parent package (the library under test).
    def requirements(self):
        self.requires(self.tested_reference_str)

    # Does the same as tool_requirements/build_requirements (requirements during build, e.g. CMake and Ninja) 
    # attribute, but in method form.
    def build_requirements(self):
        self.tool_requires(self.tested_reference_str)
        if not self.conf.get("tools.gnu:pkg_config", default=False, check_type=str):
            self.tool_requires("pkgconf/2.1.0")

    # This is necessary in addition to 
    #
    # [conf]
    # tools.cmake.cmake_layout:test_folder=C:\conan_tmp (or $TMP)
    #
    # in the Conan profile file to not clutter the test package's source repository
    # directory with build artifacts.
    def layout(self):
        cmake_layout(self)

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        cmake_deps = CMakeDeps(self)
        cmake_deps.generate()
        pkg_config_deps = PkgConfigDeps(self)
        if self._has_build_profile:
            pkg_config_deps.build_context_activated = ["wayland"]
            pkg_config_deps.build_context_suffix = {"wayland": "_BUILD"}
        pkg_config_deps.generate()
        virtual_build_env = VirtualBuildEnv(self)
        virtual_build_env.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

        if self._has_build_profile:
            pkg_config = PkgConfig(self, "wayland-scanner_BUILD", self.generators_folder)
            wayland_scanner = pkg_config.variables["wayland_scanner"]
            self.run(f"{wayland_scanner} --version", env="conanrun")

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        if can_run(self):
            bin_path = os.path.join(self.cpp.build.bindirs[0], "test_package")
            self.run(bin_path, env="conanrun")
