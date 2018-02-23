inherit qcommon

DESCRIPTION = "ALSA Framework Library"
LICENSE = "Apache-2.0"
PR = "r4"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"
DEPENDS = "acdbloader glib-2.0"

SRC_DIR = "${WORKSPACE}/qcom-opensource/mm-audio"
prefix="/etc"

S = "${WORKDIR}/mm-audio"

EXTRA_OECONF += "--prefix=/etc \
                 --with-kernel=${STAGING_KERNEL_DIR} \
                 --with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include \
                 --with-glib \
                 --with-acdb"

FILES_${PN} += "${prefix}/snd_soc_msm/*"
