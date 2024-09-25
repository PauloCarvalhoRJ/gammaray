from conan import ConanFile
from conan.tools.files import unzip, replace_in_file, copy
from conan.tools.build import build_jobs
from conan.tools.layout import basic_layout
import os
import subprocess
import sys

# This recipe builds (Linux), installs (packages) and tests Python 3.10.
# In Windows, it extracts precompiled binaries from the zip file accompaining this recipe.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class Python3Conan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "python3"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "3.10.11"
    # Declare the license of the dependency.
    license = "Python Software Foundation License"
    # Declare where the content was obtained from.
    url = "https://www.python.org/"
    # A brief description of what this recipe does.
    description = "Python is an interpreted, object-oriented programming language."
    # Declare what information are needed to build and uniquely identify this package in addition to name and version.
    # Notice that build_type is missing, meaning that only the release version of Python will be built.
    settings = "os", "compiler", "arch"
    # A set of additional custom options that are necessary to build the dependency.
    options = {
        "shared": [True, False],
        "os_version": ["linux", "windows", "other"],
        "abi_version": ["py310"]
    }
    # Declare which are the default values to the options in the options attribute.
    default_options = {"shared": True, "abi_version": "py310"}
    # Do not copy the source files to the build directory in order to build. This speeds up build time by 
    # avoiding unnecessary copies but the you HAVE to make sure no configure or build script/process change any 
    # of the source files (e.g. apply a patch).
    no_copy_source = True
    # Declare where the tarball with the dependency's source code was downloaded from.
    source_url = "https://www.python.org/ftp/python/{0}/Python-{0}.tar.xz".format(version)
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = "Python-{}.*".format(version), "pip_modules_*.zip"
    # True may cause problems when generating the installer.
    short_paths = False

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    def create_enable_python_script(self):
        is_linux   = ( self.options.os_version == "linux"   )
        is_windows = ( self.options.os_version == "windows" )
        enable_filename = os.path.join(self.package_folder, "enable_python.%s" % ('bat' if is_windows else 'sh'))
        if is_linux:
            with open(enable_filename, 'w') as enable_file:
                enable_file.write("export PATH=%s/bin${PATH:+:${PATH}}\n" % self.package_folder)
                enable_file.write("export LD_LIBRARY_PATH=%s/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}\n" % self.package_folder)
                enable_file.write("export MANPATH=%s/share/man:$MANPATH\n" % self.package_folder)
                enable_file.write("export PKG_CONFIG_PATH=%s/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}\n" % self.package_folder)
                enable_file.write("export XDG_DATA_DIRS=\"%s/share${XDG_DATA_DIRS:-/usr/local/share:/usr/share}\"\n" % self.package_folder)
        elif is_windows:
            with open(enable_filename, 'w') as enable_file:
                enable_file.write("@echo off\n")
                enable_file.write("SET PATH={0};{0}\\Scripts;%PATH%".format(self.package_folder))
        self.output.info("Python activate script (%s) in package folder." % self.package_folder)

    @property
    def source_subfolder(self):
        return "Python-{}".format(self.version)

    @property
    def source_zip_filename(self):
        is_linux = ( self.options.os_version == "linux" )
        return "{}.{}".format(self.source_subfolder, "tar.xz" if is_linux else "zip")

    @property
    def pip_modules_filename(self):
        if self.options.os_version == "windows":
            return "pip_modules_win.zip"
        return "pip_modules_linux.zip"

    @property
    def major_minor(self):
        return '.'.join(self.version.split('.', 3)[0:2])

    @property
    def major_minor_short(self):
        return ''.join(self.version.split('.', 3)[0:2])

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Runs the configuration of settings and options in the recipe for later use in the different methods like 
    # generate(), build() or package(). This method executes while building the dependency graph and expanding the
    # packages dependencies.
    def configure(self):
        if self.settings.os == "Windows":
            self.options.os_version = "windows"
        elif self.settings.os == "Linux":
            self.options.os_version = "linux"
        else:
            self.options.os_version = "other"

    # Allows to customize some standard attributes (e.g. self.folders).
    # In this case, we inform Conan that the source directory is the "src" subdirectory in 
    # the root source directory in Conan workspace.
    def layout(self):
        basic_layout(self)

    # Contains the package build instructions.
    # Generates build artifacts in the build directory from the source files in the source directory.
    def build(self):
        unzip(self, filename=os.path.join(self.source_folder, self.source_zip_filename),  destination=self.build_folder, keep_permissions=True)
        unzip(self, filename=os.path.join(self.source_folder, self.pip_modules_filename), destination=self.build_folder, keep_permissions=True)
        if self.options.os_version == "linux":
            args = ["--prefix=%s" % (self.package_folder)]
            if self.options.shared:
                args.append("--enable-shared")

            if not os.path.isdir('/usr/include/openssl11'):
                raise Exception('The openssl11-devel package >= 1.1.1 needs to be installed.')

            # Needed for sci-py.
            if not os.path.isfile('/usr/include/ffi.h'):
                raise Exception('The libffi-devel package needs to be installed.')

            replace_in_file(self, file_path='{}/configure'.format(self.source_subfolder), search='$PKG_CONFIG openssl', replace='$PKG_CONFIG openssl11')

            self.run("{}/configure {}".format(os.path.join(self.build_folder, self.source_subfolder), " ".join(args)))
            self.run("make -j %d" % build_jobs(self))
            self.run("strip %s/libpython%s.so.1.0" % (self.build_folder, self.major_minor))
        elif self.options.os_version == "windows":
            self.output.info("Decompressing archive with the binaries.")
            unzip(self, filename=os.path.join(self.source_folder, self.source_zip_filename))

    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    def package(self):
        if self.options.os_version == "linux":
            self.run("make install")
            self.run("cd %s/bin && ln -s python3 python && ln -s pip3 pip" % self.package_folder)
            package_root_folder = os.path.dirname(self.package_folder)
            package_root_folder = os.path.dirname(package_root_folder)
            with open("{}/build_package_folder.txt".format(self.package_folder), 'w') as f:
                f.write(package_root_folder)

            os.environ['PATH'] = ("%s/bin" % self.package_folder) + os.pathsep + os.environ.get('PATH', '')
            os.environ['LD_LIBRARY_PATH'] = ("%s/lib" % self.package_folder) + os.pathsep + os.environ.get('LD_LIBRARY_PATH', '')
            os.environ['PYTHONPATH'] = ("%s/lib/python%s/site-packages" % (self.package_folder, self.major_minor)) + os.pathsep + os.environ.get('PYTHONPATH', '')
            pythonExecutable = "%s/bin/python" % self.package_folder

        elif self.options.os_version == "windows":
            copy(self, pattern="*" , dst=self.package_folder, src=os.path.join(self.build_folder, self.source_subfolder))
            os.environ['PATH'] = ("%s\\Scripts" % self.package_folder) + os.pathsep + os.environ.get('PATH', '')
            pythonExecutable = "%s/python" % self.package_folder

        modulesFolder = "%s/pip_modules" % self.build_folder
        modulesNames = ''
        for _, _, modulesNames in os.walk(modulesFolder):
            pass

        if modulesNames:
            print("Updating modules...", modulesNames)

            res=''
            try:
                res=subprocess.run([pythonExecutable, "-m", "pip", "install", "--upgrade", "-f", "./", "--no-index" ] + modulesNames,
                               env=os.environ, cwd=modulesFolder, shell=False, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print("Error: {}\n{}".format(e, res))
                sys.exit(1)

            print("Updating results:")
            p = subprocess.run([pythonExecutable, "-m", "pip", "list"], env=os.environ, shell=False, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            print(p.stdout.decode())

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `build_requires = ...` attributes.
    def package_info(self):
        is_linux   = ( self.options.os_version == "linux"   )
        is_windows = ( self.options.os_version == "windows" )
        self.cpp_info.bindirs = ["bin"] if is_linux else ["Scripts"]
        self.cpp_info.includedirs = ["include/python%s" % self.major_minor]
        self.cpp_info.libdirs = ["lib"] if is_linux else ["libs"]
        self.cpp_info.libs = ["libpython%s.so.1.0" % self.major_minor] if is_linux else ["python%s.lib" % self.major_minor_short]

        if is_windows:
            self.buildenv_info.prepend_path("PATH", self.package_folder)
            self.buildenv_info.prepend_path("PATH", "%s\\Scripts" % self.package_folder)
        else:
            self.buildenv_info.prepend_path("PATH", "%s/bin" % self.package_folder)
            self.buildenv_info.prepend_path("LD_LIBRARY_PATH", "%s/lib" % self.package_folder)
            self.buildenv_info.prepend_path("MANPATH", "%s/share/man" % self.package_folder)
            self.buildenv_info.prepend_path("PKG_CONFIG_PATH", "%s/lib/pkgconfig" % self.package_folder)
            self.buildenv_info.prepend_path("XDG_DATA_DIRS", "%s/share" % self.package_folder)
            self.buildenv_info.prepend_path("PYTHONPATH", "%s/lib/python%s/site-packages" % (self.package_folder, self.major_minor))

            files_with_hardcoded_paths = ["bin/2to3-3.10",
                                          "bin/idle3.10",
                                          "bin/pip3",
                                          "bin/pip3.10",
                                          "bin/pydoc3.10",
                                          "bin/python3.10-config",
                                          "enable_python.sh",
                                          "lib/pkgconfig/python-3.10.pc",
                                          "lib/pkgconfig/python-3.10-embed.pc",
                                          "lib/python3.10/config-3.10-x86_64-linux-gnu/config.c",
                                          "lib/python3.10/config-3.10-x86_64-linux-gnu/libpython3.10.a",
                                          "lib/python3.10/config-3.10-x86_64-linux-gnu/Makefile",
                                          "lib/python3.10/config-3.10-x86_64-linux-gnu/python-config.py",
                                          "lib/python3.10/_sysconfigdata__linux_x86_64-linux-gnu.py"]

            build_package_folder = ''
            with open("{}/build_package_folder.txt".format(self.package_folder), 'r') as f:
                build_package_folder = f.readline()

            install_package_folder = os.path.dirname(self.package_folder)
            install_package_folder = os.path.dirname(install_package_folder)

            # This code needs to be executed just once.
            if (build_package_folder != install_package_folder):
                for f in files_with_hardcoded_paths:
                    try:
                        file_to_replace = os.path.join(self.package_folder, f)
                        replace_in_file(self, file_path=file_to_replace, search=build_package_folder, replace=install_package_folder, strict=False)
                    except:
                        print("WARNING: error updating hardcoded paths.")

                with open("{}/build_package_folder.txt".format(self.package_folder), 'w') as f:
                    f.write(install_package_folder)

        self.create_enable_python_script()
