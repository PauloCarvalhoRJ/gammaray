#!/usr/bin/env python
# -*- coding: utf-8 -*-
from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import cmake_layout, CMake
import os

# A test package is built and executed by the parent package automatically if it is contained in 
# a directory named test_package under the directory of the package under test.  It is not installed
# nor has a name, for example.

class QtIFWTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain", "VirtualRunEnv"
    # Declares the dependencies needed to run the test.
    requires = "qt/[>=5.15]"
    # Declares this dependeny's dependencies needed for building.
    # In this case, the Ninja and CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    # NOTE: this attribute used to be build_requires in Conan 1 and was kept in Conan 2 for backwards compatibility.
    tool_requires =  "qt/[>=5.15]", "ninja/1.12.1", "cmake_installer/3.29.3"
    
    # Sets the requirements in method form (like the tool_requires attribute).
    # For test packages, of course it requires the parent package (the library under test).
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
        cmake_layout(self)

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        self.output.info("Building QtIFW test with CMake in %s mode" % self.settings.build_type)
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    # If build() succeeds, the test steps here are executed.
    # Notice that this is not a test in the sense of software tests (e.g. unit test).
    # This only tests whether the package under test is usable.
    def test(self):
        self.output.info("Testing QtIFW with CMake.")
        bin_path = "QtIFWTest"
        self.output.info("*** Executing %s ... " % bin_path)
        self.run(bin_path)
