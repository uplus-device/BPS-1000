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

# Wait for slpi.mdt to show up
while [ ! -s /firmware/image/slpi.mdt ]; do
  usleep 10000
done

if [ "$KEEP_ALIVE" = "1" ]; then
  subsys_name=""
  for subsys in `ls /sys/bus/msm_subsys/devices`; do
    name=`cat /sys/bus/msm_subsys/devices/${subsys}/name`
    if [ "${name}" = "slpi" ]; then
      subsys_name="${subsys}"
      break
    fi
  done

  if [ -n "${subsys_name}" ]; then
    sysctl -w kernel.panic=0
    echo 1 > /sys/bus/msm_subsys/devices/${subsys_name}/keep_alive
  else
    echo "[ERROR] Could not find SLPI subsystem"
  fi
fi

# FIXME: See ATL-3054
echo 1 > /sys/module/subsystem_restart/parameters/enable_debug
#FIXME: ATL-6032:Enable SSR ramdump by default,ATL-3054,ATL-2820
#TODO:once excelsior goes production builds.check if this is required
echo 1 > /sys/module/subsystem_restart/parameters/enable_ramdump
# Bring slpi out of reset
echo "[INFO] Bringing slpi out of reset"
echo 1 > /sys/kernel/boot_slpi/boot

# sleep 10s to wait boot finished
sleep 10

# Emit slpi
initctl emit slpi
