inherit qcommon qprebuilt qlicense

DESCRIPTION = "audio-parsers Library"
PR = "r0"

SRC_DIR = "${WORKSPACE}/audio/mm-audio/audio-parsers/"
S = "${WORKDIR}/audio/mm-audio/audio-parsers/"

FILES_${PN}-dbg  = "${libdir}/.debug/*"
FILES_${PN}      = "${libdir}/*.so ${libdir}/*.so.* ${sysconfdir}/* ${libdir}/pkgconfig/*"
FILES_${PN}-dev  = "${libdir}/*.la ${includedir}"
