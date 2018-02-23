FILESEXTRAPATHS_prepend := "${THISDIR}/kernel:"
SRC_URI += "file://qti.key"
TARGET_SHA_TYPE = "sha256"

do_deploy_append() {
    #Generating signed boot.img
    cd ${DEPLOY_DIR_IMAGE}
    cp ${MACHINE}-boot.img ${MACHINE}-boot.img.nonsecure

    openssl dgst -${TARGET_SHA_TYPE} -binary ${MACHINE}-boot.img.nonsecure > ${MACHINE}-boot.img.${TARGET_SHA_TYPE}
    openssl rsautl -sign -in ${MACHINE}-boot.img.${TARGET_SHA_TYPE} -inkey ${WORKDIR}/qti.key -out ${MACHINE}-boot.img.sig
    dd if=/dev/zero of=${MACHINE}-boot.img.sig.padded bs=2048 count=1
    dd if=${MACHINE}-boot.img.sig of=${MACHINE}-boot.img.sig.padded conv=notrunc
    cat ${MACHINE}-boot.img.nonsecure ${MACHINE}-boot.img.sig.padded > ${MACHINE}-boot.img.secure
    rm -rf ${MACHINE}-boot.img.${TARGET_SHA_TYPE} ${MACHINE}-boot.img.sig ${MACHINE}-boot.img.sig.padded
    mv -f ${MACHINE}-boot.img.secure ${MACHINE}-boot.img

    if [ -s ${DEPLOY_DIR_IMAGE}/${MACHINE}-boot-2K.img ]; then
       cp ${MACHINE}-boot-2K.img ${MACHINE}-boot-2K.img.nonsecure

       openssl dgst -${TARGET_SHA_TYPE} -binary ${MACHINE}-boot-2K.img.nonsecure > ${MACHINE}-boot-2K.img.${TARGET_SHA_TYPE}
       openssl rsautl -sign -in ${MACHINE}-boot-2K.img.${TARGET_SHA_TYPE} -inkey ${WORKDIR}/qti.key -out ${MACHINE}-boot-2K.img.sig
       dd if=/dev/zero of=${MACHINE}-boot-2K.img.sig.padded bs=2048 count=1
       dd if=${MACHINE}-boot-2K.img.sig of=${MACHINE}-boot-2K.img.sig.padded conv=notrunc
       cat ${MACHINE}-boot-2K.img.nonsecure ${MACHINE}-boot-2K.img.sig.padded > ${MACHINE}-boot-2K.img.secure
       rm -rf ${MACHINE}-boot-2K.img.${TARGET_SHA_TYPE} ${MACHINE}-boot-2K.img.sig ${MACHINE}-boot-2K.img.sig.padded
       mv -f ${MACHINE}-boot-2K.img.secure ${MACHINE}-boot-2K.img
    fi
}
