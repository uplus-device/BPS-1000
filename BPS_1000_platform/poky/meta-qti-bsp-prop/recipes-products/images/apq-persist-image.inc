# This include forces the generation of a persist image file

PERSIST_IMAGE_BASENAME = "${MACHINE}-persist"
OUTPUT_PERSIST_FILE = "${DEPLOY_DIR_IMAGE}/${PERSIST_IMAGE_BASENAME}.ext4"
PERSIST_IMAGE_ROOTFS_SIZE = "6536668"
PERSIST_DATA_FOLDER = "${TMPDIR}/rootfs/${PERSIST_IMAGE_BASENAME}"

# Generating persist image in EXT4 format
IMAGE_PREPROCESS_COMMAND += "${@base_contains('DISTRO_FEATURES', 'persist-image', 'create_persist_ext4_img;', '', d)}"

create_persist_ext4_img() {
   mkdir -p ${PERSIST_DATA_FOLDER}
   mkdir -p ${PERSIST_DATA_FOLDER}/sensors
   touch ${PERSIST_DATA_FOLDER}/sensors/sensors_settings
   echo "1" > ${PERSIST_DATA_FOLDER}/sensors/sensors_settings

   if [ -e "${WORKSPACE}/android_compat/device/qcom/${SOC_FAMILY}/WCNSS_qcom_wlan_nv.bin" ];then
       cp -rf "${WORKSPACE}/android_compat/device/qcom/${SOC_FAMILY}/WCNSS_qcom_wlan_nv.bin" ${PERSIST_DATA_FOLDER}
   fi

   if [ -e "${WORKSPACE}/android_compat/device/qcom/${SOC_FAMILY}/WCNSS_wlan_dictionary.dat" ]; then
       cp -rf "${WORKSPACE}/android_compat/device/qcom/${SOC_FAMILY}/WCNSS_wlan_dictionary.dat" ${PERSIST_DATA_FOLDER}
   elif [ -e "${WORKSPACE}/android_compat/device/qcom/${SOC_FAMILY}_32/WCNSS_wlan_dictionary.dat" ]; then
       cp -rf "${WORKSPACE}/android_compat/device/qcom/${SOC_FAMILY}_32/WCNSS_wlan_dictionary.dat" ${PERSIST_DATA_FOLDER}
   fi

   make_ext4fs -s -l ${PERSIST_IMAGE_ROOTFS_SIZE} ${OUTPUT_PERSIST_FILE} ${PERSIST_DATA_FOLDER}
   chmod 644 ${OUTPUT_PERSIST_FILE}
}
