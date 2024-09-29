from conan import ConanFile
from conan.tools.system import package_manager
from conan.tools.gnu import PkgConfig
from conan.tools.layout import basic_layout

required_conan_version = ">=1.50.0"

# This recipe sets up the system so programs that depend on OpenGL can be compiled.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class SysConfigOpenGLConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "opengl"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    # As this is not actually a recipe to build a dependency, its "version" is named "system" (it could be anything else).
    version = "system"
    # A brief description of what this recipe does.
    description = "cross-platform virtual conan package for the OpenGL support"
    # State some keywords describing what the dependency does.
    topics = ("opengl", "gl")
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "https://www.opengl.org/"
    # Declare the license of the dependency.
    license = "MIT"
    # Declare what kind of artifact is being produced.
    package_type = "shared-library"
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    settings = "os", "arch", "compiler", "build_type"

    # Allows to customize some standard attributes (e.g. self.folders).
    # We could, for example, inform Conan that the source directory is 
    # the "src" subdirectory in the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self)

    # Allows to customize the package's default binary identification (ConanFile.package_id).
    # This is often used to relax some settings related to binary compatibility. For example, 
    # header-only libraries can have a single id since they don't have binaries.
    def package_id(self):
        self.info.clear()

    # Installs devel/SDK packages in the system if needed.
    def system_requirements(self):
        dnf = package_manager.Dnf(self)
        dnf.install_substitutes(["libglvnd-devel"], ["mesa-libGL-devel"], update=True, check=True)

        yum = package_manager.Yum(self)
        yum.install(["mesa-libGL-devel"], update=True, check=True)

        apt = package_manager.Apt(self)
        apt.install_substitutes(["libgl-dev"], ["libgl1-mesa-dev"], update=True, check=True)

        pacman = package_manager.PacMan(self)
        pacman.install(["libglvnd"], update=True, check=True)

        zypper = package_manager.Zypper(self)
        zypper.install_substitutes(["Mesa-libGL-devel", "glproto-devel"], 
                                   ["Mesa-libGL-devel", "xorgproto-devel"], update=True, check=True)

        pkg = package_manager.Pkg(self)
        pkg.install(["libglvnd"], update=True, check=True)

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        # TODO: Workaround for #2311 until a better solution can be found
        self.cpp_info.filenames["cmake_find_package"] = "opengl_system"
        self.cpp_info.filenames["cmake_find_package_multi"] = "opengl_system"

        self.cpp_info.set_property("cmake_file_name", "opengl_system")

        self.cpp_info.bindirs = []
        self.cpp_info.includedirs = []
        self.cpp_info.libdirs = []
        if self.settings.os == "Macos":
            self.cpp_info.defines.append("GL_SILENCE_DEPRECATION=1")
            self.cpp_info.frameworks.append("OpenGL")
        elif self.settings.os == "Windows":
            self.cpp_info.system_libs = ["opengl32"]
        elif self.settings.os in ["Linux", "FreeBSD"]:
            pkg_config = PkgConfig(self, 'gl')
            pkg_config.fill_cpp_info(self.cpp_info, is_system=self.settings.os != "FreeBSD")
