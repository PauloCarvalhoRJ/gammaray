# -*- coding: utf-8 -*-
import subprocess
import argparse
import glob
import re
import sys
from abc import ABC, abstractmethod
from pathlib import Path
from datetime import datetime
import os
import pkg_resources
import shutil
import platform
import distro
import json

'''
Build - helper

This script is intended to build GammaRay and its dependencies in a single shot
by end users and CI/CD tools.  Developers can use this script to build the 
dependencies both in release and debug modes.  However, they must setup a development
environment (e.g. Qt Creator) to develop and compile GammaRay code.

This script currently works in Windows/Visual Studio and Linux/GCC.
For other OS/compiler combinations, please contact the project maintainers.

Use:
 --help to get help on script usage.
 --deploy to upload compiled binaries to the configured Conan remote package repository.
'''

# Verbose mode flag.
VERBOSE_MODE = False

# Print log to standard output.
def log(msg):
    if VERBOSE_MODE:
        print('[build.py] %s' % msg)

# Print log to a file (no messages to standard output).
def quiet_build_log(config, msg):
    if config.quiet_build:
        config.quiet_build_file.write('[build.py %s] %s\n' % (datetime.now(), msg))
        config.quiet_build_file.flush()

# Set charset code of standard output.
def dec(output):
    return output.stdout.decode('utf-8')

# Returns the Visual Studio commercial version in YYYY format.
def getVSversionYYYY(config):
   if config.compiler_version == '14':
      return '2015'
   elif config.compiler_version == '15':
      return '2017'
   elif config.compiler_version == '16':
      return '2019'
   elif config.compiler_version == '17':
      return '2022'
   else:
      raise Exception("Unknown Visual Studio version.") 

# Returns whether the OS is Windows.
def is_windows():
    return platform.system() == 'Windows'

# Returns whether the OS is Windows.
def is_linux():
    return platform.system() == 'Linux'

# Check OS version.
def check_platform(token, config):
    supported = []
    if is_windows():
        supported = ['win32']
        if config.compiler == "msvc":
            supported.append('VS{}'.format(getVSversionYYYY(config)))
        elif config.compiler == "gcc":
            supported.append('MinGW_{}'.format(config.compiler_version))
        else :
            supported.append('Unknown_compiler_{}'.format(config.compiler_version))
    elif is_linux():
        supported = ['linux']
        supported.append('{}{}'.format(distro.id(), #returns, for example, 'centos' for CentOS
                                       distro.version())) #version() returns only major version number.
                                                          #version(best=True) returns the full version number.
        supported.append('linux_{}{}'.format(config.compiler,
                                             config.compiler_version))
    else:
      raise Exception("This script does not currently support your operating system.  Please, contact the project maintainers.") 
    return token in supported

def get_console_mode(output=False):
    '''
    Returns the mode of the active console input or output buffer.
    Note that this function may throw an EBADF IOError if the
    process is not connected to a console.
    '''
    import msvcrt
    import ctypes
    from ctypes import wintypes

    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)

    device = r'\\.\CONOUT$' if output else r'\\.\CONIN$'
    with open(device, 'r') as con:
        mode = wintypes.DWORD()
        hCon = msvcrt.get_osfhandle(con.fileno())
        kernel32.GetConsoleMode(hCon, ctypes.byref(mode))
        return mode.value

def set_console_mode(mode, output=False):
    '''
    Sets the input or output buffer mode of the active console.
    Note that this function may throw an EBADF IOError if the
    process is not connected to a console.
    '''
    import msvcrt
    import ctypes

    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)

    device = r'\\.\CONOUT$' if output else r'\\.\CONIN$'
    with open(device, 'r') as con:
        hCon = msvcrt.get_osfhandle(con.fileno())
        kernel32.SetConsoleMode(hCon, mode)

def update_console_mode(flags, mask, output=False, restore=False):
    '''
    Updates the subset defined by `mask` of the current buffer mode
    input or output from the active console. Please note that this function can
    throw an EBADF IOError if the process is not connected to a
    console.
    '''
    import atexit

    current_mode = get_console_mode(output)
    if current_mode & mask != flags & mask:
        mode = current_mode & ~mask | flags & mask
        set_console_mode(mode, output)
    else:
        restore = False
    if restore:
        atexit.register(set_console_mode, current_mode, output)

