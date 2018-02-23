SUMMARY = "Subsystem Manager Deamons"
LICENSE = "BSD"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} = "\
    init-mss \
    reboot-daemon \
    "
