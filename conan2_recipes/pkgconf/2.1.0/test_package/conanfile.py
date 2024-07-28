from io import StringIO
import os
from pathlib import Path
import re

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import cmake_layout, CMake, CMakeDeps, CMakeToolchain
from conan.tools.env import Environment, VirtualBuildEnv
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.microsoft import unix_path

# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.

# It will become the standard on Conan 2.x
class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

    @property
    def _testing_library(self):
        # Workaround, in Conan >=2.0 we should be able to remove this in favour of:
        # self.dependencies[self.tested_reference_str].options.enable_lib
        has_toolchain = sorted(Path(self.build_folder).rglob('conan_toolchain.cmake'))
        return has_toolchain

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Sets the build and runtime dependencies of this package.
    # This is the same as the requires attribute, but in method form.
    # Of course, this test requires the package under test.
    def requirements(self):
        self.requires(self.tested_reference_str, run=True)

    # This is necessary in addition to 
    #
    # [conf]
    # tools.cmake.cmake_layout:test_folder=C:\conan_tmp (or $TMP)
    #
    # in the Conan profile file to not clutter the test package's source repository
    # directory with build artifacts.
    def layout(self):
        cmake_layout(self, src_folder="src")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):

        # Expose `PKG_CONFIG_PATH` to be able to find libexample1.pc
        env = Environment()
        self.output.info(f"Source folder: {self.source_folder}")
        env.prepend_path("PKG_CONFIG_PATH", self.source_folder)
        env.vars(self, scope="run").save_script("pkgconf-config-path")

        # CMake project to test that we can link against the library,
        # when the library is built
        if self.dependencies[self.tested_reference_str].options.enable_lib:
            ct = CMakeToolchain(self)
            ct.generate()
            deps = CMakeDeps(self)
            deps.generate()

        # Check build environment postconditions
        buildenv = VirtualBuildEnv(self)
        env = buildenv.vars(scope='build')
        assert 'PKG_CONFIG' in env.keys()
        assert 'ACLOCAL_PATH' in env.keys()
        assert 'AUTOMAKE_CONAN_INCLUDES' in env.keys()
        buildenv.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        if self._testing_library:
            cmake = CMake(self)
            cmake.configure()
            cmake.build()

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        # Check that we can find pkgconf in build environment
        # and that it is the expected version
        if can_run(self):
            output = StringIO()
            self.run("pkgconf --about", output, env="conanrun")
            # TODO: When recipe is Conan 2+ only, this can be simplified
            # to: self.dependencies['pkgconf'].ref.version
            tokens = re.split('[@#]', self.tested_reference_str)
            pkgconf_expected_version = tokens[0].split("/", 1)[1]
            assert f"pkgconf {pkgconf_expected_version}" in output.getvalue() 

            self.run("pkgconf libexample1 -cflags", env="conanrun")
            
        # Test that executable linked against library runs as expected
        if can_run(self) and self._testing_library:
            test_executable = unix_path(self, os.path.join(self.cpp.build.bindirs[0], "test_package"))
            self.run(test_executable, env="conanrun")