def adjust_windows_console_mode():
    '''
    On Windows, makes adjustments to the current mode of the input or output buffer
    of the active console.
    '''

    if is_windows():
        # Flags associated with the input buffer mode.
        ENABLE_PROCESSED_INPUT = 0x0001
        ENABLE_LINE_INPUT = 0x0002
        ENABLE_ECHO_INPUT = 0x0004
        ENABLE_WINDOW_INPUT = 0x0008
        ENABLE_MOUSE_INPUT = 0x0010
        ENABLE_INSERT_MODE = 0x0020
        ENABLE_QUICK_EDIT_MODE = 0x0040
        ENABLE_EXTENDED_FLAGS = 0x0080
        # Flags associated with the output buffer mode.
        ENABLE_PROCESSED_OUTPUT = 0x0001
        ENABLE_WRAP_AT_EOL_OUTPUT = 0x0002
        ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004 # VT100 (Win 10)

        # Wanted flags.
        flags = ENABLE_EXTENDED_FLAGS
        # Modified flags.
        mask = ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE

        try:
            update_console_mode(flags, mask, restore=True)
        except IOError as err:
            log("Unable to adjust current mode of console buffers:")
            errno, strerror = err.args
            log("\tI/O error({0}): {1}".format(errno, strerror))

def adjust_env_vars():
    '''
    Adjusts the values ​​of some environment variables.
    '''
    if not is_windows():
        if "LD_LIBRARY_PATH" in os.environ:
            del os.environ["LD_LIBRARY_PATH"]
    if is_windows():
        if "UNIX" in os.environ:
            # This variable causes the Boost build to fail on Windows.
            del os.environ["UNIX"]

# Represents a Conan package and its information.
class ConanPackage:
    conanfile = ''
    name = ''
    version = ''
    userchannel = ''
    path = ''
    changed = False

    def __init__(self,
                 conanfile: str,
                 name: str,
                 version: str,
                 userchannel: str,
                 path: str,
                 changed=False):
        self.conanfile = conanfile
        self.name = name
        self.version = version
        self.userchannel = userchannel
        self.path = path
        self.changed = changed

    # Static method to return package reference.
    @classmethod
    def make_ref(self, name, version, userchannel):
        if userchannel == '':
           return '%s/%s' % (name, version)
        else:
           return '%s/%s@%s' % (name, version, userchannel)

    # Returns Conan package reference.
    def ref(self):
        return ConanPackage.make_ref(self.name, self.version, self.userchannel)


# Saves tool usage settings.
class Config:
    userchannel = None
    refname = None
    remote = None
    build_all = False
    export_all = False
    remove_all = False
    skip_build = False
    skip_upload = False
    skip_export = False
    deploy = False
    cleanup = False
    remove_locks = False
    no_overwrite = False
    build_type = ['Release']
    ignored_paths = []
    ignored_file = None
    changed_paths = []
    paths = []
    packages = []
    generate_bundle_conanfile = None
    quiet_build = False
    quiet_build_file = None

    # Returns whether any package has been modified in git.
    def has_packages_changes(self):
        for p in self.packages:
            if p.changed:
                return True
        return False

    # Returns string representation of parameters and settings..
    def to_string(self):
        s = "Used settings: \n"
        s += "--user-channel: {}\n".format(self.userchannel)
        s += "--refname: {}\n".format(self.refname)
        s += "--remote: {}\n".format(self.remote)
        s += "--build_all: {}\n".format(self.build_all)
        s += "--export_all: {}\n".format(self.export_all)
        s += "--remove_all: {}\n".format(self.remove_all)
        s += "--skip_build: {}\n".format(self.skip_build)
        s += "--skip_upload: {}\n".format(self.skip_upload)
        s += "--skip_export: {}\n".format(self.skip_export)
        s += "--deploy: {}\n".format(self.deploy)
        s += "--cleanup: {}\n".format(self.cleanup)
        s += "--remove-locks: {}\n".format(self.remove_locks)
        s += "--no-overwrite: {}\n".format(self.no_overwrite)
        s += "--build_type: {}\n".format(list(dict.fromkeys(self.build_type)))
        s += "--path: {}\n".format(list(dict.fromkeys(self.paths)))
        s += "--ignored_paths: {}\n".format(list(dict.fromkeys(self.ignored_paths)))
        s += "--ignored_file: {}\n".format(self.ignored_file)
        s += "--generate-bundle-conanfile: {}\n".format(self.generate_bundle_conanfile)
        s += "--quiet-build: {}\n".format(self.quiet_build)
        return s


