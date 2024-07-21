from conan import ConanFile
from conan.errors import ConanInvalidConfiguration, ConanException
from conan.tools.files import chdir, get, replace_in_file, copy, unzip
from conan.tools.layout import basic_layout
import fnmatch
import os
import shutil
import subprocess
import errno
import ctypes

required_conan_version = ">=1.47.0"

# This recipe installs the precompiled MSYS2 base system into Conan cache and installs 
# in it some Pacman packages needed for building software (e.g. MinGW64 compiler, make, etc.).
# NOTE: MSYS2 is a requirement only for certain packages building on Windows without Visual Studio.

# Auxiliary class used to define critical sections in recipe code.
class lock:
    def __init__(self):
        self.handle = ctypes.windll.kernel32.CreateMutexA(None, 0, "Global\\ConanMSYS2".encode())
        if not self.handle:
            raise ctypes.WinError()

    def __enter__(self):
        status = ctypes.windll.kernel32.WaitForSingleObject(self.handle, 0xFFFFFFFF)
        if status not in [0, 0x80]:
            raise ctypes.WinError()

    def __exit__(self, exc_type, exc_val, exc_tb):
        status = ctypes.windll.kernel32.ReleaseMutex(self.handle)
        if not status:
            raise ctypes.WinError()

    def close(self):
        ctypes.windll.kernel32.CloseHandle(self.handle)

    __del__ = close

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class MSYS2Conan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "msys2"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "cci.latest"
    # A brief description of what the dependency does.
    description = "MSYS2 is a software distro and building platform for Windows"
    # Declare where the content was obtained from.
    url = "https://github.com/conan-io/conan-center-index"
    # Declare the home page of the dependency's project.
    homepage = "http://www.msys2.org"
    # Declare the license of the dependency.
    license = "MSYS license"
    # State some keywords describing what the dependency does.
    topics = ("msys", "unix", "subsystem")
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    # In this case, only by OS and architecure as this recipe does not actually build MSYS2.
    settings = "os", "arch"
    # A set of additional custom options that are necessary to build the dependency.
    # "exclude_files" "packages" "additional_packages" values are a comma separated list
    options = {
        "exclude_files": ["ANY"],
        "packages": ["ANY"],
        "additional_packages": [None, "ANY"],
        "no_kill": [True, False]
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {
        "exclude_files": "*/link.exe",
        "packages": "base-devel,binutils,gcc",
        "additional_packages": None,
        "no_kill": False,
    }
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "msys2-base-x86_64-20231026.tar.xz"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def _update_pacman(self):
        with chdir(self, os.path.join(self._msys_dir, "usr", "bin")):
            try:
                self._kill_pacman()

                # https://www.msys2.org/docs/ci/
                self.run('bash -l -c "pacman --debug --noconfirm --ask 20 -Syuu"')  # Core update (in case any core packages are outdated)
                self._kill_pacman()
                self.run('bash -l -c "pacman --debug --noconfirm --ask 20 -Syuu"')  # Normal update
                self._kill_pacman()
                self.run('bash -l -c "pacman --debug -Rc dash --noconfirm"')
            except ConanException:
                self.run('bash -l -c "cat /var/log/pacman.log || echo nolog"')
                self._kill_pacman()
                raise

    # https://github.com/msys2/MSYS2-packages/issues/1966
    def _kill_pacman(self):
        if self.options.no_kill:
            return
        if (self.settings.os == "Windows"):
            taskkill_exe = os.path.join(os.environ.get('SystemRoot'), 'system32', 'taskkill.exe')

            log_out = True
            if log_out:
                out = subprocess.PIPE
                err = subprocess.STDOUT
            else:
                out = open(os.devnull, 'w', encoding='UTF-8')
                err = subprocess.PIPE

            if os.path.exists(taskkill_exe):
                taskkill_cmds = [
                    f"{taskkill_exe} /f /t /im pacman.exe",
                    f"{taskkill_exe} /f /im gpg-agent.exe",
                    f"{taskkill_exe} /f /im dirmngr.exe",
                    f'{taskkill_exe} /fi "MODULES eq msys-2.0.dll"',
                ]
                for taskkill_cmd in taskkill_cmds:
                    try:
                        proc = subprocess.Popen(taskkill_cmd, stdout=out, stderr=err, bufsize=1)
                        proc.wait()
                    except OSError as e:
                        if e.errno == errno.ENOENT:
                            raise ConanException("Cannot kill pacman") from e

    @property
    def _msys_dir(self):
        subdir = "msys64" # top-level directoy in tarball
        return os.path.join(self.source_folder, subdir)

    def _do_build(self):
        packages = []
        if self.options.packages:
            packages.extend(str(self.options.packages).split(","))
        if self.options.additional_packages:
            packages.extend(str(self.options.additional_packages).split(","))

        self._update_pacman()

        with chdir(self, os.path.join(self._msys_dir, "usr", "bin")):
            for package in packages:
                self.run(f'bash -l -c "pacman -S {package} --noconfirm"')
            for package in ['pkgconf']:
                self.run(f'bash -l -c "pacman -Rs -d -d $(pacman -Qsq {package}) --noconfirm"')

        self._kill_pacman()

        # create /tmp dir in order to avoid
        # bash.exe: warning: could not find /tmp, please create!
        tmp_dir = os.path.join(self._msys_dir, 'tmp')
        if not os.path.isdir(tmp_dir):
            os.makedirs(tmp_dir)
        tmp_name = os.path.join(tmp_dir, 'dummy')
        with open(tmp_name, 'a', encoding='UTF-8'):
            os.utime(tmp_name, None)

        # Prepend the PKG_CONFIG_PATH environment variable with an eventual PKG_CONFIG_PATH environment variable
        # Note: this is no longer needed when we exclusively support Conan 2 integrations
        replace_in_file(self, os.path.join(self._msys_dir, "etc", "profile"),
                              'PKG_CONFIG_PATH="', 'PKG_CONFIG_PATH="${PKG_CONFIG_PATH:+${PKG_CONFIG_PATH}:}')


    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self, src_folder="src")

    # This is used to define which settings and options parcticipate in the hash calculation
    # for the package in Conan's cache.
    # In this case, we don't want the no_kill setting to change the hash.
    def package_id(self):
        del self.info.options.no_kill

    # This callback is evoked to perform some validations (e.g. unsupported operating system) before building.
    # The detection of an invalid configuration must be signaled by raising an exception.
    def validate(self):
        if self.settings.os != "Windows":
            raise ConanInvalidConfiguration("Only Windows supported")
        if self.settings.arch != "x86_64":
            raise ConanInvalidConfiguration("Only Windows x64 supported")

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the build directory in Conan's workspace.
    def source(self):
        #get(self, **self.conan_data["sources"][self.version],
        #    destination=self.source_folder, strip_root=False) # Preserve tarball root dir (msys64/)  --> in the original recipe, this was used to download the file with the URL in conandata.yml.
        unzip(self, filename=os.path.join(self.source_folder, "..", "msys2-base-x86_64-20231026.tar.xz"), destination=".")

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        with lock():
            self._do_build()

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        excludes = None
        if self.options.exclude_files:
            excludes = tuple(str(self.options.exclude_files).split(","))
        for exclude in excludes:
            for root, _, filenames in os.walk(self._msys_dir):
                for filename in filenames:
                    fullname = os.path.join(root, filename)
                    if fnmatch.fnmatch(fullname, exclude):
                        os.unlink(fullname)
        # See https://github.com/conan-io/conan-center-index/blob/master/docs/error_knowledge_base.md#kb-h013-default-package-layout
        copy(self, "*", dst=os.path.join(self.package_folder, "bin", "msys64"), src=self._msys_dir, excludes=excludes)
        shutil.copytree(os.path.join(self._msys_dir, "usr", "share", "licenses"),
                        os.path.join(self.package_folder, "licenses"))

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `tool_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = []

        msys_root = os.path.join(self.package_folder, "bin", "msys64")
        msys_bin = os.path.join(msys_root, "usr", "bin")
        self.cpp_info.bindirs.append(msys_bin)

        self.buildenv_info.define_path("MSYS_ROOT", msys_root)
        self.buildenv_info.define_path("MSYS_BIN", msys_bin)

        self.conf_info.define("tools.microsoft.bash:subsystem", "msys2")
        self.conf_info.define("tools.microsoft.bash:path", os.path.join(msys_bin, "bash.exe"))

        # conan v1 specific stuff
        self.env_info.MSYS_ROOT = msys_root
        self.env_info.MSYS_BIN = msys_bin
        self.env_info.path.append(msys_bin)
