inherit qcommon qlicense autotools update-rc.d qprebuilt

DESCRIPTION = "CSD QMI Server"
DEPENDS = "glib-2.0 qmi-framework acdbloader alsa-intf"

SRC_DIR = "${WORKSPACE}/audio/mm-audio/audio-qmi/csd-server/"
S = "${WORKDIR}/audio/mm-audio/audio-qmi/csd-server/"

PR = "r4"

EXTRA_OECONF += "--with-kernel=${STAGING_KERNEL_DIR} \
                 --with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include \
                 --with-glib \
                 --enable-target=${BASEMACHINE}"

INITSCRIPT_NAME = "csdserver"
INITSCRIPT_PARAMS = "start 45 2 3 4 5 . stop 80 0 1 6 ."
