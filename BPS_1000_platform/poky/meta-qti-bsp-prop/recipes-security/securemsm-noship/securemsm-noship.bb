inherit autotools qcommon qlicense qprebuilt
DESCRIPTION = "securemsmnoship with QseecomAPI user space library to interact with qseecom driver"
DEPENDS = "virtual/kernel glib-2.0 linux-libc-headers securemsm-initial time-services"

SRC_DIR = "${WORKSPACE}/security/securemsm-noship/"
S = "${WORKDIR}/security/securemsm-noship/"

PR = "1"

INSANE_SKIP_${PN} = "dev-so"
INSANE_SKIP_${PN} += "dev-deps"
INSANE_SKIP_${PN} += "debug-files"

PREBUILT = "1"

EXTRA_OEMAKE += "ARCH=${TARGET_ARCH} CROSS_COMPILE=${TARGET_PREFIX}"

EXTRA_OECONF += "--with-kernel=${STAGING_KERNEL_DIR} \
                --with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include"
FILES_${PN} += "/usr/bin/*"
FILES_${PN} += "${bindir}/*"

FILES_${PN} += "${libdir}"

do_install() {
   mkdir -p ${D}/usr/bin
   cp -pPr ${S}${WORKDIR}/image/usr/bin/hdcp* ${D}/usr/bin
   }