from conan import ConanFile
from conan.tools.gnu import PkgConfig
from conan.tools.system import package_manager
from conan.errors import ConanInvalidConfiguration
from conan.tools.layout import basic_layout

required_conan_version = ">=1.50.0"

# This recipe doesn't actually build anything.  Instead, it checks whether the X.org/X11
# Linux/BSD packages containing keyboard databases necessary to build Qt in Linux/BSD are installed and 
# propagates env info on them to dependant packages.
# NOTE: You can make this recipe install-if-missing by passing `-c tools.system.package_manager:mode=install`
#       in the `conan create` command provided you have the system privileges to do so.
# This recipe is not needed by Windows builds.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class XkeyboardConfigConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "xkeyboard-config"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "system"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "application"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the license of the dependency.
    license = "MIT"
    # Declare the home page of the dependency's project.
    homepage = "https://www.freedesktop.org/wiki/Software/XKeyboardConfig/"
    # A brief description of what the dependency does.
    description = "The non-arch keyboard configuration database for X Window."
    # In addition to name and version, the build must be disntinguished by these info.
    settings = "os", "compiler", "build_type" # no arch here, because the xkeyboard-config system package is arch independant
    # State some keywords describing what the dependency does.
    topics = ("x11", "xorg", "keyboard")

    # Validates the configuration.  E.g. Is the detected compiler in the selected Conan
    # profile supported?
    def validate(self):
        if self.settings.os not in ["Linux", "FreeBSD"]:
            raise ConanInvalidConfiguration("This recipe supports only Linux and FreeBSD")

    # Determines which information is used to take part on the hash calculation for
    # the build in Conan cache.
    def package_id(self):
        self.info.clear() #no info other than name and version takes part in package hash (no actual build).

    # Customize the default Conan workspace before building (e.g. define the name for the source directory).
    def layout(self):
        basic_layout(self, src_folder="src")

    # Determines/sets OS requirements for the build.
    def system_requirements(self):
        # For distros using the Apt package manager.
        apt = package_manager.Apt(self)
        apt.install(["xkb-data"], update=True, check=True)
        # For distros using the yum package manager.
        yum = package_manager.Yum(self)
        yum.install(["xkeyboard-config-devel"], update=True, check=True)
        # For distros using the dnf package manager.
        dnf = package_manager.Dnf(self)
        dnf.install(["xkeyboard-config-devel"], update=True, check=True)
        # For distros using the zypper package manager.
        zypper = package_manager.Zypper(self)
        zypper.install(["xkeyboard-config"], update=True, check=True)
        # For distros using the pacman package manager.
        pacman = package_manager.PacMan(self)
        pacman.install(["xkeyboard-config"], update=True, check=True)
        # For distros using some other package manager (fallback case).
        package_manager.Pkg(self).install(["xkeyboard-config"], update=True, check=True)

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        pkg_config = PkgConfig(self, "xkeyboard-config")
        pkg_config.fill_cpp_info(
            self.cpp_info, is_system=self.settings.os != "FreeBSD")
        self.cpp_info.set_property("pkg_config_name", "xkeyboard-config")
        self.cpp_info.set_property("component_version", pkg_config.version)
        self.cpp_info.set_property("pkg_config_custom_content",
                                                    "\n".join(f"{key}={value}" for key, value in pkg_config.variables.items() if key not in ["pcfiledir","prefix", "includedir"]))
