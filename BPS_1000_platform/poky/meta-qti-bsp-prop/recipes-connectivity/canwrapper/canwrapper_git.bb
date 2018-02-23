inherit autotools qcommon qlicense qprebuilt

DESCRIPTION = "CAN Wrapper "
PR = "r1"

SRC_DIR = "${WORKSPACE}/vnw-noship/CanWrapper/"
DEPENDS = "glib-2.0 qmi-framework canwrapper-hdr"
EXTRA_OECONF ="--with-glib \
              --with-locflp-includes=${STAGING_INCDIR}/canwrapper-hdr"

S = "${WORKDIR}/vnw-noship/CanWrapper"
