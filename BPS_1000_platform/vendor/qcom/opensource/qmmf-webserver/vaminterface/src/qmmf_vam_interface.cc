/*
* Copyright (c) 2017, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "qmmf_vam_interface.h"
#include "qmmf_vam_config_parser.h"

#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>

#include <VAM/vaapi.h>
#include <VAM/VAMUtilities.h>

namespace qmmf {

namespace vaminterface {

typedef struct VAMContext_t {
  VAMContext_t() = delete;
  VAMContext_t(bool in_present, bool in_active, uint32_t in_session_id,
               uint32_t in_track_id) :
      present(in_present), active(in_active), session_id(in_session_id),
      track_id(in_track_id) {};

  bool present;
  bool active;
  uint32_t session_id;
  uint32_t track_id;
} VAMContext;

typedef struct VAMPendingBuffer_t {
  VAMPendingBuffer_t() :
      track_id(0), session_id(0), buffer() {};
  VAMPendingBuffer_t(uint32_t in_track_id, uint32_t in_session_id,
                     BufferDescriptor in_buffer) :
      track_id(in_track_id), session_id(in_session_id), buffer(in_buffer) {};

  uint32_t track_id;
  uint32_t session_id;
  BufferDescriptor buffer;
} VAMPendingBuffer;

class VAMInstance : public VAMInterface {
public:
  VAMInstance(VAMCb *vam_cb);
  virtual ~VAMInstance();

private:
  int32_t InitVAM(const uint32_t session_id,
                  const uint32_t track_id) override;
  int32_t CloseVAM() override;

  int32_t StartVAM(CameraBufferMetaData &meta_data) override;
  int32_t VAMEnroll(VAMEnrollmentInfo &enroll_info) override;
  int32_t VAMConfig(const char *json_config) override;
  int32_t VAMRemoveConfig(const char *json_config) override;
  int32_t QueueVAMBuffers(uint32_t track_id, uint32_t session_id,
                          std::vector<BufferDescriptor> &buffers,
                          std::vector<MetaData> &meta_data) override;

private:
  int32_t VAMWaitForInit();
  int32_t VAMFrameProcessed(uint64_t time_stamp);
  static int32_t VAMFrameProcessedCb(uint64_t time_stamp, void *usr_data);
  int32_t VAMEvent(struct vaapi_event *event);
  static int32_t VAMEventCb(struct vaapi_event *event, void* usr_data);
  int32_t VAMMetadata(struct vaapi_metadata_frame *frame);
  static int32_t VAMMetadataCb(struct vaapi_metadata_frame *frame,
                               void* usr_data);
  int32_t RcConvert(int32_t ret_val);

private:
  static const char kVAMDynamicPath[];
  static const char kVAMDataPath[];
  static const nsecs_t kWaitDuration;

  Mutex vam_pending_buffers_lock_;
  Mutex vam_config_context_lock_;
  Condition wait_for_vam_to_init_;

  VAMCb *vam_cb_;

  VAMContext vam_context_;
  KeyedVector<uint64_t, VAMPendingBuffer> vam_pending_buffers_;
  struct vaapi_configuration vam_config_;
};

VAMInterface *VAMInterfaceFactory::NewInstance(VAMCb *vam_cb) {
  VAMInstance *new_instance = new VAMInstance(vam_cb);
  return static_cast<VAMInterface *>(new_instance);
}

const char VAMInstance::kVAMDynamicPath[] = "/usr/lib/vam_engines";
const char VAMInstance::kVAMDataPath[] = "/data/misc/camera";
const nsecs_t VAMInstance::kWaitDuration = 1000000000; // 1 sec.

VAMInstance::VAMInstance(VAMCb *vam_cb) :
    vam_cb_(vam_cb),
    vam_context_(false, false, 0, 0) {
  memset(&vam_config_, 0, sizeof(vam_config_));
}

VAMInstance::~VAMInstance() {
}

int32_t VAMInstance::VAMWaitForInit() {
  Mutex::Autolock l(vam_config_context_lock_);
  if (!vam_context_.active) {
    ALOGW("%s: VAM is not yet active, Wait!", __func__);
    auto ret = wait_for_vam_to_init_.waitRelative(vam_config_context_lock_,
                                                  kWaitDuration);
    if (ret == TIMED_OUT) {
      ALOGE("%s: Timed out! VAM is not initialized!", __func__);
      return NO_INIT;
    }
  }
  ALOGD("%s: VAM is in init state!", __func__);
  return NO_ERROR;
}

int32_t VAMInstance::VAMEnroll(VAMEnrollmentInfo &enroll_info) {
  int32_t ret = VAM_OK;

  if (NULL == enroll_info.data) {
    ALOGE("%s: Enroll data is missing!\n", __func__);
    return BAD_VALUE;
  }

  ret = VAMWaitForInit();
  if (NO_ERROR != ret)
    return ret;

  vaapi_enrollment_info eInfo;
  memset(&eInfo, 0, sizeof(eInfo));

  if (enroll_info.id) {
    strncpy(eInfo.id, enroll_info.id, sizeof(eInfo.id) - 1);
    eInfo.id[sizeof(eInfo.id) - 1] = '\0';
  }
  if (enroll_info.display_name) {
    strncpy(eInfo.display_name, enroll_info.display_name,
            sizeof(eInfo.display_name)-1);
    eInfo.display_name[sizeof(eInfo.display_name)-1] = '\0';
  }
  if (enroll_info.img_id) {
    strncpy(eInfo.img_id, enroll_info.img_id, sizeof(eInfo.img_id)-1);
    eInfo.img_id[sizeof(eInfo.img_id)-1] = '\0';
  }

  eInfo.type = (vaapi_object_type)enroll_info.object_type;
  eInfo.img_format = (vaapi_img_format)enroll_info.image_format;

  switch (eInfo.img_format) {
    case vaapi_format_GRAY8:
      eInfo.img_width[0] = enroll_info.image_width;
      eInfo.img_height[0] = enroll_info.image_height;
      eInfo.img_pitch[0] = enroll_info.image_width;
      eInfo.img_data[0] = enroll_info.data;
      break;
    case vaapi_format_nv12:
    case vaapi_format_nv21:
    case vaapi_format_yv12:
      //TODO: Add support for additional pixelformats
    default:
      ALOGE("%s: Unsupported format: %d\n", __func__, eInfo.img_format);
      return BAD_VALUE;
  }

  ret = vaapi_enroll_obj((vaapi_event_type)enroll_info.event_type, &eInfo);
  if (VAM_OK != ret) {
    ALOGE("%s: Failed to entroll data to VAM: %d\n", __func__, ret);
  }

  return RcConvert(ret);
}

int32_t VAMInstance::VAMConfig(const char *json_config) {
  int32_t ret = NO_ERROR;
  if (NULL == json_config) {
    ALOGE("%s: Invalid json config!\n", __func__);
    return BAD_VALUE;
  }

  ret = VAMWaitForInit();
  if (NO_ERROR != ret)
    return ret;

  VAMConfigParser parser;
  ret = parser.Init();
  if (NO_ERROR == ret) {
    Mutex::Autolock l(vam_config_context_lock_);
    memset(&vam_config_, 0, sizeof(vam_config_));

    ret = parser.ParseConfig(json_config, vam_config_);
    if (NO_ERROR == ret) {
        ret = vaapi_set_config(&vam_config_);
        if (VAM_OK != ret) {
          ALOGE("%s: Failed to configure VAM: %d\n", __func__, ret);
        }
        ret = RcConvert(ret);
        memset(&vam_config_, 0, sizeof(vam_config_));
    } else {
      ALOGE("%s: Configuration parsing failed: %d\n", __func__, ret);
    }
  } else {
    ALOGE("%s: Failed to initialize json config parser: %d\n",
          __func__, ret);
  }

  return ret;
}

int32_t VAMInstance::VAMRemoveConfig(const char *json_config) {
  int32_t ret = NO_ERROR;
  if (NULL == json_config) {
    ALOGE("%s: Invalid json config!\n", __func__);
    return BAD_VALUE;
  }

  ret = VAMWaitForInit();
  if (NO_ERROR != ret)
    return ret;

  VAMConfigParser parser;
  ret = parser.Init();
  if (NO_ERROR == ret) {
    Mutex::Autolock l(vam_config_context_lock_);
    memset(&vam_config_, 0, sizeof(vam_config_));

    ret = parser.ParseConfig(json_config, vam_config_);
    if (NO_ERROR == ret) {
        ret = vaapi_del_config(&vam_config_);
        if (VAM_OK != ret) {
          ALOGE("%s: Failed to remove VAM config: %d\n", __func__, ret);
        }
        ret = RcConvert(ret);
        memset(&vam_config_, 0, sizeof(vam_config_));
    } else {
      ALOGE("%s: Configuration parsing failed: %d\n", __func__, ret);
    }
  } else {
    ALOGE("%s: Failed to initialize json config parser: %d\n",
          __func__, ret);
  }

  return ret;
}

int32_t VAMInstance::InitVAM(const uint32_t session_id,
                             const uint32_t track_id) {
  int32_t ret = NO_ERROR;
  Mutex::Autolock l(vam_config_context_lock_);
  if (!vam_context_.present) {
    vam_context_.present = true;
    vam_context_.session_id = session_id;
    vam_context_.track_id = track_id;
    vam_context_.active = false;
  } else {
    ALOGE("%s: VAM enabled track with id: %d already present!\n",
          __func__, vam_context_.track_id);
    ret = INVALID_OPERATION;
  }

  return ret;
}

int32_t VAMInstance::StartVAM(CameraBufferMetaData &meta_data) {
  vaapi_source_info info;

  memset(&info, 0, sizeof(info));
  snprintf(info.data_folder, sizeof(info.data_folder), kVAMDataPath);
  info.frame_l_enable = 1;

  switch (meta_data.format) {
    case BufferFormat::kNV12:
      info.img_format = vaapi_format_nv12;
      assert(2 == meta_data.num_planes);
      info.frame_l_width[0] = meta_data.plane_info[0].width;
      info.frame_l_height[0] = meta_data.plane_info[0].height;
      info.frame_l_pitch[0] = meta_data.plane_info[0].stride;
      info.frame_l_scanline[0] = meta_data.plane_info[0].scanline;
      info.frame_l_width[1] = meta_data.plane_info[1].width;
      info.frame_l_height[1] = meta_data.plane_info[1].height;
      info.frame_l_pitch[1] = meta_data.plane_info[1].stride;
      info.frame_l_scanline[1] = meta_data.plane_info[1].scanline;
      break;
    case BufferFormat::kNV21:
      info.img_format = vaapi_format_nv21;
      assert(2 == meta_data.num_planes);
      info.frame_l_width[0] = meta_data.plane_info[0].width;
      info.frame_l_height[0] = meta_data.plane_info[0].height;
      info.frame_l_pitch[0] = meta_data.plane_info[0].stride;
      info.frame_l_scanline[0] = meta_data.plane_info[0].scanline;
      info.frame_l_width[1] = meta_data.plane_info[1].width;
      info.frame_l_height[1] = meta_data.plane_info[1].height;
      info.frame_l_pitch[1] = meta_data.plane_info[1].stride;
      info.frame_l_scanline[1] = meta_data.plane_info[1].scanline;
      break;
    case BufferFormat::kBLOB:// These don't seem supported
    case BufferFormat::kRAW10:// by VAM currently!
    case BufferFormat::kRAW16:
    default:
      ALOGE("%s: Unsupported format: %d\n", __func__,
            meta_data.format);
      return BAD_VALUE;
  }

  {
    Mutex::Autolock l(vam_pending_buffers_lock_);
    vam_pending_buffers_.clear();
  }
  auto ret = vaapi_init(&info, kVAMDynamicPath);
  if (VAM_OK != ret) {
    ALOGE("%s: Failed to initialize VAM: %d\n", __func__, ret);
    return RcConvert(ret);
  }

  //TODO: Add callback for snapshot
  ret = vaapi_register_frame_processed_cb(VAMFrameProcessedCb, this);
  if (VAM_OK != ret) {
    ALOGE("%s: Failed to register processed callback: %d\n", __func__, ret);
    goto exit;
  }

  ret = vaapi_register_event_cb(VAMEventCb, this);
  if (VAM_OK != ret) {
    ALOGE("%s: Failed to register event callback: %d\n", __func__, ret);
    goto exit;
  }

  ret = vaapi_register_metadata_cb(VAMMetadataCb, this);
  if (VAM_OK != ret) {
    ALOGE("%s: Failed to register metadata callback: %d\n", __func__, ret);
    goto exit;
  }

  ret = vaapi_run();
  if (VAM_OK != ret) {
    ALOGE("%s: Failed to start VAM: %d\n", __func__, ret);
    goto exit;
  }

  {
    Mutex::Autolock l(vam_config_context_lock_);
    vam_context_.active = true;
    wait_for_vam_to_init_.signal();
  }

  return RcConvert(ret);

exit:

  vaapi_deinit();

  return RcConvert(ret);
}

int32_t VAMInstance::QueueVAMBuffers(uint32_t track_id,
                                     uint32_t session_id,
                                     std::vector<BufferDescriptor> &buffers,
                                     std::vector<MetaData> &meta_data) {
  int32_t ret = NO_ERROR;
  uint32_t camera_flag = static_cast<uint32_t>(MetaParamType::kCamBufMetaData);
  if (vam_context_.present) {
    if (meta_data.empty() || (buffers.size() != meta_data.size())) {
      ALOGE("%s: Invalid meta data!\n",
            __func__);
      return BAD_VALUE;
    }

    for (size_t i = 0; i < buffers.size(); i++) {
      BufferDescriptor &iter = buffers[i];
      MetaData &meta_buffer = meta_data[i];
      if (0 == (meta_buffer.meta_flag &= camera_flag)) {
        ALOGE("%s: No valid meta data for buffer %d!\n", __func__, i);
        continue;
      }
      CameraBufferMetaData camera_meta = meta_buffer.cam_buffer_meta_data;

      struct vaapi_frame_info buffer_info;
      memset(&buffer_info, 0, sizeof(buffer_info));

      switch (camera_meta.format) {
        case BufferFormat::kNV12:
        case BufferFormat::kNV21:
          buffer_info.frame_l_data[0] = (uint8_t *) iter.data;
          buffer_info.frame_l_data[1] = ((uint8_t *) iter.data) +
              (camera_meta.plane_info[0].stride *
                  camera_meta.plane_info[0].scanline);
          break;
        case BufferFormat::kBLOB:// These don't seem supported
        case BufferFormat::kRAW10:// by VAM currently!
        case BufferFormat::kRAW16:
        default:
          ALOGE("%s: Unsupported format: %d\n", __func__,
                camera_meta.format);
          return BAD_VALUE;
      }
      buffer_info.time_stamp = iter.timestamp;

      {
        Mutex::Autolock l(vam_pending_buffers_lock_);
        VAMPendingBuffer pending_buffer(track_id, session_id, iter);
        vam_pending_buffers_.add(buffer_info.time_stamp, pending_buffer);
      }

      ret = vaapi_process(&buffer_info);
      if (VAM_OK != ret) {
        if (VAM_BUSY != ret) {
          ALOGE("%s: VAM process failed: %d\n", __func__,
                ret);
        }
        {
          Mutex::Autolock l(vam_pending_buffers_lock_);
          vam_pending_buffers_.removeItem(buffer_info.time_stamp);
        }
      }
      ret = RcConvert(ret);
    }
  } else {
    ALOGE("%s: VAM context not present!\n", __func__);
    return NO_INIT;
  }

  return ret;
}

int32_t VAMInstance::VAMFrameProcessed(uint64_t time_stamp) {
  Mutex::Autolock l(vam_pending_buffers_lock_);
  ssize_t idx = vam_pending_buffers_.indexOfKey(time_stamp);
  if (0 <= idx) {
    VAMPendingBuffer pending_buffer = vam_pending_buffers_.valueAt(idx);
    vam_pending_buffers_.removeItem(time_stamp);
    auto ret = vam_cb_->ReturnTrackBuffer(pending_buffer.session_id,
                                          pending_buffer.track_id,
                                          pending_buffer.buffer);
    if(NO_ERROR != ret) {
      ALOGE("%s: ReturnTrackBuffer failed: %d!", __func__, ret);
      return VAM_FAIL;
    }
  } else {
    ALOGE("%s: No pending buffers found!\n", __func__);
    return VAM_NOTFOUND;
  }

  return VAM_OK;
}

int32_t VAMInstance::VAMFrameProcessedCb(uint64_t time_stamp, void *usr_data) {
  int32_t ret;
  VAMInstance *ctx = static_cast<VAMInstance *> (usr_data);
  if (NULL != ctx) {
    ret = ctx->VAMFrameProcessed(time_stamp);
  } else {
    ALOGE("%s: Invalid user data!\n", __func__);
    ret = VAM_NULLPTR;
  }

  return ret;
}

int32_t VAMInstance::VAMEvent(struct vaapi_event *event) {
  if (NULL == event) {
    return VAM_NULLPTR;
  }

  std::string eventString = getStrFromEvent(event);
  vam_cb_->SendVAMMeta(vam_context_.session_id, vam_context_.track_id,
                        eventString.c_str(), eventString.size(), event->pts);

  return VAM_OK;
}

int32_t VAMInstance::VAMEventCb(struct vaapi_event *event, void* usr_data) {
  int32_t ret;
  VAMInstance *ctx = static_cast<VAMInstance *> (usr_data);
  if (NULL != ctx) {
    ret = ctx->VAMEvent(event);
  } else {
    ALOGE("%s: Invalid user data!\n", __func__);
    ret = VAM_NULLPTR;
  }

  return ret;
}

int32_t VAMInstance::VAMMetadata(struct vaapi_metadata_frame *frame) {
  if (NULL == frame) {
    return VAM_NULLPTR;
  }

  std::string metaString = getStrFromMetadataFrame(frame);
  vam_cb_->SendVAMMeta(vam_context_.session_id, vam_context_.track_id,
                       metaString.c_str(), metaString.size(), frame->pts);

  return VAM_OK;
}

int32_t VAMInstance::VAMMetadataCb(struct vaapi_metadata_frame *frame,
                                   void* usr_data) {
  int32_t ret;
  VAMInstance *ctx = static_cast<VAMInstance *> (usr_data);
  if (NULL != ctx) {
    ret = ctx->VAMMetadata(frame);
  } else {
    ALOGE("%s: Invalid user data!\n", __func__);
    ret = VAM_NULLPTR;
  }

  return ret;
}

int32_t VAMInstance::CloseVAM() {
  int32_t ret = VAM_OK;
  Mutex::Autolock l(vam_config_context_lock_);

  if (vam_context_.present) {
    if (vam_context_.active) {
      ret = vaapi_stop();
      if (VAM_OK != ret) {
        ALOGE("%s: Error trying to stop VAM: %d\n", __func__, ret);
      }

      ret = vaapi_deinit();
      if (VAM_OK != ret) {
        ALOGE("%s: Error trying to de-initialize VAM: %d\n",
              __func__, ret);
      }
    }
    memset(&vam_context_, 0, sizeof(vam_context_));
  }

  return RcConvert(ret);
}

int32_t VAMInstance::RcConvert(int32_t ret) {
  int32_t rc;

  switch (ret) {
  case VAM_OK:
    rc = NO_ERROR;
    break;
  case VAM_FAIL:
    rc = UNKNOWN_ERROR;
    break;
  case VAM_NOTFOUND:
    rc = NAME_NOT_FOUND;
    break;
  case VAM_TIMEOUT:
    rc = TIMED_OUT;
    break;
  case VAM_NULLPTR:
    rc = BAD_VALUE;
    break;
  case VAM_NOT_INITED:
    rc = NO_INIT;
    break;
  case VAM_IMG_FORMAT_NOT_SUPPORTED:
    rc = BAD_TYPE;
    break;
  case VAM_LOCK_FAIL:
    rc = WOULD_BLOCK;
    break;
  case VAM_UNLOCK_FAIL:
    rc = INVALID_OPERATION;
    break;
  case VAM_FRAME_QUEUE_FULL:
    rc = NO_MEMORY;
    break;
  case VAM_ENGINE_IS_NOT_RUNNING:
    rc = DEAD_OBJECT;
    break;
  case VAM_ENGINE_HAS_NO_CONFIG:
    rc = NOT_ENOUGH_DATA;
    break;
  case VAM_ENGINE_DYLIB_PATH_NOT_SET:
    rc = PERMISSION_DENIED;
    break;
  case VAM_ENGINE_DYLIB_NOT_EXIST:
    rc = UNKNOWN_TRANSACTION;
    break;
  case VAM_ENGINE_SYMBOL_ERROR:
    rc = FAILED_TRANSACTION;
    break;
  case VAM_SETRULE_FAIL:
    rc = FDS_NOT_ALLOWED;
    break;
  case VAM_BUSY:
    rc = ALREADY_EXISTS;
    break;
  case VAM_STR_OVERSIZE:
    rc = INVALID_OPERATION;
    break;
  default:
    ALOGE("%s: Error not converted VAM return code: %d\n", __func__, ret);
    rc = UNKNOWN_ERROR + ret + 100;
  }

  return rc;
}
} //namespace vaminterface ends here

} //namespace qmmf ends here
