from conan import ConanFile
from conan.errors import ConanException
from conan.tools.build import can_run
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import chdir

# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.

class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "VirtualBuildEnv", "VirtualRunEnv"
    test_type = "explicit"
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires = "ninja/1.12.1", "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _boost_option(self, name, default):
        try:
            return getattr(self.dependencies["boost"].options, name, default)
        except (AttributeError, ConanException):
            return default

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
    def requirements(self):
        self.requires(self.tested_reference_str)

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["HEADER_ONLY"] = self.dependencies["boost"].options.header_only
        if not self.dependencies["boost"].options.header_only:
            tc.cache_variables["Boost_USE_STATIC_LIBS"] = not self.dependencies["boost"].options.shared
        tc.cache_variables["WITH_PYTHON"] = False; # not self.dependencies["boost"].options.without_python        --> boost::python is not used by GammaRay
        #if not self.dependencies["boost"].options.without_python:                                                --> boost::python is not used by GammaRay
        #    pyversion = self.dependencies["boost"].options.python_version
        #    tc.cache_variables["PYTHON_VERSION_TO_SEARCH"] = pyversion
        #    tc.cache_variables["Python_EXECUTABLE"] = self.dependencies["boost"].options.python_executable
        tc.cache_variables["WITH_GEOMETRY"] = True                                                              #--> boost::geometry is required by GammaRay
        tc.cache_variables["WITH_NUMERIC"] = True                                                               #--> boost::numeric is required by GammaRay
        tc.cache_variables["WITH_RANDOM"] = False #not self.dependencies["boost"].options.without_random        --> boost::random is not used by GammaRay
        tc.cache_variables["WITH_REGEX"] = False #not self.dependencies["boost"].options.without_regex          --> boost::random is not used by GammaRay
        tc.cache_variables["WITH_TEST"] = False #not self.dependencies["boost"].options.without_test            --> boost::test is not used by GammaRay
        tc.cache_variables["WITH_COROUTINE"] = False #not self.dependencies["boost"].options.without_coroutine  --> boost::coroutine is not used by GammaRay
        tc.cache_variables["WITH_CHRONO"] = False #not self.dependencies["boost"].options.without_chrono        --> boost::chrono is not used by GammaRay
        tc.cache_variables["WITH_FIBER"] = False #not self.dependencies["boost"].options.without_fiber          --> boost::fiber is not used by GammaRay
        tc.cache_variables["WITH_LOCALE"] = False #not self.dependencies["boost"].options.without_locale        --> boost::locale is not used by GammaRay
        tc.cache_variables["WITH_NOWIDE"] = not self._boost_option("without_nowide", True)
        tc.cache_variables["WITH_JSON"] = not self._boost_option("without_json", True)
        tc.cache_variables["WITH_STACKTRACE"] = False #not self.dependencies["boost"].options.without_stacktrace  --> boost::stacktrace is not used by GammaRay
        tc.cache_variables["WITH_STACKTRACE_ADDR2LINE"] = self.dependencies["boost"].conf_info.get("user.boost:stacktrace_addr2line_available")
        tc.cache_variables["WITH_STACKTRACE_BACKTRACE"] = self._boost_option("with_stacktrace_backtrace", False)
        tc.cache_variables["WITH_URL"] = not self._boost_option("without_url", True)
        if self.dependencies["boost"].options.namespace != 'boost' and not self.dependencies["boost"].options.namespace_alias:
            tc.cache_variables['BOOST_NAMESPACE'] = self.dependencies["boost"].options.namespace
        tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        if not can_run(self):
            return
        with chdir(self, self.folders.build_folder):
            # When boost and its dependencies are built as shared libraries,
            # the test executables need to locate them. Typically the 
            # `conanrun` env should be enough, but this may cause problems on macOS
            # where the CMake installation has dependencies on Apple-provided
            # system libraries that are incompatible with Conan-provided ones. 
            # When `conanrun` is enabled, DYLD_LIBRARY_PATH will also apply
            # to ctest itself. Given that CMake already embeds RPATHs by default, 
            # we can bypass this by using the `conanbuild` environment on
            # non-Windows platforms, while still retaining the correct behaviour.
            #env = "conanrun" if self.settings.os == "Windows" else "conanbuild"
            #self.run(f"ctest --output-on-failure -C {self.settings.build_type}", env=env)
            if self.settings.os == "Windows":
                self.run(f"ctest --output-on-failure -C {self.settings.build_type}") #env="conanrun" causes CMake bins not being found for some reason on Windows...
            else:
                self.run(f"ctest --output-on-failure -C {self.settings.build_type}", env="conanbuild")