# Utility class.
class Util:
    # Displays program arguments.
    @classmethod
    def get_program_args(self):
        parser = argparse.ArgumentParser(prog='build',
                                         description='CI build helper')
#        parser.add_argument(
#            '-r', '--remote', metavar='STR', default='conancenter',
#            required=False,  action='store',
#            help='Registered Conan remote repository name. '
#                 'Ex: -r conancenter')

        parser.add_argument(
            '-r', '--remote', metavar='STR', default='',
            required=False,  action='store',
            help='Registered Conan remote repository name. '
                 'Ex: -r conancenter')

#        parser.add_argument(
#            '--user_channel', metavar='STR',
#            default='conancenter/stable', required=False,
#            action='store',
#            help='user/channel: Default: conancenter/stable')

        parser.add_argument(
            '--user_channel', metavar='STR',
            default='', required=False,
            action='store',
            help='user/channel: Default: conancenter/stable')

        parser.add_argument(
            '-ccf', '--check-changes-from', metavar='STR',
            default=None, required=False,
            action='store',
            help='Performs rebuild only on paths modified from informed '
                 'refname base (git). Enter the branch name for comparison: '
                 'Ex: remotes/origin/develop, etc')

        parser.add_argument(
            '-cc', '--check-changes', action='store_true',
            default=False, required=False,
            help='Performs rebuild only on paths modified from HEAD~1'
                 'To reset the refname to be checked, use: '
                 'Ex: --check-changes-from remotes/origin/develop')

        parser.add_argument(
            '-f', '--force', action='store_true',
            default=False, required=False,
            help='Force rebuild all found packages.'
                 'This option disables the --check-changes option. '
                 'Ex: --check-changes-from remotes/origin/develop')

        parser.add_argument(
            '--export-all', action='store_true', default=False,
            help='Export all packages locally.',
            required=False)

        parser.add_argument(
            '--remove-all', action='store_true', default=False,
            help='Remove all packages locally.',
            required=False)

        parser.add_argument(
            '--build-type', action='append', default=[],
            help='Defines Debug/Release build modes for packages. '
            'By default only Release mode is added. To change the standard'
            'mode it is necessary to inform the substitute modes in sequence, '
            'there may be more than one:'
            'Ex: --build-type Debug --build-type Release. ',
            required=False)

        parser.add_argument(
            '--ignored-file', metavar='PATH',
            default='ignored_package_paths.txt', required=False,
            action='store',
            help='Inform a file with the list of paths to be ignored from '
                 'package generation. The following format is accepted: '
                 '<path>/<version>, <platformA>, <platformB>, <platformN>. '
                 'Where each line must have the package folder and version to be '
                 'removed from compilation, optionally filtered by a '
                 'list of platforms where the rule will apply, for example, '
                 'win32, linux, etc. If no platform is informed, '
                 'the rule will apply to any platform. '
                 'By default, it uses a file named ignored_package_paths.txt in the '
                 'repository root. To enter another file, use this '
                 'option with the desired file name. '
                 'Ex: --ignored-file myfile.txt.'
                 )

        parser.add_argument(
            '-i', '--ignored-paths', action='append',
            default=[],
            help='Defines the paths of packages to be removed from the compilation. '
            'There can be more than one in sequence: '
            'Ex: --ignored-path gmock/1.8.0 --ignored-path gtest/1.8.0 ',
            required=False)

        parser.add_argument(
            '-p', '--path', action='append',
            default=[],
            help='Defines the paths of the packages that will be used in the compilation. '
            'When using this option the script stops finding candidate packages '
            'in the file system, starting to compile only the indicated paths. '
            'There can be more than one in sequence:'
            'Ex: --path gmock/1.8.0 --path gtest/1.8.0 ',
            required=False)

        parser.add_argument(
            '--skip-build', action='store_true', default=False,
            help='Skip the installation step for found packages. ',
            required=False)

        parser.add_argument(
            '--skip-upload', action='store_true', default=False,
            help='In deploy, the packages are not sent. Just compress and '
                 'performs integrity check of the created package. ',
            required=False)

        parser.add_argument(
            '--skip-export', action='store_true', default=False,
            help='Does not export recipes to the local copy. ',
            required=False)

        parser.add_argument(
            '-v', '--verbose', action='store_true', default=False,
            help='Print log messages. ',
            required=False)

        parser.add_argument(
            '-q', '--quiet-build', action='store_true', default=False,
            help='Suppresses compilation messages. Compilation errors will continue to appear.',
            required=False)

        parser.add_argument(
            '--generate-bundle-conanfile', metavar='DIR',
            default=None, required=False,
            action='store',
            help='Generates a conanfile.txt file in the informed DIR, which brings together the references of all selected packages.')

        parser.add_argument(
            '--deploy', action='store_true', default=False,
            help='Deploy the package if OK. ',
            required=False)

        parser.add_argument(
            '--cleanup', action='store_true', default=False,
            help='Remove temporary builds and outdated packages.',
            required=False)

        parser.add_argument(
            '--remove-locks', action='store_true', default=False,
            help='Remove locks from local cache.',
            required=False)

        parser.add_argument(
            '--no-overwrite', action='store_true', default=False,
            help='Sends packages without overwriting the recipe and packages if the recipe has been modified.',
            required=False)

        return parser.parse_args()

    # Gets the requested attribute of a Conan recipe.
    @classmethod
    def get_package_attr(self, conanfile: str, attr: str):
        json_result = json.loads(dec(subprocess.run('conan inspect %s --format=json' % (conanfile),
                                  shell=True, stdout=subprocess.PIPE)))
        return json_result[attr]

    # Checks if the package exists locally.
    @classmethod
    def exists_package(self, ref: str):
        res = subprocess.run('conan search %s -o' % ref,
                             shell=True,
                             stderr=subprocess.PIPE,
                             stdout=subprocess.DEVNULL).stderr.decode('utf-8')
        return 'ERROR' not in res

    # Retrieves modified packages in git based on the given refname.
    @classmethod
    def get_changed_paths(self, config: Config):
        log('Recovering modified packages for refname: %s.' % config.refname)
        ps = dec(subprocess.run("git diff --dirstat=files,0 %s" % config.refname,
                                shell=True,
                                stdout=subprocess.PIPE)).split('\n')
        paths = []
        for i in ps:
            path = i.lstrip()
            if len(path) > 0:
                paths.append(re.sub(r"^[0-9. ]+% ", "", path))
        log("Modified paths: %s " % paths)
        return paths

    @classmethod
    def read_ignored_file(self, config: Config):
        if config.ignored_file is not None:
            file = Path(config.ignored_file)
            if file.exists():
                lines = []
                with open(config.ignored_file) as f:
                    lines = f.readlines()
                for f in lines:
                    tokens = f.split(',')
                    if len(tokens) > 0:
                        path = tokens[0].strip().replace('/', os.sep)
                        if path in config.ignored_paths:
                            log("Error: Path '{}' was included more than once in the ignored_file.".format(path))
                            sys.exit(1)
                        if len(tokens[1:]) > 0:  # filter by platform.
                            for s in tokens[1:]:
                                if check_platform(s.strip(), config):
                                    config.ignored_paths.append(path)
                                    continue
                        else:
                            config.ignored_paths.append(path)
        print('Ignored paths: {}'.format(config.ignored_paths))

    # Retrieves the packages to be generated and their information..
    @classmethod
    def get_packages(self, config: Config):
        log('Retrieving packets to be processed...')
        packages = []
        unusedPaths = config.paths.copy()
        for f in glob.glob('**/conanfile.py', recursive=True):
            if 'test_package' in str(Path(f)):
                continue
            path = str(Path(f).parent)
            if path not in config.ignored_paths:
                changed = False
                if len(config.paths) > 0:
                    if path in config.paths:
                        unusedPaths.remove(path)
                        changed = True;
                    else:
                        continue
                name = Util.get_package_attr(f, 'name')
                version = Util.get_package_attr(f, 'version')
                ref = ConanPackage.make_ref(name, version, config.userchannel)
                changed = changed and not Util.exists_package(ref)

                _tempPath = path.replace('\\', '/')
                changed = changed or config.build_all \
                    or config.export_all \
                    or any(_tempPath in s for s in config.changed_paths)
                package = ConanPackage(f, name, version,
                                       config.userchannel, path, changed)

                if "qt/" in package.ref():
                    packages.insert(0, package)
                else:
                    packages.append(package)

                log('Reading package %s \t %s' % (package.ref(),
                                               '[MODIFIED]' if changed else ''))
        if unusedPaths:
            log('Error: Unable to find valid packages in the following paths: {}'.format(unusedPaths))
            sys.exit(1)

        return packages

    # Generates conanfile.txt with the references of all selected packages.
    @classmethod
    def generate_bundle_conanfile(self, config: Config):
        txt = '[requires]\n'
        for package in config.packages:
            txt += package.ref() + '\n'
        with open(os.path.join(config.generate_bundle_conanfile, 'conanfile-%s.txt' % ('windows' if is_windows() else 'linux')), "w") as text_file:
            text_file.write(txt)

    # Remove locks from local cache.
    @classmethod
    def remove_locks(self):
        log("Removendo locks ")
        subprocess.run('conan remove --locks', shell=True, check=False)

    # Creates the configuration file based on the program arguments.
    @classmethod
    def get_config(self, args):
        config = Config()
        config.userchannel = args.user_channel
        if args.check_changes or args.check_changes_from is not None:
            config.refname = 'HEAD~1' if args.check_changes_from is None \
                                      else args.check_changes_from
        config.remote = args.remote
        config.build_all = args.force
        config.export_all = args.export_all
        config.remove_all = args.remove_all
        config.skip_build = args.skip_build
        config.skip_upload = args.skip_upload
        config.skip_export = args.skip_export
        config.deploy = args.deploy
        config.cleanup = args.cleanup

        config.quiet_build = args.quiet_build
        if config.quiet_build:
            config.quiet_build_file = open("quiet_build_log-%s.txt" % ('windows' if is_windows() else 'linux'), "w", encoding='utf-8');

        config.remove_locks = args.remove_locks
        config.paths = []
        for p in list(dict.fromkeys(args.path)):
            config.paths.append(p.strip().replace('/', os.sep))
        config.ignored_paths = list(dict.fromkeys(args.ignored_paths))
        config.ignored_file = args.ignored_file
        config.generate_bundle_conanfile = args.generate_bundle_conanfile
        if len(args.build_type):
            config.build_type = list(dict.fromkeys(args.build_type))
        if not config.skip_build and config.remove_all:
            config.build_all = True

        # Makes sure we have at least a default profile
        profile = dec(subprocess.run('conan profile list',
                                     shell=True, stdout=subprocess.PIPE)).strip()
        if profile == 'No profiles defined':
            profile = 'default'
            subprocess.run("conan profile new {} --detect".format(profile),
                           shell=True, stdout=subprocess.PIPE)
        else:
            profile = dec(subprocess.run('conan config show core:default_profile',
                                         shell=True, stdout=subprocess.PIPE)).strip()
            if profile == '':
               profile = 'default'

        # Detect compiler name and version to compare with ignored_package_paths.txt.
        # -pr:a == apply same profile to both host (:h==current machine) and build (:b==target machine) contexts.
        profiles_json = json.loads(dec(subprocess.run('conan profile show -pr:a=%s --format=json' % profile,
                                             shell=True, stdout=subprocess.PIPE)).strip())
        config.compiler =         profiles_json['build']['settings']['compiler']
        config.compiler_version = profiles_json['build']['settings']['compiler.version']

        return config

    # Build environment information.
    @classmethod
    def envinfo(self):
        log("Getting build environment information...")
        s = "\n[ENVIRONMENT INFO]:\n"

        # Python
        s += "\n[Python]\n"
        s += "\tPython binary: {}\n".format(sys.executable)
        s += "\tPython version: {}.{}.{}\n".format(sys.version_info.major, sys.version_info.minor, sys.version_info.micro)

        # Conan
        s += "\n[Conan]\n"
        s += "\tConan location: {}\n".format(pkg_resources.get_distribution('conan').location)
        s += "\tConan version: {}\n".format(pkg_resources.get_distribution('conan').version)

        # Compiler
        if is_windows():
            try:
            # cl
                s += "\n[Cl]\n"
                cl_binary = shutil.which('cl')
                s += "\tCl location: {}\n".format(cl_binary)
            except OSError as e:
                s += ("\tERROR: CL not found. Run this script from a suitable 'x64 Native Tools Command Prompt' cmd.\n")
                s += ("\t       Other Windows compilers such as MinGW's gcc or clang are not currently supported by this script.  Please, contact the project maintainers.\n")
        elif is_linux():
            # g++
            s += "\n[g++]\n"
            gpp_binary = shutil.which('g++')
            gpp_version = subprocess.run(['g++', '--version'], stdout=subprocess.PIPE)
            gpp_version = str(gpp_version.stdout).split(" ")[2]
            s += "\tg++ location: {}\n".format(gpp_binary)
            s += "\tg++ version: {}\n".format(gpp_version)

        # Environment vars
        s += "\n[Environment variables]\n"
        for v in os.environ.items():
            s += "\t{} = {}\n".format(v[0], v[1])

        print(s)

