inherit androidmk deploy
inherit qlicense qprebuilt

FILESPATH =+ "${WORKSPACE}:"
SRC_URI = "file://video/lib/mm-video-noship/"
SRC_URI += "file://0001-enable-gpu-stats.patch"
SRC_URI += "file://fpv.cfg"
SRC_DIR = "${WORKSPACE}/video/lib/mm-video-noship"

SRCREV = "${AUTOREV}"
S      = "${WORKDIR}/video/lib/mm-video-noship"

DEPENDS += "adreno200"
DEPENDS += "libnl"
CFLAGS += "-I${STAGING_INCDIR}/adreno/"
CFLAGS += "-I${STAGING_KERNEL_DIR}/usr/include"
CFLAGS += "-I${STAGING_INCDIR}/utils/"
CFLAGS += "-std=c++11"
CFLAGS += "-Wno-missing-field-initializers"
CFLAGS += "-D__ANDROID__"
CFLAGS += "-I${WORKSPACE}/hardware/qcom/media/mm-core/inc/"

CXXFLAGS += "-I ${STAGING_INCDIR}/libnl3"
CFLAGS += "-I ${STAGING_INCDIR}/libnl3"
CFLAGS_armv7a-vfp += "-mfloat-abi=softfp -mfpu=neon"

EXTRA_OEMAKE += "LINUX_OMX_TEST_ONLY_ENCODE=true"
EXTRA_OEMAKE += "LOCAL_CLANG=false"
EXTRA_OEMAKE += "LINUX_FPV_RAVE_ENABLED=true"

LDFLAGS += "-llog"
LDFLAGS += "-lcutils"

FILES_${PN} += "${sysconfdir}/fpv.cfg \
                ${includedir}/fpv_rave/fpv_config.hpp \
                ${includedir}/fpv_rave/fpv_dbg.hpp \
                ${includedir}/fpv_rave/fpv_queue.hpp \
                ${includedir}/fpv_rave/fpv_ra.hpp \
                ${includedir}/fpv_rave/fpv_rave.hpp \
                ${includedir}/fpv_rave/fpv_utils.hpp"

do_compile() {
        androidmk_setenv
        oe_runmake -f ${LA_COMPAT_DIR}/build/core/main.mk BUILD_MODULES_IN_PATHS=${S} \
            all_modules SHOW_COMMANDS=true || die "make failed"
}

do_install_append() {
install -d ${D}${includedir}/libgpustats
install -m 0644 ${S}/gpustats/gpustats.h -D ${D}${includedir}/libgpustats/gpustats.h

# install FPV config file
dest=/etc
install -d ${D}${dest}
install -m 755 ${WORKDIR}/fpv.cfg ${D}${dest}

# install FPV header file
install -d ${D}${includedir}/fpv_rave
cp -r ${S}/fpv_rave/inc/* ${D}${includedir}/fpv_rave/
}

do_install_append_apq8053(){
# install VQZIP
install -d ${D}${includedir}/libvqzip
install -m 0644 ${S}/vqzip/VQZip.h -D ${D}${includedir}/libvqzip/VQZip.h
}

