from conan import ConanFile
from conan.tools.files import unzip, copy
import os

# This recipe prepares Eigen for being used as a Conan package by other packages.  Eigen is a header template library,
# so it is not built like a library or executable.  Notice that this recipe doesn't declare a ConanFile.settings attribute,
# meaning there's no need to manage this dependency by operating system, compiler, architecture and build type (Debug, 
# Release, etc.).  Header-only libraries are, however, still managed by their names and versions, of course.

# All recipes are classes derived from the abstract class ConanFile that implement its "virtual" methods.
class EigenConan(ConanFile):
    # This is the name that will be used to locate the package/dependency in Conan repository.
    name = "eigen3"
    # Sets the version of this package/dependency and is used by Conan to manage packages accurately.
    version = "3.3.4"
    # Declare where the content was obtained from.
    url = "http://bitbucket.org/eigen/eigen"
    # Declare the home page of the dependency's project.
    homepage = "http://eigen.tuxfamily.org"
    # A brief description of what this recipe does.
    description = "Eigen is a C++ template library for linear algebra: matrices, vectors, \
                   numerical solvers, and related algorithms."
    # Declare the license of the dependency.
    license = "Mozilla Public License Version 2.0"
    # Do not copy the source files to the build directory in order to build. This speeds up build time by 
    # avoiding unnecessary copies but the you HAVE to make sure no configure or build script/process change any 
    # of the source files (e.g. apply a patch).
    no_copy_source = True
    # Declare where the tarball with the dependency's source code was downloaded from.
    source_url = "http://bitbucket.org/eigen/eigen/get/3.3.4.tar.gz"
    # A list of files that should be exported (copied) alongside the recipe (conanfile.py) so the recipe can work.
    # For example, other Python scripts that the recipe may depend on.
    # In this case, the .cmake files must follow the recipe so they are installed and made available for CMake find_package()es.
    exports = ["Eigen3Config.cmake", "Eigen3ConfigVersion.cmake", "Eigen3Targets.cmake", "UserEigen3.cmake"]
    # A list of files that should be exported (copied) from here (the Git repository) to the source directory in
    # Conan's workspace.
    exports_sources = ["eigen-{}.tar.xz".format(version)]
    # Declares this dependeny's dependencies needed for building.
    # In this case, the CMake binaries that must exist in CMake package (named "cmake_installer" in its recipe).
    build_requires = "cmake_installer/3.29.3"

    #---------------------------------------------Class/non-Conan API------------------------------------------------

    # Returns the name of the bzip source sub-folder varying with the bzip2 version.
    @property
    def _source_subfolder(self):
        return "eigen-{}".format(self.version)

    # Returns the path to the tarball containing the source code of bzip2.
    @property
    def _source_zip_filename(self):
        return "{}.tar.xz".format(self._source_subfolder)

    #------------------------------------------Conan API methods/callbacks-------------------------------------------

    # Performs the actions necessary to obtain the source files.
    # In this case, as the files are already in the source repository, this only
    # unzips the source files into the source directory in Conan's workspace.
    def source(self):
        unzip(self, filename=os.path.join(self.source_folder, self._source_zip_filename), keep_permissions=True)
        self.output.info("Source code was exported (copied) with the recipe.")
        self.output.info("From here: {}".format(self.source_url))
   
    # Copies the wanted build artifacts (e.g. shared libraries) from the build directory into the 
    # package directory so they can be used by other packages that are dependant of this one.
    # Running cmake.install() is often a shortcut to copy all the necessary files (e.g. headers and libraries).
    # In this case, since Eigen is a header-only library, we copy to the package directory (deployment) only the files
    # necessary for the other packages to use.
    def package(self):
        unsupported_folder = os.path.join(self.package_folder, "include", "eigen3", "unsupported", "Eigen")
        eigen_folder = os.path.join(self.package_folder, "include", "eigen3", "Eigen")
        target_cmake_folder = os.path.join(self.package_folder, "share", "eigen3", "cmake")
        copy(self, pattern="COPYING.*",                          dst="licenses",                        src=os.path.join(self.source_folder, self._source_subfolder))
        copy(self, pattern="*",                                  dst=eigen_folder,                      src=os.path.join(self.source_folder, self._source_subfolder, "Eigen"))
        copy(self, pattern="*",                                  dst=unsupported_folder,                src=os.path.join(self.source_folder, self._source_subfolder, "unsupported", "Eigen"))
        copy(self, pattern="signature_of_eigen3_matrix_library", dst=os.path.join("include", "eigen3"), src=os.path.join(self.source_folder, self._source_subfolder))
        copy(self, pattern="*.cmake",                            dst=target_cmake_folder,               src=os.path.join(self.source_folder, self._source_subfolder))
        os.remove(os.path.join(eigen_folder, "CMakeLists.txt"))
        os.remove(os.path.join(unsupported_folder, "CMakeLists.txt"))
        os.remove(os.path.join(unsupported_folder, "CXX11", "CMakeLists.txt"))
        os.remove(os.path.join(unsupported_folder, "CXX11", "src", "Tensor", "README.md"))
        os.remove(os.path.join(unsupported_folder, "src", "EulerAngles", "CMakeLists.txt"))
        new_licenses_dir = os.path.join(self.package_folder, "licenses")
        if not os.path.exists( new_licenses_dir ):
            os.makedirs( new_licenses_dir )
        os.rename(os.path.join(unsupported_folder, "src", "LevenbergMarquardt", "CopyrightMINPACK.txt"),
                  os.path.join( new_licenses_dir, "CopyrightMINPACK.txt"))

    # Allows to customize the package's default binary identification (ConanFile.package_id).
    # This is often used to relax some settings related to binary compatibility. For example, 
    # header-only libraries can have a single id since they don't have binaries.  This is the
    # case of the Eigen library.  Of course, Eigen libraries are still told apart by their version.
    def package_id(self):
        self.info.clear() #settings, options and requirements are removed from id calculation.

    # This is called when running recipes for packages which are dependant of this one.
    # E.g. In their `requires = ...` or `build_requires = ...` attributes.
    def package_info(self):
        self.cpp_info.name = "eigen3"
        #self.cpp_info.includedirs = ['include', 'include/eigen3', 'include/unsupported',
        #                             'include/eigen3/unsupported' ]
        self.cpp_info.includedirs = ['include', 'include/eigen3', 'include/eigen3/unsupported' ]                                     