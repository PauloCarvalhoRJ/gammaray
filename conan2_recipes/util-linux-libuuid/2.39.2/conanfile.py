from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import fix_apple_shared_install_name, is_apple_os, XCRun
from conan.tools.files import copy, get, rm, rmdir, unzip
from conan.tools.gnu import Autotools, AutotoolsToolchain, AutotoolsDeps
from conan.tools.layout import basic_layout
from conan.tools.scm import Version
import os

required_conan_version = ">=1.53.0"

# This recipe builds, installs and tests Universal Uinique ID (a Linux util) 2.39.2.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class UtilLinuxLibuuidConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "util-linux-libuuid"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2.39.2"
    # A brief description of what this dependency does.
    description = "Universally unique id library"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://github.com/util-linux/util-linux.git"
    # Declare the license of the dependency.
    license = "BSD-3-Clause"
    # State some keywords describing what the dependency does.
    topics = "id", "identifier", "unique", "uuid"
    # Hints Conan of what information should be propagated to consumer packages (e.g. include and library directories).
    package_type = "library"
    # IMPORTANT: 
    # Declares that this dependency implements the same API as the packages listed here.
    # Trying to build them in the same Conan dependency graph results in a Conan exception.
    provides = "libuuid"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # Declare additional configurations needed to define a build. 
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    # Sets the default values to the additional configurations declared in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "util-linux-{}.tar.xz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _has_sys_file_header(self):
        return self.settings.os in ["FreeBSD", "Linux", "Macos"]

    def _minimum_compiler_version(self, compiler, build_type):
        min_version = {
            "gcc": {
                "Release": "4",
                "Debug": "8",
            },
            "clang": {
                "Release": "3",
                "Debug": "3",
            },
            "apple-clang": {
                "Release": "5",
                "Debug": "5",
            },
        }
        return min_version.get(str(compiler), {}).get(str(build_type), "0")

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Does the same as the options attribute, but in method form.
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.settings.rm_safe("compiler.cppstd")
        self.settings.rm_safe("compiler.libcxx")

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self, src_folder="src")

    # Checks whether there are no inconsistencies in the configuration before building.
    def validate(self):
        min_version = self._minimum_compiler_version(self.settings.compiler, self.settings.build_type)
        if Version(self.settings.compiler.version) < min_version:
            raise ConanInvalidConfiguration(f"{self.settings.compiler} {self.settings.compiler.version} does not meet the minimum version requirement of version {min_version}")
        if self.settings.os == "Windows":
            raise ConanInvalidConfiguration(f"{self.ref} is not supported on Windows")

    # Does the same as the requires/tool_requires, but in method form.
    def requirements(self):
        if self.settings.os == "Macos":
            # Required because libintl.{a,dylib} is not distributed via libc on Macos
            self.requires("libgettext/0.22")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> archive is locally present, no need to download it
        unzip(self, filename=os.path.join(self.source_folder, "..", "util-linux-{}.tar.xz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "util-linux-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        tc = AutotoolsToolchain(self)
        tc.configure_args.append("--disable-all-programs")
        tc.configure_args.append("--enable-libuuid")
        if self._has_sys_file_header:
            tc.extra_defines.append("HAVE_SYS_FILE_H")
        if "x86" in self.settings.arch:
            tc.extra_cflags.append("-mstackrealign")

        # Based on https://github.com/conan-io/conan-center-index/blob/c647b1/recipes/libx264/all/conanfile.py#L94
        if is_apple_os(self) and self.settings.arch == "armv8":
            tc.configure_args.append("--host=aarch64-apple-darwin")
            tc.extra_asflags = ["-arch arm64"]
            tc.extra_ldflags = ["-arch arm64"]
            if self.settings.os != "Macos":
                xcrun = XCRun(self)
                platform_flags = ["-isysroot", xcrun.sdk_path]
                apple_min_version_flag = AutotoolsToolchain(self).apple_min_version_flag
                if apple_min_version_flag:
                    platform_flags.append(apple_min_version_flag)
                tc.extra_asflags.extend(platform_flags)
                tc.extra_cflags.extend(platform_flags)
                tc.extra_ldflags.extend(platform_flags)

        tc.generate()

        deps = AutotoolsDeps(self)
        deps.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        autotools = Autotools(self)
        autotools.configure()
        autotools.make()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING.BSD-3-Clause", src=os.path.join(self.source_folder, "Documentation", "licenses"), dst=os.path.join(self.package_folder, "licenses"))
        autotools = Autotools(self)
        autotools.install()
        rm(self, "*.la", os.path.join(self.package_folder, "lib"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "bin"))
        rmdir(self, os.path.join(self.package_folder, "sbin"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        rmdir(self, os.path.join(self.package_folder, "usr"))
        fix_apple_shared_install_name(self)

    # This is called when running recipes for packages which are dependent of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("pkg_config_name", "uuid")
        self.cpp_info.set_property("cmake_target_name", "libuuid::libuuid")
        self.cpp_info.set_property("cmake_file_name", "libuuid")
        # Maintain alias to `LibUUID::LibUUID` for previous version of the recipe
        self.cpp_info.set_property("cmake_target_aliases", ["LibUUID::LibUUID"])

        self.cpp_info.libs = ["uuid"]
        self.cpp_info.includedirs.append(os.path.join("include", "uuid"))
