import os

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.layout import basic_layout
from conan.tools.meson import Meson
from conan.tools.cmake import cmake_layout # --> this is needed so cmake_layout() in the layout() callback can function.


# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.

class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "MesonToolchain", "VirtualBuildEnv", "VirtualRunEnv"
    # Declares this dependeny's dependencies needed for building.
    # In this case, Ninja is required to run the test.
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        self.tool_requires(self.tested_reference_str)

    # This is necessary in addition to 
    #
    # [conf]
    # tools.cmake.cmake_layout:test_folder=C:\conan_tmp (or $TMP)
    #
    # in the Conan profile file to not clutter the test package's source repository
    # directory with build artifacts.
    def layout(self):
        # basic_layout(self) --> from the original recipe
        cmake_layout(self)

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        meson = Meson(self)
        meson.configure()
        meson.build()

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        self.run("meson --version")
        if can_run(self):
            bin_path = os.path.join(self.cpp.build.bindirs[0], "test_package")
            self.run(bin_path, env="conanrun")
