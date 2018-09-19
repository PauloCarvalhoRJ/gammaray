#This is the configuration that must be shared amongst the .pro files
#used to build the different binaries (executables, libraries, ...)

CONFIG( release, debug|release ) {
	DESTDIR = ../GammaRay_release/dist
	OBJECTS_DIR = ../GammaRay_release/obj
	MOC_DIR = ../GammaRay_release/moc
	RCC_DIR = ../GammaRay_release/rcc
	UI_DIR = ../GammaRay_release/ui
} else {
	DESTDIR = ../GammaRay_debug/dist
	OBJECTS_DIR = ../GammaRay_debug/obj
	MOC_DIR = ../GammaRay_debug/moc
	RCC_DIR = ../GammaRay_debug/rcc
	UI_DIR = ../GammaRay_debug/ui
}
CONFIG += c++11
