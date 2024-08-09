#include <vtkVersion.h>
#include <QVTKOpenGLNativeWidget.h>
#include <iostream>

int main() {
    if(vtkVersion::GetVTKMajorVersion() != 9 && vtkVersion::GetVTKMinorVersion() != 3) {
        std::cerr << "VTK version 9.3 was expected. Got " << vtkVersion::GetVTKMajorVersion() << "." 
                  << vtkVersion::GetVTKMinorVersion()  << " instead. Test failed." << std::endl;
        return 1;
    }
    return 0;
}
