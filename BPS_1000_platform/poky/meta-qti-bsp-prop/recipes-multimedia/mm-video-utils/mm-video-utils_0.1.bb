inherit androidmk qlicense qprebuilt

SUMMARY = "video utils"
SECTION = "multimedia"

FILESPATH =+ "${WORKSPACE}:"
SRC_URI = "file://mm-video-utils/"

SRC_DIR = "${WORKSPACE}/mm-video-utils"

SRCREV = "${AUTOREV}"
S      = "${WORKDIR}/mm-video-utils"

DEPENDS += "adreno200"
DEPENDS += "libhardware"
DEPENDS += "native-frameworks"
DEPENDS += "media"
DEPENDS += "libxml2"
DEPENDS += "system-core"
DEPENDS += "display-hal"

EXTRA_OEMAKE += "BOARD_USES_ADRENO=false"
EXTRA_OEMAKE += "VDEC_ENABLE=false"
EXTRA_OEMAKE += "LINUX_OMX_TEST_ONLY_ENCODE=true"

CFLAGS += "-Dstrlcpy=g_strlcpy"
CFLAGS += "-Dstrlcat=g_strlcat"
CFLAGS += "-DNON_BIONIC"
CFLAGS += "-include ${STAGING_INCDIR}/asm-generic/errno-base.h"
CFLAGS += "-I${STAGING_INCDIR}/cutils/"
CFLAGS += "-I${STAGING_INCDIR}/adreno/"
CFLAGS += "-I${STAGING_INCDIR}/ui/"
CFLAGS += "-I${STAGING_INCDIR}"
CFLAGS += "-I${STAGING_INCDIR}/glib-2.0"
CFLAGS += "-I${STAGING_LIBDIR}/glib-2.0/include"
CFLAGS += "-I${STAGING_KERNEL_DIR}/usr/include"
CFLAGS += "-I${STAGING_INCDIR}/libgralloc/"
CFLAGS += "-I${STAGING_INCDIR}/utils/"
CFLAGS += "-I${STAGING_INCDIR}/binder/"
CFLAGS += "-I${STAGING_INCDIR}/mm-core/"
CFLAGS += "-I${STAGING_INCDIR}/libxml2/"
CFLAGS += "-I${STAGING_INCDIR}/system/"
CFLAGS += "-I${STAGING_INCDIR}/mm-video-v4l2/vidc/"
CFLAGS += "-I${STAGING_INCDIR}/libstagefrighthw/"
CFLAGS += "-I${STAGING_INCDIR}/libqdutils/"
CFLAGS += "-I${STAGING_INCDIR}/media/hardware"
CFLAGS += "-I${WORKSPACE}/hardware/qcom/media/mm-video-v4l2/vidc/"

CFLAGS += "-include stdint.h"
CFLAGS += "-Dstrlcpy=g_strlcpy"
CFLAGS += "-Dstrlcat=g_strlcat"
CFLAGS += "-include stdlib.h"

#TODO: remove the -D defines and get proper solution later
CFLAGS += "-Dchar16_t=uint16_t"
CFLAGS += "-Dchar32_t=uint32_t"

CFLAGS += "-include glib.h"
CFLAGS += "-include glibconfig.h"
CFLAGS += "-include sys/ioctl.h"
CFLAGS += "-include time.h"
CFLAGS += "-include assert.h"

CFLAGS += "-DPTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP"
CFLAGS += "-D__need_timespec"
CFLAGS += "-D_POSIX_C_SOURCE=199309"
CFLAGS += "-DLINUX_OMX_TEST_ONLY_ENCODE"

LDFLAGS += "-lcutils"
LDFLAGS += "-lglib-2.0"
LDFLAGS += "-llog"
LDFLAGS += "-lbase"
LDFLAGS += "-lutils"
LDFLAGS += "-lbinder"
LDFLAGS += "-lxml2"

export TARGET_LIBRARY_SUPPRESS_LIST="libui libgui libbinder libxml2"

do_compile() {
    # Current support is limited to 32-bit build
    #
    if [ "${MLPREFIX}" == "lib32-" ] || [ "${MLPREFIX}" == "" -a "${TUNE_ARCH}" == "arm" ]; then
        androidmk_setenv
        oe_runmake -f ${LA_COMPAT_DIR}/build/core/main.mk BUILD_MODULES_IN_PATHS=${S} \
            all_modules SHOW_COMMANDS=true || die "make failed"
    else
        die "mm-video-utils supports only 32-bit build."
    fi
}
