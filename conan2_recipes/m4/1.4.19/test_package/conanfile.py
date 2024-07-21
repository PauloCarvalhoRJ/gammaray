from conan import ConanFile
from conan.tools.files import save
from conan.tools.cmake import cmake_layout #cmake is not used in this recipe, but required for the code in the layout() callback to work.
from io import StringIO
import os
import textwrap

# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.

class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "VirtualBuildEnv"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _m4_input_path(self):
        return os.path.join(self.build_folder, "input.m4")

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

    # Sets the requirements in method form (like the tool_requires attribute).
    # For test packages, of course it requires the parent package (the library under test).
    def build_requirements(self):
        self.tool_requires(self.tested_reference_str)

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        save(self, self._m4_input_path, textwrap.dedent("""\
            m4_define(NAME1, `Harry, Jr.')
            m4_define(NAME2, `Sally')
            m4_define(MET, `$1 met $2')
            MET(`NAME1', `NAME2')
        """))

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        extension = ".exe" if self.settings.os == "Windows" else ""
        self.run(f"m4{extension} --version")
        self.run(f"m4{extension} -P {self._m4_input_path}")

        self.run(f"m4{extension} -R {self.source_folder}/frozen.m4f {self.source_folder}/test.m4")

        output = StringIO()
        self.run(f"m4{extension} -P {self._m4_input_path}", output)
        assert "Harry, Jr. met Sally" in output.getvalue()
