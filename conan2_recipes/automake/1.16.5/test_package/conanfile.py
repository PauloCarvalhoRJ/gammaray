import os

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.env import Environment, VirtualBuildEnv
from conan.tools.files import chdir
from conan.tools.gnu import Autotools, AutotoolsToolchain
#from conan.tools.layout import basic_layout
from conan.tools.cmake import cmake_layout #cmake is not used in this recipe, but required for the code in the layout() callback to work.
from conan.tools.microsoft import unix_path


required_conan_version = ">=1.53.0"

# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.


class TestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    test_type = "explicit"
    win_bash = True

    #---------------------------------------------Class/non-Conan members--------------------------------------------

    _default_cc = {
        "gcc": "gcc",
        "clang": "clang",
        "Visual Studio": "cl -nologo",
        "msvc": "cl -nologo",
        "apple-clang": "clang",
    }

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _system_compiler(self, cxx=False):
        system_cc = self._default_cc.get(str(self.settings.compiler))
        if system_cc and cxx:
            if self.settings.compiler == "gcc":
                system_cc = "g++"
            elif "clang" in self.settings.compiler:
                system_cc = "clang++"
        return system_cc

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Sets the requirements in method form (like the tool_requires attribute).
    # For test packages, of course it requires the parent package (the library under test).
    def build_requirements(self):
        self.build_requires(self.tested_reference_str)
        if self._settings_build.os == "Windows" and not self.conf.get(
            "tools.microsoft.bash:path", check_type=str
        ):
            self.build_requires("msys2/cci.latest")

    # This is necessary in addition to 
    #
    # [conf]
    # tools.cmake.cmake_layout:test_folder=C:\conan_tmp (or $TMP)
    #
    # in the Conan profile file to not clutter the test package's source repository
    # directory with build artifacts.
    def layout(self):
        #basic_layout(self, src_folder="src")
        cmake_layout(self, src_folder="src")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.generate()

        env = VirtualBuildEnv(self)
        env.generate()

        env = Environment()

        compile_script = unix_path(self,
            self.dependencies.build["automake"].conf_info.get("user.automake:compile-wrapper"))

        # define CC and CXX such that if the user hasn't already defined it
        # via `tools.build:compiler_executables` or buildenv variables,
        # we tell autotools to guess the name matching the current setting
        # (otherwise it falls back to finding gcc first)
        cc = self._system_compiler()
        cxx = self._system_compiler(cxx=True)
        if cc and cxx:
            # Using shell parameter expansion
            env.define("CC", f"${{CC-{cc}}}")
            env.define("CXX", f"${{CXX-{cxx}}}")

        env.define("COMPILE", compile_script)
        env.define("ACLOCAL_PATH", unix_path(self, os.path.join(self.source_folder)))
        env.vars(self, scope="build").save_script("automake_build_test")

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        # Test compilation through compile wrapper script
        compiler = self._system_compiler()
        source_file = unix_path(self, os.path.join(self.source_folder, "test_package_1.c"))
        with chdir(self, self.build_folder):
            self.run(f"$COMPILE {compiler} {source_file} -o script_test", env="conanbuild")

        # Build test project
        autotools = Autotools(self)
        autotools.autoreconf(args=["--debug"])
        autotools.configure()
        autotools.make()

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        if can_run(self):
            with chdir(self, self.build_folder):
                self.run("./script_test")
                self.run("./test_package")
