inherit androidmk qlicense qprebuilt

SUMMARY = "Display NoShip"

SRCREV = "${AUTOREV}"

FILESPATH =+ "${WORKSPACE}/display:"
SRC_URI   = "file://display-noship/"

S = "${WORKDIR}/display-noship"
PR = "r1"

SRC_DIR = "${WORKSPACE}/display/display-noship"

DEPENDS += "system-core"
DEPENDS += "libhardware"
DEPENDS += "display-hal"

export TARGET_LIBRARY_SUPPRESS_LIST="libbacktrace libbase libdl libunwind libhardware libbinder"

EXTRA_OEMAKE += "TARGET_IS_HEADLESS=true"
EXTRA_OEMAKE += "QTI_LINUX_DISPLAY_HAL=true"
EXTRA_OECONF = " --with-core-includes=${WORKSPACE}/system/core/include --with-glib"

CFLAGS += "-Wno-error=unused-parameter"
CFLAGS += "-Wno-error=conversion"
CFLAGS += "-Wno-uninitialized -Wno-error=attributes"
CFLAGS += "-I${STAGING_INCDIR}/system"
CFLAGS += "-I${WORKSPACE}/display/display-hal/sdm/include/"
CFLAGS += "-I${WORKSPACE}/display/display-noship/scalar/scalar_v1/"
CFLAGS += "-I${WORKSPACE}/display/display-noship/scalar/qseed3/inc/"
CFLAGS += "-I${WORKSPACE}/display/display-hal/sdm/libs/"
CFLAGS += "-I${WORKSPACE}/display/display-hal/include"

CFLAGS += "-I${STAGING_INCDIR}"
CFLAGS += "-DPAGE_SIZE=PAGE_SZ"
CFLAGS += "-DAID_CAMERASERVER=1047"

LDFLAGS += "-llog -lbinder -lutils -lcutils -lsdmutils"

do_compile() {
        androidmk_setenv
        oe_runmake -f ${LA_COMPAT_DIR}/build/core/main.mk BUILD_MODULES_IN_PATHS=${S} \
            all_modules SHOW_COMMANDS=true || die "make failed"
}

do_install() {
        androidmk_setenv
        export TARGET_OUT_HEADERS=${D}${includedir}
        export TARGET_OUT_VENDOR_SHARED_LIBRARIES=${D}${libdir}
        export TARGET_OUT_SHARED_LIBRARIES=${D}${libdir}
        export TARGET_OUT_EXECUTABLES=${D}${bindir}
        export TARGET_OUT_DATA=${D}/data
        oe_runmake -f ${LA_COMPAT_DIR}/build/core/main.mk BUILD_MODULES_IN_PATHS=${S} \
            all_modules SHOW_COMMANDS=true || die "make failed"
}

PACKAGES = "${PN}"

FILES_${PN} +="/usr/* /data/* "
