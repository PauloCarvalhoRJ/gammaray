from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
import os

# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.

class Eigen3TestConan(ConanFile):
    # Although Eigen itself is a header-only library, the test executable is not.
    # So we need these four information to uniquely identify its binary compatibility.
    settings = "os", "compiler", "build_type", "arch"
    #generators = "CMakeToolchain"  --> a CMakeToolchain object is already instantiated in generate()
    generators = "CMakeDeps"
    build_requires = "cmake_installer/3.29.3" # we need cmake's binaries during build.
    requires = "eigen3/3.3.4" # so we can query eigen3's (the parent package of this test package) 
                              # config options during generate() and configure()

    # Contains the package configuration steps.
    # E.g. run CMake to configure the project.
    def configure(self):
        pass

    # This is necessary in addition to 
    #
    # [conf]
    # tools.cmake.cmake_layout:test_folder=C:\conan_tmp (or $TMP)
    #
    # in the Conan profile file to not clutter the test package's source repository
    # directory with build artifacts.
    def layout(self):
        cmake_layout(self, src_folder=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        #cmake = CMake(self, generator='Ninja')
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        bin_path = os.path.join(str(self.settings.build_type), "example")
        self.run(bin_path)
