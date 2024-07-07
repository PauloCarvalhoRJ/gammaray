#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

from conan import ConanFile
from conan.tools.cmake import cmake_layout

# This test package simply tests whether JOM executable is runnable.

class TestPackageConan(ConanFile):
    settings = "build_type" # Necessary to cmake_layout()
    requires = "jom_installer/1.1.2"

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
        self.run("jom /VERSION")