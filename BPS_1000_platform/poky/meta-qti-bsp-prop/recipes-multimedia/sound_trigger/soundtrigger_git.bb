inherit qcommon qprebuilt qlicense

DESCRIPTION = "sound trigger loader Library"
PR = "r9"

SRC_DIR = "${WORKSPACE}/audio/mm-audio/sound_trigger_hal/"
S = "${WORKDIR}/audio/mm-audio/sound_trigger_hal/"

do_fetch_extra () {
  cp -rf ${WORKSPACE}/poky/meta-qti-bsp-prop/recipes-multimedia/sound_trigger/mixers/ ${WORKDIR}
}
addtask do_fetch_extra before do_fetch

DEPENDS = "tinyalsa expat libcutils tinycompress audiohal libhardware acdbloader graphite-client"

EXTRA_OECONF += "--with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include"
EXTRA_OECONF += "--with-glib"
EXTRA_OECONF += "BOARD_SUPPORTS_SOUND_TRIGGER_TRANSITION=false"
EXTRA_OECONF += "BUILD_APP=false"
EXTRA_OECONF += "BOARD_SUPPORTS_QSTHW_API=true"

EXTRA_OEMAKE = "DEFAULT_INCLUDES= CPPFLAGS="-I. -I${STAGING_KERNEL_BUILDDIR}/usr/include -I${STAGING_INCDIR}/graphite-client/gcs -I${STAGING_INCDIR}/graphite-client/osal""

do_install_append () {
  cd ${D}/${libdir}/ &&  ln -s sound_trigger_primary_default.so sound_trigger.primary.default.so

  if [ -d "${WORKDIR}/mixers/${BASEMACHINE}" ] && [ $(ls -1 ${WORKDIR}/mixers/${BASEMACHINE} | wc -l) -ne 0 ]; then
    install -d ${D}${sysconfdir}
    install -m 0755 ${WORKDIR}/mixers/${BASEMACHINE}/* ${D}${sysconfdir}/
  fi
}

FILES_${PN}     += "${libdir}/*.so"
INSANE_SKIP_${PN} = "dev-so"
