inherit qcommon qprebuilt qlicense

DESCRIPTION = "Audio Calibration Library"
PR = "r11"

TARGETNAME = "${@d.getVar('BASEMACHINE', True).replace('mdm','').replace('apq','').replace('sdx','')}"
SRC_DIR = "${WORKSPACE}/audio/mm-audio/audcal/"

S = "${WORKDIR}/audio/mm-audio/audcal/"

DEPENDS = "glib-2.0 diag common acdbmapper"

EXTRA_OECONF += "--with-kernel=${STAGING_KERNEL_DIR} \
                 --with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include \
                 --with-glib \
                 --enable-target=${BASEMACHINE}"

do_install_append() {
    if [ -d ${S}/family-b/acdbdata/${TARGETNAME}/MTP/ ];then
        mkdir -p ${D}${sysconfdir}/
        install -m 0755 ${S}family-b/acdbdata/${TARGETNAME}/MTP/*  -D ${D}${sysconfdir}
    fi
}

do_install_append_apq8053() {
    mkdir -p -m 0755 ${D}${sysconfdir}/acdbdata/MTP
    cp -r ${S}family-b/acdbdata/8953/MTP/* ${D}${sysconfdir}/acdbdata/MTP/
}

do_install_append_apq8098() {
    mkdir -p -m 0755 ${D}${sysconfdir}/acdbdata/MTP
    cp -r ${S}family-b/acdbdata/8998/MTP/* ${D}${sysconfdir}/acdbdata/MTP/
}
