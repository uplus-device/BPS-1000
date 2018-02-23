inherit qcommon qprebuilt qlicense

DESCRIPTION = "Audio Calibration Library"
PR = "r7"

include ${PN}-${BASEMACHINE}.inc

EXTRA_OECONF += "--with-kernel=${STAGING_KERNEL_DIR} \
                 --with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include \
                 --with-glib"

INSANE_SKIP_${PN} = "dev-deps"
