inherit androidmk qlicense qprebuilt

DESCRIPTION = "Generate libfastcvopt"

FILESPATH =+ "${WORKSPACE}/cv/:"
SRC_URI = "file://fastcv-noship/"


SRC_DIR  = "${WORKSPACE}/cv/fastcv-noship/"
SRCREV = "${AUTOREV}"
S      = "${WORKDIR}/fastcv-noship"


def get_depends(bb, d):
    srcdir = d.getVar('SRC_DIR', True)
    if (os.path.isdir(srcdir)):
        return "system-core llvm-arm-toolchain-native"
    else:
        return "system-core"

DEPENDS += "${@get_depends(bb, d)}"
DEPENDS += "adsprpc liblog adreno200"

EXTRA_OEMAKE += " PATH_FOR_LLVM=${STAGING_BINDIR_NATIVE}/llvm-arm-toolchain/bin"
EXTRA_OEMAKE += " SYSROOT_64=${STAGING_DIR_TARGET}"
EXTRA_OEMAKE += " SYSROOT_32=${STAGING_DIR_TARGET}"
EXTRA_OEMAKE += " TARGET_TRIPLE_64=${TARGET_SYS}"
EXTRA_OEMAKE += " TARGET_TRIPLE_32=${TARGET_SYS}"
EXTRA_OEMAKE += " FASTCV_NOSHIP_PATH=${S}"
EXTRA_OEMAKE += " LE_ENABLE="true""

#Uncomment this to use MDSP on LE for fastCV call
#EXTRA_OEMAKE += " MDSP_ENABLE="true""

CFLAGS += " -I${STAGING_INCDIR}"
CFLAGS += " -I${STAGING_INCDIR}/cutils"
CFLAGS += " -I${STAGING_INCDIR}/glib-2.0"
CFLAGS += " -I${STAGING_LIBDIR}/glib-2.0/include"

CFLAGS += " -include unistd.h"
CFLAGS += " -include sys/time.h"
CFLAGS += " -Wno-error"
CFLAGS += " -DLE_ENABLE"


CFLAGS += "-include stdint.h"
CFLAGS += "-Dstrlcpy=g_strlcpy"
CFLAGS += "-Dstrlcat=g_strlcat"
CFLAGS += "-include stdlib.h"
CFLAGS += "-include glib.h"
CFLAGS += "-include glibconfig.h"
CFLAGS += " -include properties.h"

LDFLAGS += " -L${STAGING_BASELIBDIR} -L${STAGING_LIBDIR} -L${STAGING_DIR_TARGET}"
LDFLAGS += " -lcutils"
LDFLAGS += " -lglib-2.0"
LDFLAGS += " -llog"

# 64-bit fastcv uses qcld linker instead of GCC linker
# (i.e. LDFLAGS += -fuse-ld=qcld added in makefile)
# As qcld don't expect any optimization flags, remove
# the default linker flags passed on from bitbake.conf
TARGET_LDFLAGS_aarch64 = ""

#Uncomment this to use MDSP on LE for fastCV calls
#LDFLAGS += " -lmdsprpc"
#Comment this to use MDSP on LE for fastCV calls
LDFLAGS += " -ladsprpc"

#Uncomment this to use MDSP on LE for fastCV call
#export TARGET_LIBRARY_SUPPRESS_LIST="libmdsprpc"
#Comment this to use MDSP on LE for fastCV call
export TARGET_LIBRARY_SUPPRESS_LIST="libadsprpc"

do_compile() {
    androidmk_setenv
    export TARGET_OUT_VENDOR=${D}/usr
    oe_runmake -f ${LA_COMPAT_DIR}/build/core/main.mk BUILD_MODULES_IN_PATHS=${S} \
        all_modules SHOW_COMMANDS=true || die "make failed"
}

do_install() {
    androidmk_setenv
    export TARGET_OUT_VENDOR=${D}/usr
    export TARGET_OUT_HEADERS=${D}${includedir}
    oe_runmake -f ${LA_COMPAT_DIR}/build/core/main.mk BUILD_MODULES_IN_PATHS=${S} \
        all_modules SHOW_COMMANDS=true USER_INSTALL=true || die "make failed"
}
PACKAGES = "${PN}"
INSANE_SKIP_${PN} = "arch"
INSANE_SKIP_${PN} += "dev-deps"

FILES_${PN} += "/lib/* /lib64/* "
