import os
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.build import cross_building

# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.

class TestZlibConan(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    #generators = "CMakeToolchain"  --> a CMakeToolchain object is already instantiated in generate()
    generators = "CMakeDeps"
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"
    requires = "zlib/1.2.11" # so we can query ZLib's (the parent package of this test package) 
                             # config options during generate() and configure()

    # Contains the package configuration steps.
    # E.g. run CMake to configure the project.
    def configure(self):
        if self.settings.compiler != "msvc":
            del self.settings.compiler.libcxx

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
        tc.variables["WITH_MINIZIP"] = self.dependencies[self.tested_reference_str].options.minizip
        tc.generate()
        self._zlib_rootpath = self.dependencies[self.tested_reference_str].package_folder
        self._with_minizip = self.dependencies[self.tested_reference_str].options.minizip

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        cmake = CMake(self)
        # Setting WITH_MINIZIP (or other likewise variables/definitions) here no longer is possible in Conan 2.
        #   see https://github.com/conan-io/conan/issues/16495
        # cmake.definitions["WITH_MINIZIP"] = self.dependencies[self.tested_reference_str].options.minizip
        # cmake.configure(build_script_folder=self.conf.tools.cmake.cmake_layout['test_folder'])
        cmake.configure()
        cmake.build()

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        assert os.path.exists(os.path.join(self._zlib_rootpath, "licenses", "LICENSE"))
        #assert os.path.exists(os.path.join(self.build_folder, "source_subfolder", "zlib.pc")) //this used to work in Conan 1
        if "x86" in self.settings.arch and not cross_building(self):
            #self.run(os.path.join("bin", "test"), run_environment=True)
            # Conan 2: the run_environment attribute no longer exists.  See https://github.com/conan-io/conan/issues/16528
            #           on how to set the environment so the tests can run (e.g. find the dynamic libraries of the package under test):
            #               "By instantiating the VirtualRunEnv/VirtualBuildEnv generators as needed, that information is already propagated
            #                to your scripts, no need to do it manually for most cases"
            #self.run(os.path.join(str(self.settings.build_type), "test")) --> works sans Ninja
            self.run( os.path.join(self.cpp.build.bindir, "test") )
            if self._with_minizip:
                self.run( os.path.join(self.cpp.build.bindir, "test_minizip") )

