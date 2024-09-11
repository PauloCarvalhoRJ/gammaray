from conan import ConanFile, conan_version
from conan.tools.gnu import PkgConfig
from conan.tools.system import package_manager
from conan.errors import ConanInvalidConfiguration
from conan.tools.scm import Version

required_conan_version = ">=1.50.0"

# This recipe doesn't actually build anything.  Instead, it checks whether the X.org/X11
# Linux/BSD packages containing necessary to build Qt in Linux/BSD are installed and 
# propagates env info on them to dependant packages.
# NOTE: You can make this recipe install-if-missing by passing `-c tools.system.package_manager:mode=install`
#       in the `conan create` command provided you have the system privileges to do so.
# This recipe is not needed by Windows builds.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class XorgConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "xorg"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "system"
    # Hints Conan of what kind of information to propagate to dependant packages (e.g. include dir, libs dir, etc.).
    package_type = "shared-library"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the license of the dependency.
    license = "MIT"
    # Declare the home page of the dependency's project.
    homepage = "https://www.x.org/wiki/"
    # A brief description of what the dependency does.
    description = "The X.Org project provides an open source implementation of the X Window System."
    # In addition to name and version, the build must be disntinguished by these info.
    settings = "os", "arch", "compiler", "build_type"
    # State some keywords describing what the dependency does.
    topics = ("x11", "xorg")

    # Validates the configuration.  E.g. Is the detected compiler in the selected Conan
    # profile supported?
    def validate(self):
        if self.settings.os not in ["Linux", "FreeBSD"]:
            raise ConanInvalidConfiguration("This recipe supports only Linux and FreeBSD")

    # Determines which information is used to take part on the hash calculation for
    # the build in Conan cache.
    def package_id(self):
        self.info.clear()

    # Determines/sets OS requirements for the build.
    def system_requirements(self):
        apt = package_manager.Apt(self)
        apt.install(["libx11-dev", "libx11-xcb-dev", "libfontenc-dev", "libice-dev", "libsm-dev", "libxau-dev", "libxaw7-dev",
                     "libxcomposite-dev", "libxcursor-dev", "libxdamage-dev", "libxdmcp-dev", "libxext-dev", "libxfixes-dev",
                     "libxi-dev", "libxinerama-dev", "libxkbfile-dev", "libxmu-dev", "libxmuu-dev",
                     "libxpm-dev", "libxrandr-dev", "libxrender-dev", "libxres-dev", "libxss-dev", "libxt-dev", "libxtst-dev",
                     "libxv-dev", "libxxf86vm-dev", "libxcb-glx0-dev", "libxcb-render0-dev",
                     "libxcb-render-util0-dev", "libxcb-xkb-dev", "libxcb-icccm4-dev", "libxcb-image0-dev",
                     "libxcb-keysyms1-dev", "libxcb-randr0-dev", "libxcb-shape0-dev", "libxcb-sync-dev", "libxcb-xfixes0-dev",
                     "libxcb-xinerama0-dev", "libxcb-dri3-dev", "uuid-dev", "libxcb-cursor-dev", "libxcb-dri2-0-dev",
                     "libxcb-dri3-dev", "libxcb-present-dev", "libxcb-composite0-dev", "libxcb-ewmh-dev",
                     "libxcb-res0-dev"], update=True, check=True)
        apt.install_substitutes(
            ["libxcb-util-dev"], ["libxcb-util0-dev"], update=True, check=True)

        yum = package_manager.Yum(self)
        yum.install(["libxcb-devel", "libfontenc-devel", "libXaw-devel", "libXcomposite-devel",
                           "libXcursor-devel", "libXdmcp-devel", "libXtst-devel", "libXinerama-devel",
                           "libxkbfile-devel", "libXrandr-devel", "libXres-devel", "libXScrnSaver-devel",
                           "xcb-util-wm-devel", "xcb-util-image-devel", "xcb-util-keysyms-devel",
                           "xcb-util-renderutil-devel", "libXdamage-devel", "libXxf86vm-devel", "libXv-devel",
                           "xcb-util-devel", "libuuid-devel", "xcb-util-cursor-devel"], update=True, check=True)

        dnf = package_manager.Dnf(self)
        dnf.install(["libxcb-devel", "libfontenc-devel", "libXaw-devel", "libXcomposite-devel",
                           "libXcursor-devel", "libXdmcp-devel", "libXtst-devel", "libXinerama-devel",
                           "libxkbfile-devel", "libXrandr-devel", "libXres-devel", "libXScrnSaver-devel",
                           "xcb-util-wm-devel", "xcb-util-image-devel", "xcb-util-keysyms-devel",
                           "xcb-util-renderutil-devel", "libXdamage-devel", "libXxf86vm-devel", "libXv-devel",
                           "xcb-util-devel", "libuuid-devel", "xcb-util-cursor-devel"], update=True, check=True)

        zypper = package_manager.Zypper(self)
        zypper.install(["libxcb-devel", "libfontenc-devel", "libXaw-devel", "libXcomposite-devel",
                              "libXcursor-devel", "libXdmcp-devel", "libXtst-devel", "libXinerama-devel",
                              "libxkbfile-devel", "libXrandr-devel", "libXres-devel", "libXss-devel",
                              "xcb-util-wm-devel", "xcb-util-image-devel", "xcb-util-keysyms-devel",
                              "xcb-util-renderutil-devel", "libXdamage-devel", "libXxf86vm-devel", "libXv-devel",
                              "xcb-util-devel", "libuuid-devel", "xcb-util-cursor-devel"], update=True, check=True)

        pacman = package_manager.PacMan(self)
        pacman.install(["libxcb", "libfontenc", "libice", "libsm", "libxaw", "libxcomposite", "libxcursor",
                              "libxdamage", "libxdmcp", "libxtst", "libxinerama", "libxkbfile", "libxrandr", "libxres",
                              "libxss", "xcb-util-wm", "xcb-util-image", "xcb-util-keysyms", "xcb-util-renderutil",
                              "libxxf86vm", "libxv", "xcb-util", "util-linux-libs", "xcb-util-cursor"], update=True, check=True)

        package_manager.Pkg(self).install(["libX11", "libfontenc", "libice", "libsm", "libxaw", "libxcomposite", "libxcursor",
                           "libxdamage", "libxdmcp", "libxtst", "libxinerama", "libxkbfile", "libxrandr", "libxres",
                           "libXScrnSaver", "xcb-util-wm", "xcb-util-image", "xcb-util-keysyms", "xcb-util-renderutil",
                           "libxxf86vm", "libxv", "xkeyboard-config", "xcb-util", "xcb-util-cursor"], update=True, check=True)

        if Version(conan_version) >= "2.0.10":
            alpine = package_manager.Apk(self)
            alpine.install(["libx11-dev", "	libxcb-dev", "libfontenc-dev", "libice-dev", "libsm-dev", "	libxau-dev", "libxaw-dev",
                            "libxcomposite-dev", "libxcursor-dev", "libxdamage-dev", "libxdmcp-dev", "	libxext-dev", "libxfixes-dev", "libxi-dev",
                            "libxinerama-dev", "libxkbfile-dev", "	libxmu-dev", "libxpm-dev", "libxrandr-dev", "libxrender-dev", "libxres-dev",
                            "libxscrnsaver-dev", "libxt-dev", "libxtst-dev", "libxv-dev", "libxxf86vm-dev",
                            "xcb-util-wm-dev", "xcb-util-image-dev", "xcb-util-keysyms-dev", "xcb-util-renderutil-dev",
                            "libxinerama-dev", "libxcb-dev", "xcb-util-dev", "xcb-util-cursor-dev"], update=True, check=True)

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        if Version(conan_version) >= 2:
            self.cpp_info.bindirs = []
            self.cpp_info.includedirs = []
            self.cpp_info.libdirs = []

        for name in ["x11", "x11-xcb", "fontenc", "ice", "sm", "xau", "xaw7",
                     "xcomposite", "xcursor", "xdamage", "xdmcp", "xext", "xfixes", "xi",
                     "xinerama", "xkbfile", "xmu", "xmuu", "xpm", "xrandr", "xrender", "xres",
                     "xscrnsaver", "xt", "xtst", "xv", "xxf86vm",
                     "xcb-xkb", "xcb-icccm", "xcb-image", "xcb-keysyms", "xcb-randr", "xcb-render",
                     "xcb-renderutil", "xcb-shape", "xcb-shm", "xcb-sync", "xcb-xfixes",
                     "xcb-xinerama", "xcb", "xcb-atom", "xcb-aux", "xcb-event", "xcb-util",
                     "xcb-dri3", "xcb-cursor", "xcb-dri2", "xcb-dri3", "xcb-glx", "xcb-present",
                     "xcb-composite", "xcb-ewmh", "xcb-res"] + ([] if self.settings.os == "FreeBSD" else ["uuid"]):
            pkg_config = PkgConfig(self, name)
            pkg_config.fill_cpp_info(
                self.cpp_info.components[name], is_system=self.settings.os != "FreeBSD")
            self.cpp_info.components[name].version = pkg_config.version
            self.cpp_info.components[name].set_property(
                "pkg_config_name", name)
            self.cpp_info.components[name].set_property(
                "component_version", pkg_config.version)
            self.cpp_info.components[name].bindirs = []
            self.cpp_info.components[name].includedirs = []
            self.cpp_info.components[name].libdirs = []
            self.cpp_info.components[name].set_property("pkg_config_custom_content",
                                                        "\n".join(f"{key}={value}" for key, value in pkg_config.variables.items() if key not in ["pcfiledir","prefix", "includedir"]))

        if self.settings.os == "Linux":
            self.cpp_info.components["sm"].requires.append("uuid")