# Abstract class to represent a job to be executed.
class Job(ABC):
    def __init__(self):
        super().__init__()

    # Virtual method to perform a task.
    @abstractmethod
    def run(self, package: ConanPackage, config: Config):
        pass

    # Runs the task for all selected packages.
    def run_all(self, config: Config):
        failures = []
        for package in config.packages:
            try:
                self.run(package, config)
            except subprocess.CalledProcessError:
                fail = 'Package failed: %s. ' % package.ref()
                log(fail)
                failures.append(fail)
                continue
        return failures


# Export a package locally.
class Export(Job):
    def run(self, package: ConanPackage, config: Config):
        if package.changed and not config.skip_export:
            log("Exporting package %s " % package.ref())
            subprocess.run('conan export %s %s' % (package.conanfile,
                                                   config.userchannel),
                           shell=True, check=True)


# Remove builds and, eventually, temporary sources from the local package.
class CleanUp(Job):
    def run(self, package: ConanPackage, config: Config):
        log("Removing intermediary artifacts for package %s " % package.ref())
        subprocess.run('conan remove %s -b -s -f' % (package.ref()),
                       shell=True, check=True)


# Install the package locally, forcing the build if necessary.
# Packages that have not been modified or outdated will not be
# recompiled. Only exported packages are recompiled.
class Install(Job):
    count = 0

    def run(self, package: ConanPackage, config: Config):
        for build_type in config.build_type:
            self.count += 1
            quiet_build_log(config, "%02d / %02d Installing package %s - mode %s " %
                            (self.count, len(config.packages) * len(config.build_type),
                            package.ref(), build_type))
            log("Installing package %s - mode %s " % (package.ref(),
                                                     build_type))
            bt = '-s build_type=%s' % build_type
            bm = '--build=missing'
            bo = '--build=outdated'
            bb = '--build=%s' % package.name if package.changed else ''
            up = '-u' if not package.changed else ''

            cmd = 'conan install %s %s %s %s %s %s' % (package.ref(),
                                                       bt, bm, bo,
                                                       bb, up)
            if config.quiet_build:
                subprocess.run(cmd, shell=True, check=True,
                               stdout=subprocess.DEVNULL)
            else:
                subprocess.run(cmd, shell=True, check=True)

