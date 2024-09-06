import os
import re
from io import StringIO

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMake, cmake_layout

# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.

class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeToolchain", "VirtualBuildEnv", "VirtualRunEnv", "CMakeDeps"
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _assert_expected_version(self):
        def tested_reference_version():
            tokens = re.split('[@#]', self.tested_reference_str)
            return tokens[0].split("/", 1)[1]
        output = StringIO()
        self.run("flex --version", output)
        output_str = str(output.getvalue()) 
        self.output.info("Installed version: {}".format(output_str))
        expected_version = tested_reference_version()
        self.output.info("Expected version: {}".format(expected_version))
        assert_flex_version = "flex {}".format(expected_version)
        assert(assert_flex_version in output_str)

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Sets the requirements in method form (like the tool_requires attribute).
    # For test packages, of course it requires the parent package (the library under test).
    def requirements(self):
        self.requires(self.tested_reference_str)

    # Does the same as tool_requirements/build_requirements (requirements during build, e.g. CMake and Ninja) 
    # attribute, but in method form.
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
        cmake_layout(self)

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        # Let's check flex version installed
        self._assert_expected_version()
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        if can_run(self):
            bin_path = os.path.join(self.cpp.build.bindirs[0], "test_package")
            txt_file = os.path.join(self.source_folder, "basic_nr.txt")
            self.run(f"{bin_path} {txt_file}", env="conanrun")
