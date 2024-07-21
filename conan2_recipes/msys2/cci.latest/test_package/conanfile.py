from conan import ConanFile
from conan.tools.env import Environment
from conan.tools.cmake import cmake_layout #cmake is not used in this recipe, but required for the code in the layout() callback to work.
from io import StringIO

# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.

class TestPackageConan(ConanFile):
    settings = "os", "arch", "build_type" # build_type is required by cmake_layout() to work, not actually required by MSYS2 which is obviously in release mode.
    generators = "VirtualBuildEnv"

    # Sets the requirements in method form (like the tool_requires attribute).
    # For test packages, of course it requires the parent package (the library under test).
    def build_requirements(self):
        self.tool_requires(self.tested_reference_str)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _secret_value(self):
        return "SECRET_CONAN_PKG_VARIABLE"

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

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
        env = Environment()
        env.define("PKG_CONFIG_PATH", self._secret_value)
        envvars = env.vars(self)
        envvars.save_script("conanbuildenv_pkg_config_path")

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        pass # nothing to do, skip hook warning

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        self.run('bash.exe -c ^"make --version^"')
        self.run('bash.exe -c ^"! test -f /bin/link^"')
        self.run('bash.exe -c ^"! test -f /usr/bin/link^"')

        output = StringIO()
        self.run('bash.exe -c "echo $PKG_CONFIG_PATH"', output)
        assert self._secret_value in output.getvalue()
