from conan import ConanFile
from conan.tools.apple import fix_apple_shared_install_name
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import ( 
                                apply_conandata_patches, copy, export_conandata_patches, get, 
                                rename, replace_in_file, rm, rmdir, save, unzip
                              )
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.layout import basic_layout
from conan.tools.microsoft import is_msvc, is_msvc_static_runtime, MSBuild, MSBuildToolchain
from conan.tools.scm import Version
import os
import textwrap

required_conan_version = ">=1.54.0"

# This recipe builds, installs and tests the xz_utils library (replacement to compression/decompression LZMA Utils).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class XZUtilsConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "xz_utils"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "5.4.5"
    # A brief description of what the dependency does.
    description = (
        "XZ Utils is free general-purpose data compression software with a high "
        "compression ratio. XZ Utils were written for POSIX-like systems, but also "
        "work on some not-so-POSIX systems. XZ Utils are the successor to LZMA Utils."
    )
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://tukaani.org/xz"
    # State some keywords describing what the dependency does.
    topics = ("lzma", "xz", "compression")
    # Declare the license of the dependency.
    license = "Unlicense", "LGPL-2.1-or-later",  "GPL-2.0-or-later", "GPL-3.0-or-later"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "shared": False,
        "fPIC": True,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "xz_utils-{}.tar.xz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

    @property
    def _effective_msbuild_type(self):
        # treat "RelWithDebInfo" and "MinSizeRel" as "Release"
        # there is no DebugMT configuration in upstream vcxproj, we patch Debug configuration afterwards
        return "{}{}".format(
            "Debug" if self.settings.build_type == "Debug" else "Release",
            "MT" if is_msvc_static_runtime(self) and self.settings.build_type != "Debug" else "",
        )

    @property
    def _msbuild_target(self):
        return "liblzma_dll" if self.options.shared else "liblzma"

    @property
    def _msvc_sln_folder(self):
        if (str(self.settings.compiler) == "Visual Studio" and Version(self.settings.compiler) >= "15") or \
           (str(self.settings.compiler) == "msvc" and Version(self.settings.compiler) >= "191"):
            return "vs2017"
        return "vs2013"

    def _build_msvc(self):
        build_script_folder = os.path.join(self.source_folder, "windows", self._msvc_sln_folder)

        #==============================
        # TODO: to remove once https://github.com/conan-io/conan/pull/12817 available in conan client.
        vcxproj_files = [
            os.path.join(build_script_folder, "liblzma.vcxproj"),
            os.path.join(build_script_folder, "liblzma_dll.vcxproj"),
        ]
        if (str(self.settings.compiler) == "Visual Studio" and Version(self.settings.compiler) >= "15") or \
           (str(self.settings.compiler) == "msvc" and Version(self.settings.compiler) >= "191"):
            old_toolset = "v141"
        else:
            old_toolset = "v120"
        new_toolset = MSBuildToolchain(self).toolset
        conantoolchain_props = os.path.join(self.generators_folder, MSBuildToolchain.filename)
        for vcxproj_file in vcxproj_files:
            replace_in_file(
                self, vcxproj_file,
                f"<PlatformToolset>{old_toolset}</PlatformToolset>",
                f"<PlatformToolset>{new_toolset}</PlatformToolset>",
            )
            replace_in_file(
                self, vcxproj_file,
                "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />",
                f"<Import Project=\"{conantoolchain_props}\" /><Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />",
            )
        #==============================

        msbuild = MSBuild(self)
        msbuild.build_type = self._effective_msbuild_type
        msbuild.platform = "Win32" if self.settings.arch == "x86" else msbuild.platform
        msbuild.build(os.path.join(build_script_folder, "xz_win.sln"), targets=[self._msbuild_target])

    def _create_cmake_module_variables(self, module_file):
        # TODO: also add LIBLZMA_HAS_AUTO_DECODER, LIBLZMA_HAS_EASY_ENCODER & LIBLZMA_HAS_LZMA_PRESET
        content = textwrap.dedent(f"""\
            set(LIBLZMA_FOUND TRUE)
            if(DEFINED LibLZMA_INCLUDE_DIRS)
                set(LIBLZMA_INCLUDE_DIRS ${{LibLZMA_INCLUDE_DIRS}})
            endif()
            if(DEFINED LibLZMA_LIBRARIES)
                set(LIBLZMA_LIBRARIES ${{LibLZMA_LIBRARIES}})
            endif()
            set(LIBLZMA_VERSION_MAJOR {Version(self.version).major})
            set(LIBLZMA_VERSION_MINOR {Version(self.version).minor})
            set(LIBLZMA_VERSION_PATCH {Version(self.version).patch})
            set(LIBLZMA_VERSION_STRING "{self.version}")
        """)
        save(self, module_file, content)

    @property
    def _module_file_rel_path(self):
        return os.path.join("lib", "cmake", f"conan-official-{self.name}-variables.cmake")

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Has the same effect as the export_sources attribute, but in method form. Hence, this callback is called when
    # exporting (copying) files from the source repository to the source directory in Conan's workplace.
    def export_sources(self):
        export_conandata_patches(self)

    # Configure or constrain the available options in a package before assigning them a value. A typical use case is
    # to remove an option in a given platform. For example, the SSE2 flag doesn’t exist in architectures different 
    # than 32 bits, so it should be removed in this method.
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

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        if self._settings_build.os == "Windows" and not is_msvc(self):
            self.win_bash = True
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "xz_utils-{}.tar.xz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "xz_utils-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        if is_msvc(self):
            tc = MSBuildToolchain(self)
            tc.configuration = self._effective_msbuild_type
            tc.generate()
        else:
            env = VirtualBuildEnv(self)
            env.generate()
            tc = AutotoolsToolchain(self)
            tc.configure_args.append("--disable-doc")
            if self.settings.build_type == "Debug":
                tc.configure_args.append("--enable-debug")
            tc.generate()

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        apply_conandata_patches(self)
        if is_msvc(self):
            self._build_msvc()
        else:
            autotools = Autotools(self)
            autotools.configure()
            autotools.make()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        if is_msvc(self):
            inc_dir = os.path.join(self.source_folder, "src", "liblzma", "api")
            copy(self, "*.h", src=inc_dir, dst=os.path.join(self.package_folder, "include"))
            output_dir = os.path.join(self.source_folder, "windows")
            copy(self, "*.lib", src=output_dir, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
            copy(self, "*.dll", src=output_dir, dst=os.path.join(self.package_folder, "bin"), keep_path=False)
            rename(self, os.path.join(self.package_folder, "lib", "liblzma.lib"),
                         os.path.join(self.package_folder, "lib", "lzma.lib"))
        else:
            autotools = Autotools(self)
            autotools.install()
            rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
            rmdir(self, os.path.join(self.package_folder, "share"))
            rm(self, "*.la", os.path.join(self.package_folder, "lib"))
            fix_apple_shared_install_name(self)

        self._create_cmake_module_variables(
            os.path.join(self.package_folder, self._module_file_rel_path),
        )

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "LibLZMA")
        self.cpp_info.set_property("cmake_target_name", "LibLZMA::LibLZMA")
        self.cpp_info.set_property("cmake_build_modules", [self._module_file_rel_path])
        self.cpp_info.set_property("pkg_config_name", "liblzma")
        self.cpp_info.libs = ["lzma"]
        if not self.options.shared:
            self.cpp_info.defines.append("LZMA_API_STATIC")
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs.append("pthread")

        # TODO: to remove in conan v2 once cmake_find_package* & pkg_config generators removed
        self.cpp_info.names["cmake_find_package"] = "LibLZMA"
        self.cpp_info.names["cmake_find_package_multi"] = "LibLZMA"
        self.cpp_info.build_modules["cmake_find_package"] = [self._module_file_rel_path]
