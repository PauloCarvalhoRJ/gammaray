from conan import ConanFile
from conan.tools.gnu import PkgConfigDeps
from conan.tools.layout import basic_layout
from conan.tools.cmake import CMake, cmake_layout #--> not in the original recipe

# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.

class TestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    # generators = "VirtualBuildEnv", "VirtualRunEnv"
    generators = "CMakeToolchain", "VirtualBuildEnv", "VirtualRunEnv"
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    # Does the same as requirements (runetime requirements, e.g. CMake and Ninja) 
    # attribute, but in method form.
    def requirements(self):
        self.requires(self.tested_reference_str)

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

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        pkg_config_deps = PkgConfigDeps(self)
        pkg_config_deps.generate()

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        pkg_config = self.conf_info.get("tools.gnu:pkg_config", default="pkg-config")
        # self.run(f"{pkg_config} --validate xkeyboard-config")  --> in the original recipe: the --validate option does not exist.
        self.run(f"{pkg_config} --print-errors xkeyboard-config")
