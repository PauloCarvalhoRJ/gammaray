from conan import ConanFile
from conan.tools.apple import fix_apple_shared_install_name
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, get, rename, rm, rmdir, unzip
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.tools.layout import basic_layout
from conan.tools.microsoft import check_min_vs, is_msvc, unix_path
import os

required_conan_version = ">=1.57.0"

# This recipe builds, installs and tests the GSL library (GNU Scientific Library).

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class GslConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "gsl"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "2.7.1"
    # A brief description of what the dependency does.
    description = "GNU Scientific Library"
    # Declare the home page of the dependency's project.
    homepage = "https://www.gnu.org/software/gsl"
    # State some keywords describing what the dependency does.
    topics = ("numerical", "math", "random", "scientific")
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the license of the dependency.
    license = "GPL-3.0-or-later"
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
    exports_sources = "gsl-{}.tar.gz".format(version)

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

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

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self, src_folder="src")

    # Does the same as the build_requirements/tool_requirements but in method form.
    def build_requirements(self):
        self.tool_requires("libtool/2.4.7")
        if self._settings_build.os == "Windows":
            self.win_bash = True
            if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                self.tool_requires("msys2/cci.latest")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        # get(self, **self.conan_data["sources"][self.version], strip_root=True) --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "gsl-{}.tar.gz".format(self.version)), destination=".")
        copy(self, pattern="*", src=os.path.join(self.source_folder, "gsl-{}".format(self.version)), dst=".")

    # Contains the generation steps for the package.
    # E.g. runs CMake to generate a runnable Makefile or VS Solution.
    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        tc = AutotoolsToolchain(self)
        if self.settings.os == "Windows":
            tc.extra_defines.extend(["HAVE_WIN_IEEE_INTERFACE", "WIN32"])
            if self.options.shared:
                tc.extra_defines.append("GSL_DLL")
        if self.settings.os == "Linux" and "x86" in self.settings.arch:
            tc.extra_defines.append("HAVE_GNUX86_IEEE_INTERFACE")
        if is_msvc(self):
            tc.configure_args.extend([
                "ac_cv_func_memcpy=yes",
                "ac_cv_func_memmove=yes",
                "ac_cv_c_c99inline=no",
            ])
            if check_min_vs(self, "180", raise_invalid=False):
                tc.extra_cflags.append("-FS")
        env = tc.environment()
        if is_msvc(self):
            automake_conf = self.dependencies.build["automake"].conf_info
            compile_wrapper = unix_path(self, automake_conf.get("user.automake:compile-wrapper", check_type=str))
            ar_wrapper = unix_path(self, automake_conf.get("user.automake:lib-wrapper", check_type=str))
            env.define("CC", f"{compile_wrapper} cl -nologo")
            env.define("CXX", f"{compile_wrapper} cl -nologo")
            env.define("LD", "link -nologo")
            env.define("AR", f"{ar_wrapper} \"lib -nologo\"")
            env.define("NM", "dumpbin -symbols")
            env.define("OBJDUMP", ":")
            env.define("RANLIB", ":")
            env.define("STRIP", ":")
        tc.generate(env)

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        apply_conandata_patches(self)
        autotools = Autotools(self)
        autotools.autoreconf()
        autotools.configure()
        autotools.make()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        copy(self, "COPYING", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        autotools = Autotools(self)
        autotools.install()
        rmdir(self, os.path.join(self.package_folder, "share"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rm(self, "*.la", os.path.join(self.package_folder, "lib"))
        rm(self, "*.c", os.path.join(self.package_folder, "include", "gsl"))
        rm(self, "gsl-config", os.path.join(self.package_folder, "bin"))
        fix_apple_shared_install_name(self)
        if is_msvc(self) and self.options.shared:
            pjoin = lambda p: os.path.join(self.package_folder, "lib", p)
            rename(self, pjoin("gsl.dll.lib"), pjoin("gsl.lib"))
            rename(self, pjoin("gslcblas.dll.lib"), pjoin("gslcblas.lib"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "GSL")
        self.cpp_info.set_property("cmake_target_name", "GSL::gsl")
        self.cpp_info.set_property("pkg_config_name", "gsl")

        self.cpp_info.components["libgsl"].set_property("cmake_target_name", "GSL::gsl")
        self.cpp_info.components["libgsl"].libs = ["gsl"]
        self.cpp_info.components["libgsl"].requires = ["libgslcblas"]

        self.cpp_info.components["libgslcblas"].set_property("cmake_target_name", "GSL::gslcblas")
        self.cpp_info.components["libgslcblas"].libs = ["gslcblas"]

        if self.settings.os in ("FreeBSD", "Linux"):
            self.cpp_info.components["libgsl"].system_libs = ["m"]
            self.cpp_info.components["libgslcblas"].system_libs = ["m"]

        # TODO: to remove in conan v2 once cmake_find_package* generators removed
        self.cpp_info.names["cmake_find_package"] = "GSL"
        self.cpp_info.names["cmake_find_package_multi"] = "GSL"
        self.cpp_info.components["libgsl"].names["cmake_find_package"] = "gsl"
        self.cpp_info.components["libgsl"].names["cmake_find_package_multi"] = "gsl"
        self.cpp_info.components["libgslcblas"].names["cmake_find_package"] = "gslcblas"
        self.cpp_info.components["libgslcblas"].names["cmake_find_package_multi"] = "gslcblas"
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