# Remove entire package from local cache.
class Remove(Job):
    def run(self, package: ConanPackage, config: Config):
        log("Removing package %s " % package.ref())
        try:
            subprocess.run('conan remove %s -f' % package.ref(),
                       shell=True, check=True, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as e:
            if not 'Recipe not found' in str(e.output):
                raise e

# Sends the recipe and packages (binaries) to the informed remote repository.s
# You must have entered one or used the default remote.
class Deploy(Job):
    def run(self, package: ConanPackage, config: Config):
        log("Sending package %s to remote server: %s " % (package.ref(),
                                                              config.remote))
        skip = '--skip-upload' if config.skip_upload else ''
        force = '--force' if package.changed else ''
        subprocess.run('conan upload %s --all -c -r %s %s %s' % (package.ref(),
                                                                 config.remote,
                                                                 skip, force),
                       shell=True, check=True)


# Main function.
def main():

    # On Windows, performs adjustments to the input mode of a console input buffer and/or
    # in output mode of a console screen buffer.    adjust_windows_console_mode()

    # Gets program parameters.
    args = Util.get_program_args()

    global VERBOSE_MODE
    VERBOSE_MODE = args.verbose

    # Creates object with the settings to be used.
    config = Util.get_config(args)

    # Adjusts some environment variables.
    adjust_env_vars()

    log('Start of package generation.')
    log(config.to_string())

    failures = {}

    # Prints environment information.
    if VERBOSE_MODE:
        try:
            Util.envinfo()
        except Exception as ex:
            print("Error getting environment information: ", ex)
            print("Continuing anyway...")

    # Reads the file with ignored paths.
    if config.ignored_file is not None:
        Util.read_ignored_file(config)

    # Removes locks from previous or competing processes.
    if config.remove_locks:
        Util.remove_locks()

    # Gets the modified paths according to the configured refname, if not
    # the build-all parameter has been informed.
    if config.refname is not None and not config.build_all:
        config.changed_paths = Util.get_changed_paths(config)

    # Retrieves candidate packages with their information.
    config.packages = Util.get_packages(config)

    # Generates conanfile.txt with all packages.
    if config.generate_bundle_conanfile is not None:
        Util.generate_bundle_conanfile(config)

    # Remove packages if this option was informed.
    if config.remove_all:
        failures['Removed'] = Remove().run_all(config)

    # Export only modified packages or all if build_all was
    # informed. If skip_build was informed, it does not export anything.
    if config.export_all or (not config.skip_build and config.has_packages_changes()):
        failures['Exported'] = Export().run_all(config)

    if VERBOSE_MODE:
        total, used, free = shutil.disk_usage("/")

        temp = 2**30
        quiet_build_log(config, "Disk usage before installation:\nTotal: %d GiB\nUsed: %d GiB\nFree: %d GiB" % (total // temp, used // temp, free // temp))

    # Installs packages locally.
    # This procedure will compile modified packages or those that have
    # some updated dependency.
    if not config.skip_build:
        failures['Build/Installed'] = Install().run_all(config)

    if VERBOSE_MODE:
        total, used, free = shutil.disk_usage("/")

        temp = 2**30
        quiet_build_log(config, "Disk usage after installation:\nTotal: %d GiB\nUsed: %d GiB\nFree: %d GiB" % (total // temp, used // temp, free // temp))

    # Sends the generated packets to the specified remote server.
    if (config.skip_build or len(failures['Build/Installed']) == 0) and config.deploy and config.remote is not None:
        failures['Deploy'] = Deploy().run_all(config)

    # Remove temporary builds and obsolete packages.
    if config.cleanup:
        failures['Temporaries cleanse:'] = CleanUp().run_all(config)

    if VERBOSE_MODE:
        total, used, free = shutil.disk_usage("/")

        temp = 2**30
        quiet_build_log(config, "Disk usage after cleanup:\nTotal: %d GiB\nUsed: %d GiB\nFree: %d GiB" % (total // temp, used // temp, free // temp))

    fail = False
    msg = '\n==== Error report ====\n'
    for k, v in failures.items():
        if len(v) > 0:
            msg += '%s errors: %s\n ' % (k, v)
            fail = True
    if fail:
        print('[build.py] %s' % msg)
        sys.exit(1)
    print('[build.py] All wanted packages were generated successfully.')


if __name__ == "__main__":
    main()
