inherit autotools qcommon qlicense qprebuilt
inherit autotools pkgconfig

DESCRIPTION = "libgbm Library"
PR = "r2"

SRC_DIR = "${WORKSPACE}/display/libgbm"
S = "${WORKDIR}/libgbm/"

DEPENDS += "virtual/kernel"

EXTRA_OECONF += " --with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include"

do_install_append () {
  install -d ${D}/data/display/libgbm
  install -d                                           ${D}${includedir}
  cp -rf ${S}/inc/gbm.h                         ${D}${includedir}
  cp -rf ${S}/inc/gbm_priv.h                    ${D}${includedir}
  install -d                                           ${D}${libdir}/
  cp -rf ${WORKDIR}/image/usr/lib/libgbm.so            ${D}${libdir}/
  install -d                                           ${D}${libdir}/pkgconfig
  cp -rf ${WORKDIR}/libgbm/gbm.pc  ${D}${libdir}/pkgconfig
}

FILES_${PN} +="/data/* "
