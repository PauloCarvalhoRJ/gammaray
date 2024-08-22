from conan import ConanFile
from conan.tools.apple import fix_apple_shared_install_name
from conan.tools.build import cross_building
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import (
    apply_conandata_patches,
    copy,
    export_conandata_patches,
    get,
    rename,
    rm,
    rmdir,
    replace_in_file,
    unzip
)
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.layout import basic_layout
from conan.tools.microsoft import is_msvc, unix_path
from conan.tools.scm import Version
import os

required_conan_version = ">=1.54.0"

# This recipe builds, installs and tests libiconv 3.4.4 (Unicode codec).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class LibiconvConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "libiconv"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "1.17"
    # A brief description of what this recipe does.
    description = "Convert text to and from Unicode"
    # Declare the license of the dependency.
    license = ("LGPL-2.0-or-later", "LGPL-2.1-or-later")
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://www.gnu.org/software/libiconv/"
    # State some keywords describing what the dependency does.
    topics = ("iconv", "text", "encoding", "locale", "unicode", "conversion")
    # Hints Conan of what information should be propagated to consumer packages (e.g. include and library directories).
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
    exports_sources = "libiconv-{}.tar.gz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _is_clang_cl(self):
        return self.settings.compiler == "clang" and self.settings.os == "Windows" and \
               self.settings.compiler.get_safe("runtime")

    @property
    def _msvc_tools(self):
        return ("clang-cl", "llvm-lib", "lld-link") if self._is_clang_cl else ("cl", "lib", "link")

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

    def _apply_resource_patch(self):
        if self.settings.arch == "x86":
            windres_options_path = os.path.join(self.source_folder, "windows", "windres-options")
            self.output.info("Applying {} resource patch: {}".format(self.settings.arch, windres_options_path))
            replace_in_file(self, windres_options_path, '#   PACKAGE_VERSION_SUBMINOR', '#   PACKAGE_VERSION_SUBMINOR\necho "--target=pe-i386"', strict=True)

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
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")
        if Version(self.version) >= "1.17":
            self.license = "LGPL-2.1-or-later"
        else:
            self.license = "LGPL-2.0-or-later"

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self, src_folder="src")

    # Does the same as build_requirements/tool_requirements attributes, but in method form.
    def build_requirements(self):
        if self._settings_build.os == "Windows":
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")
            self.win_bash = True

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> conan_data.yml was removed (archive is locally present, no need to download it)
        unzip(self, filename=os.path.join(self.source_folder, "..", "libiconv-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "libiconv-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        tc = AutotoolsToolchain(self)
        if self.settings.os == "Windows" and self.settings.compiler == "gcc":
            if self.settings.arch == "x86":
                tc.update_configure_args({
                    "--host": "i686-w64-mingw32",
                    "RC": "windres --target=pe-i386",
                    "WINDRES": "windres --target=pe-i386",
                })
            elif self.settings.arch == "x86_64":
                tc.update_configure_args({
                    "--host": "x86_64-w64-mingw32",
                    "RC": "windres --target=pe-x86-64",
                    "WINDRES": "windres --target=pe-x86-64",
                })
        msvc_version = {"Visual Studio": "12", "msvc": "180"}
        if is_msvc(self) and Version(self.settings.compiler.version) >= msvc_version[str(self.settings.compiler)]:
            # https://github.com/conan-io/conan/issues/6514
            tc.extra_cflags.append("-FS")
        if cross_building(self) and is_msvc(self):
            triplet_arch_windows = {"x86_64": "x86_64", "x86": "i686", "armv8": "aarch64"}
            # ICU doesn't like GNU triplet of conan for msvc (see https://github.com/conan-io/conan/issues/12546)
            host_arch = triplet_arch_windows.get(str(self.settings.arch))
            build_arch = triplet_arch_windows.get(str(self._settings_build.arch))

            if host_arch and build_arch:
                host = f"{host_arch}-w64-mingw32"
                build = f"{build_arch}-w64-mingw32"
                tc.configure_args.extend([
                    f"--host={host}",
                    f"--build={build}",
                ])
        env = tc.environment()
        if is_msvc(self) or self._is_clang_cl:
            cc, lib, link = self._msvc_tools
            build_aux_path = os.path.join(self.source_folder, "build-aux")
            lt_compile = unix_path(self, os.path.join(build_aux_path, "compile"))
            lt_ar = unix_path(self, os.path.join(build_aux_path, "ar-lib"))
            env.define("CC", f"{lt_compile} {cc} -nologo")
            env.define("CXX", f"{lt_compile} {cc} -nologo")
            env.define("LD", link)
            env.define("STRIP", ":")
            env.define("AR", f"{lt_ar} {lib}")
            env.define("RANLIB", ":")
            env.define("NM", "dumpbin -symbols")
            env.define("win32_target", "_WIN32_WINNT_VISTA")
        tc.generate(env)

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self): 
        apply_conandata_patches(self)
        self._apply_resource_patch()
        autotools = Autotools(self)
        autotools.configure()
        autotools.make()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING.LIB", self.source_folder, os.path.join(self.package_folder, "licenses"))
        autotools = Autotools(self)
        autotools.install()
        rm(self, "*.la", os.path.join(self.package_folder, "lib"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        fix_apple_shared_install_name(self)
        if (is_msvc(self) or self._is_clang_cl) and self.options.shared:
            for import_lib in ["iconv", "charset"]:
                rename(self, os.path.join(self.package_folder, "lib", f"{import_lib}.dll.lib"),
                             os.path.join(self.package_folder, "lib", f"{import_lib}.lib"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "Iconv")
        self.cpp_info.set_property("cmake_target_name", "Iconv::Iconv")
        self.cpp_info.libs = ["iconv", "charset"]

        # TODO: to remove in conan v2
        self.cpp_info.names["cmake_find_package"] = "Iconv"
        self.cpp_info.names["cmake_find_package_multi"] = "Iconv"
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
