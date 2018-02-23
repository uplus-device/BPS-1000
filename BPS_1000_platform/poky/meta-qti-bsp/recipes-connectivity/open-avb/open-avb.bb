DESCRIPTION = "open-avb tools"
HOMEPAGE = "https://github.com/AVnu/Open-AVB"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://examples/LICENSE;md5=81ccd62d4bc28bafc5e1a2576536b927 \
		    file://daemons/LICENSE;md5=81ccd62d4bc28bafc5e1a2576536b927 \
		    file://lib/avtp_pipeline/LICENSE;md5=8f7b370a91d698ed80d2d20e8e01fbb6"
PR = "r0"

DEPENDS += "alsa-lib alsa-intf libpcap pciutils cmake-native glib-2.0"

FILESPATH =+ "${WORKSPACE}:"
SRC_URI = "file://external/open-avb/"

S = "${WORKDIR}/external/open-avb/"

do_compile() {
    export AVB_FEATURE_NEUTRINO=1
    mkdir -p ${S}/daemons/maap/build
    oe_runmake daemons_all
    make avtp_pipeline
}

do_install() {
    mkdir -p ${D}/${bindir}/
    mkdir -p ${D}/${bindir}/avb/
    install ${S}/daemons/maap/linux/maap_daemon ${D}/${bindir}/avb
    install ${S}/daemons/mrpd/mrpd ${D}/${bindir}/avb
    install ${S}/daemons/mrpd/mrpctl ${D}/${bindir}/avb
    install ${S}/daemons/gptp/linux/build/obj/daemon_cl ${D}/${bindir}/avb
    install ${S}/lib/avtp_pipeline/build/bin/* ${D}/${bindir}/avb
}

FILES_${PN} =+ "${bindir}/avb/*"

FILES_${PN}-dbg += "${bindir}/avb/.debug/*"

