#!/bin/sh
###############################################################################
#
# This script is used for administration of the Hexagon DSP
#
# Copyright (c) 2012-2016 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################

KEEP_ALIVE=0

# Wait for adsp.mdt to show up
while [ ! -f /firmware/image/adsp.mdt ]; do
  usleep 10000
done

if [ "$KEEP_ALIVE" = "1" ]; then
  subsys_name=""
  for subsys in `ls /sys/bus/msm_subsys/devices`; do
    name=`cat /sys/bus/msm_subsys/devices/${subsys}/name`
    if [ "${name}" = "adsp" ]; then
      subsys_name="${subsys}"
      break
    fi
  done

  if [ -n "${subsys_name}" ]; then
    sysctl -w kernel.panic=0
    echo 1 > /sys/bus/msm_subsys/devices/${subsys_name}/keep_alive
  else
    echo "[ERROR] Could not find ADSP subsystem"
  fi
fi

# FIXME: See ATL-3054
echo 1 > /sys/module/subsystem_restart/parameters/enable_debug
# Bring adsp out of reset
echo "[INFO] Bringing ADSP out of reset"
echo 1 > /sys/kernel/boot_adsp/boot

# sleep 10s to wait boot finished
sleep 10

# Emit adsp
initctl emit adsp
