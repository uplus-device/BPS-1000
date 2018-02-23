inherit autotools qcommon qlicense

DESCRIPTION = "libsdmextension Library"
PR = "r3"

PACKAGES = "${PN}"

SRC_DIR = "${WORKSPACE}/display/display-noship/"
S = "${WORKDIR}/display/display-noship/"

DEPENDS += "display-hal-linux"

EXTRA_OECONF = " --with-core-includes=${WORKSPACE}/system/core/include"
EXTRA_OECONF += " --with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include"

CPPFLAGS += "-DTARGET_HEADLESS"

# Need the display sdm headers
CPPFLAGS += "-I${STAGING_INCDIR}/sdm"

FILES_${PN} = "${libdir}/*.so"
