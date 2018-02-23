inherit qcommon qprebuilt qlicense

DESCRIPTION = "Audio FTM"
PR = "r5"
DEPENDS = "common glib-2.0 alsa-intf"

SRC_DIR = "${WORKSPACE}/audio/mm-audio/audio_ftm/"
S = "${WORKDIR}/audio/mm-audio/audio_ftm/"

DEPENDS = "tinyalsa alsa-intf glib-2.0"

EXTRA_OECONF += "--with-kernel=${STAGING_KERNEL_DIR} \
                 --with-common-includes=${STAGING_INCDIR} \
                 --with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include \
                 --with-glib"

do_install_append_mdm9607() {
    mkdir -p ${D}${sysconfdir}/
    install -m 0755 ${S}/legacy_intf/config/9607/ftm_test_config*  -D ${D}${sysconfdir}
}

do_install_append_apq8053() {
    mkdir -p ${D}${sysconfdir}/
    install -m 0755 ${S}/family-b/config/8953/ftm_test_config  -D ${D}${sysconfdir}
}

FILES_${PN}-dbg  = "${libdir}/.debug/* ${bindir}/.debug/*"
FILES_${PN}      = "${libdir}/*.so ${libdir}/*.so.* ${sysconfdir}/* ${bindir}/* ${libdir}/pkgconfig/*"
FILES_${PN}-dev  = "${libdir}/*.la ${includedir}"
