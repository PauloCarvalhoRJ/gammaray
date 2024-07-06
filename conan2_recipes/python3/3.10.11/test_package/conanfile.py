from conan import ConanFile
from conan.tools.cmake import cmake_layout

# This test package simply tests whether the Python 3.* package contains a usable Python interpreter.

class PythonTestConan(ConanFile):
    settings = "os", "compiler", "arch", "build_type" # Necessary to cmake_layout()
    requires = "python3/3.10.11"

    # This is necessary in addition to 
    #
    # [conf]
    # tools.cmake.cmake_layout:test_folder=C:\conan_tmp (or $TMP)
    #
    # in the Conan profile file to not clutter the test package's source repository
    # directory with build artifacts.
    def layout(self):
        cmake_layout(self, src_folder=".")

    def test(self):
        # TODO: test whether the returned version matched that in the "requires" attribute.
        self.run("python --version")
