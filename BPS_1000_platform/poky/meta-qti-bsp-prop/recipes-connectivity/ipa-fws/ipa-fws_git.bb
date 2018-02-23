inherit qcommon qlicense

DESCRIPTION = "IPA firmwares"
PR = "r2"

INSANE_SKIP_${PN} = "arch"
FILES_${PN} += "${base_libdir}/firmware/ipa/*"
FILESPATH = "${WORKSPACE}:"
SRC_DIR = "${WORKSPACE}/data/ipa_fws"
S = "${WORKDIR}/ipa_fws"

IPA_FIRMWARE_PATH = "${D}${base_libdir}/firmware/ipa"

EXTRA_OECONF += "${@base_conditional('BASEMACHINE', 'mdm9650', '--enable-target-mdm9650=yes', '', d)}"
EXTRA_OECONF += "${@base_conditional('BASEMACHINE', 'sdxhedgehog', '--enable-target-sdxhedgehog=yes', '', d)}"

do_install() {
   install -d ${IPA_FIRMWARE_PATH}
   install -m 0755 ${S}/ipa_fws.elf -D ${IPA_FIRMWARE_PATH}
}
