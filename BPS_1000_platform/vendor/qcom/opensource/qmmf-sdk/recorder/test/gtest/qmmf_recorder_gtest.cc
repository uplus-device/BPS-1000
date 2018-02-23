/*
* Copyright (c) 2016, The Linux Foundation. All rights reserved.
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

#define TAG "RecorderGTest"


#include <utils/Log.h>
#include <utils/String8.h>
#include <utils/Errors.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <camera/CameraMetadata.h>
#include <system/graphics.h>
#include <random>
#include <QCamera3VendorTags.h>
#include <sys/time.h>

#include <qmmf-sdk/qmmf_queue.h>
#include "recorder/test/gtest/qmmf_recorder_gtest.h"

#define DUMP_META_PATH "/data/misc/qmmf/param.dump"

//#define DEBUG
#define TEST_INFO(fmt, args...)  ALOGD(fmt, ##args)
#define TEST_ERROR(fmt, args...) ALOGE(fmt, ##args)
#define TEST_WARN(fmt, args...) ALOGW(fmt, ##args)
#ifdef DEBUG
#define TEST_DBG  TEST_INFO
#else
#define TEST_DBG(...) ((void)0)
#endif

static const int32_t kRecordDuration = 2*60;   // 2 min for each iteration.
static const uint32_t kZslWidth      = 1920;
static const uint32_t kZslHeight     = 1080;
static const uint32_t kZslQDepth     = 10;
static const int32_t kDefaultJpegQuality = 85;

#define COLOR_DARK_GRAY 0x202020FF;
#define COLOR_YELLOW    0xFFFF00FF;
#define COLOR_BLUE      0x0000CCFF;
#define COLOR_WHITE     0xFFFFFFFF;
#define COLOR_ORANGE    0xFF8000FF;
#define COLOR_LIGHT_GREEN 0x33CC00FF;
#define COLOR_LIGHT_BLUE 0x189BF2FF;

#define FHD_1080p_STREAM_WIDTH 1920
#define FHD_1080p_STREAM_HEIGHT 1080

using namespace qcamera;

void RecorderGtest::SetUp() {

  TEST_INFO("%s:%s Enter ", TAG, __func__);

  test_info_ = ::testing::UnitTest::GetInstance()->current_test_info();

  recorder_status_cb_.event_cb = [this] (EventType event_type, void *event_data,
                                         size_t event_data_size) -> void
      { RecorderCallbackHandler(event_type, event_data, event_data_size); };

  char prop_val[PROPERTY_VALUE_MAX];
  property_get(PROP_DUMP_BITSTREAM, prop_val, "0");
  if (atoi(prop_val) == 0) {
    dump_bitstream_.Enable(false);
  } else {
    dump_bitstream_.Enable(true);
  }
  property_get(PROP_DUMP_JPEG, prop_val, "0");
  is_dump_jpeg_enabled_ = (atoi(prop_val) == 0) ? false : true;
  property_get(PROP_DUMP_RAW, prop_val, "0");
  is_dump_raw_enabled_ = (atoi(prop_val) == 0) ? false : true;
  property_get(PROP_DUMP_YUV_FRAMES, prop_val, "0");
  is_dump_yuv_enabled_ = (atoi(prop_val) == 0) ? false : true;
  property_get(PROP_DUMP_YUV_FREQ, prop_val, DEFAULT_YUV_DUMP_FREQ);
  dump_yuv_freq_ = atoi(prop_val);
  property_get(PROP_N_ITERATIONS, prop_val, DEFAULT_ITERATIONS);
  iteration_count_ = atoi(prop_val);
  property_get(PROP_CAMERA_ID, prop_val, "0");
  camera_id_ = atoi(prop_val);

  memset(&camera_start_params_, 0x0, sizeof camera_start_params_);
  camera_start_params_.zsl_mode         = false;
  camera_start_params_.zsl_queue_depth  = kZslQDepth;
  camera_start_params_.zsl_width        = kZslWidth;
  camera_start_params_.zsl_height       = kZslHeight;
  camera_start_params_.frame_rate       = 30;
  camera_start_params_.flags            = 0x0;

  TEST_INFO("%s:%s Exit ", TAG, __func__);
}

void RecorderGtest::TearDown() {

  TEST_INFO("%s:%s Enter ", TAG, __func__);
  TEST_INFO("%s:%s Exit ", TAG, __func__);
}

int32_t RecorderGtest::Init() {

  auto ret = recorder_.Connect(recorder_status_cb_);
  assert(ret == NO_ERROR);
  return ret;
}

int32_t RecorderGtest::DeInit() {

  auto ret = recorder_.Disconnect();
  assert(ret == NO_ERROR);
  return ret;
}

void RecorderGtest::InitSupportedVHDRModes() {
  camera_metadata_entry_t entry;
  if (static_info_.exists(QCAMERA3_AVAILABLE_VIDEO_HDR_MODES)) {
    entry = static_info_.find(QCAMERA3_AVAILABLE_VIDEO_HDR_MODES);
    for (uint32_t i = 0 ; i < entry.count; i++) {
      supported_hdr_modes_.push_back(entry.data.i32[i]);
    }
  }
}

bool RecorderGtest::IsVHDRSupported() {
  bool is_supported = false;
  for (const auto& mode : supported_hdr_modes_) {
    if (QCAMERA3_VIDEO_HDR_MODE_ON == mode) {
      is_supported = true;
      break;
    }
  }
  return is_supported;
}

void RecorderGtest::InitSupportedNRModes() {
  camera_metadata_entry_t entry;
  if (static_info_.exists(
      ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES)) {
    entry = static_info_.find(
        ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES);
    for (uint32_t i = 0 ; i < entry.count; i++) {
      supported_nr_modes_.push_back(entry.data.u8[i]);
    }
  }
}

bool RecorderGtest::IsNRSupported() {
  bool is_supported = false;
  for (const auto& mode : supported_nr_modes_) {
    if (ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY == mode) {
      is_supported = true;
      break;
    }
  }
  return is_supported;
}

/*
* ConnectToService: This test case will test Connect/Disconnect Api.
* Api test sequence:
*  - Connect
*  - Disconnect
*/
TEST_F(RecorderGtest, ConnectToService) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    auto ret = recorder_.Connect(recorder_status_cb_);
    assert(ret == NO_ERROR);
    sleep(3);

    ret = recorder_.Disconnect();
    assert(ret == NO_ERROR);
  }
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* ZSLStartStopCamera: This test case will test Start & StopCamera Api in ZSL
*  mode.
* Api test sequence:
*   loop Start {
*   ------------------
*  - StartCamera
*  - StopCamera
*   ------------------
*   } loop End
*/
TEST_F(RecorderGtest, StartStopCameraZSLMode) {

  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();

  camera_start_params_.zsl_mode = true;
  camera_start_params_.zsl_width = 1920;
  camera_start_params_.zsl_height = 1080;
  camera_start_params_.zsl_queue_depth = 4;
  camera_start_params_.frame_rate = 30;

  assert(ret == NO_ERROR);
  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.StartCamera(camera_id_, camera_start_params_);
    assert(ret == NO_ERROR);
    sleep(3);

    ret = recorder_.StopCamera(camera_id_);
    assert(ret == NO_ERROR);
  }
  ret = DeInit();
  assert(ret == NO_ERROR);
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
 * FaceDetectionFor1080pYUVPreview: This test will test FD at 1080p preview stream.
 * Api test sequence:
 *  - StartCamera
 *   ------------------
 *   - CreateSession
 *   - CreateVideoTrack
 *   - StartVideoTrack
 *   - SetCameraParam
 *   - StopSession
 *   - Delete Overlay object
 *   - DeleteVideoTrack
 *   - DeleteSession
 *   ------------------
 *  - StopCamera
 */
TEST_F(RecorderGtest, FaceDetectionFor1080pYUVPreview) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
    test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  int32_t stream_width  = FHD_1080p_STREAM_WIDTH;
  int32_t stream_height = FHD_1080p_STREAM_HEIGHT;
  face_info_.fd_stream_width = stream_width;
  face_info_.fd_stream_height = stream_height;
  CameraResultCb result_cb = [this] (uint32_t camera_id,
      const CameraMetadata &result) {
           ParseFaceInfo(result, face_info_);
           ApplyFaceOveralyOnStream(face_info_);};

  ret = recorder_.StartCamera(camera_id_, camera_start_params_, result_cb);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
    size_t event_data_size) -> void
    { SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id     = camera_id_;
  video_track_param.width         = stream_width;
  video_track_param.height        = stream_height;
  video_track_param.frame_rate    = 30;
  video_track_param.format_type   = VideoFormat::kYUV;
  video_track_param.low_power_mode = true;
  uint32_t video_track_id = 1;

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
    std::vector<BufferDescriptor> buffers,
    std::vector<MetaData> meta_buffers) {
    VideoTrackYUVDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
    void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
    event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
  video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  CameraMetadata meta;
  uint8_t fd_mode = ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE;

  ret = recorder_.GetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);
  EXPECT_TRUE(meta.exists(ANDROID_STATISTICS_FACE_DETECT_MODE));

  meta.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &fd_mode, 1);
  ret = recorder_.SetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  face_bbox_active_ = true;
  face_track_id_ = video_track_id;
  TEST_INFO("Enable Face Detection");

  sleep(kRecordDuration);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);
  face_overlay_lock_.lock();
  for (uint32_t i = 0; i < face_bbox_id_.size(); i++) {
    // Delete overlay object.
    ret = recorder_.DeleteOverlayObject(face_track_id_,
                                        face_bbox_id_[i]);
    assert(ret == NO_ERROR);
  }
  face_bbox_active_ = false;
  face_bbox_id_.clear();
  face_overlay_lock_.unlock();

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
 * FaceDetectionFor1080pAVCVideo: This test will test FD at 1080p video stream.
 * Api test sequence:
 *  - StartCamera
 *   ------------------
 *   - CreateSession
 *   - CreateVideoTrack
 *   - StartVideoTrack
 *   - SetCameraParam
 *   - StopSession
 *   - Delete Overlay object
 *   - DeleteVideoTrack
 *   - DeleteSession
 *   ------------------
 *  - StopCamera
 */
TEST_F(RecorderGtest, FaceDetectionFor1080pAVCVideo) {

  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t stream_width  = FHD_1080p_STREAM_WIDTH;
  int32_t stream_height = FHD_1080p_STREAM_HEIGHT;

  face_info_.fd_stream_width = stream_width;
  face_info_.fd_stream_height = stream_height;
  CameraResultCb result_cb = [this] (uint32_t camera_id,
      const CameraMetadata &result) {
           ParseFaceInfo(result, face_info_);
           ApplyFaceOveralyOnStream(face_info_);};

  ret = recorder_.StartCamera(camera_id_, camera_start_params_, result_cb);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
     [this] (EventType event_type, void *event_data, size_t event_data_size) -> void {
     SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id  = camera_id_;
  video_track_param.width = stream_width;
  video_track_param.height = stream_height;
  video_track_param.frame_rate = 30;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      video_track_param.format_type,
      video_track_id,
      stream_width,
      stream_height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
     std::vector<BufferDescriptor> buffers, std::vector<MetaData> meta_buffers) {
        VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
    [this] (uint32_t track_id, EventType event_type,void *event_data,
           size_t event_data_size) -> void { VideoTrackEventCb(track_id,
             event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  CameraMetadata meta;
  uint8_t fd_mode = ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE;

  ret = recorder_.GetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);
  EXPECT_TRUE(meta.exists(ANDROID_STATISTICS_FACE_DETECT_MODE));

  meta.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &fd_mode, 1);
  ret = recorder_.SetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  face_bbox_active_ = true;
  face_track_id_ = video_track_id;
  TEST_INFO("Enable Face Detection");

  sleep(kRecordDuration);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  face_overlay_lock_.lock();
  for (uint32_t i = 0; i < face_bbox_id_.size(); i ++) {
    // Delete overlay object.
    ret = recorder_.DeleteOverlayObject(face_track_id_,
                                        face_bbox_id_[i]);
    assert(ret == NO_ERROR);
  }
  face_bbox_active_ = false;
  face_bbox_id_.clear();
  face_overlay_lock_.unlock();

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);
  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);
  ClearSessions();
  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);
  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 1080pZSLCapture: This case will test 1080p ZSL capture.
* Api test sequence:
*   ------------------
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  *   loop Start {
*   ------------------
*  - CaptureImage (ZSL)
*  *   ------------------
*   } loop End
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - CaptureImage
*  - StopCamera
*   ------------------
*/
TEST_F(RecorderGtest, 1080pZSLCapture) {

  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  camera_start_params_.zsl_mode = true;
  camera_start_params_.zsl_width = 1920;
  camera_start_params_.zsl_height = 1080;
  camera_start_params_.zsl_queue_depth = 4;
  camera_start_params_.frame_rate = 30;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void
      { SessionCallbackHandler(event_type, event_data, event_data_size); };

  sleep(3);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = camera_start_params_.zsl_width;
  image_param.height        = camera_start_params_.zsl_height;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer,
                              MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                 cb);
    assert(ret == NO_ERROR);
    sleep(3);
  }

  ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
  assert(ret == NO_ERROR);
  sleep(3);

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 1080pZSL1080pVideo: This case will test 1080p ZSL along with 1080p AVC video.
* Api test sequence:
*   ------------------
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  *   loop Start {
*   ------------------
*  - CaptureImage (ZSL)
*  *   ------------------
*   } loop End
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - CaptureImage
*  - StopCamera
*   ------------------
*/
TEST_F(RecorderGtest, 1080pZSL1080pVideo) {

  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  camera_start_params_.zsl_mode = true;
  camera_start_params_.zsl_width = 1920;
  camera_start_params_.zsl_height = 1080;
  camera_start_params_.zsl_queue_depth = 4;
  camera_start_params_.frame_rate = 30;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void
      { SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = camera_start_params_.zsl_width;
  video_track_param.height      = camera_start_params_.zsl_height;
  video_track_param.frame_rate  = 30;
  video_track_param.format_type = VideoFormat::kAVC;
  uint32_t video_track_id       = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      video_track_param.format_type,
      video_track_id,
      static_cast<int32_t>(video_track_param.width),
      static_cast<int32_t>(video_track_param.height) };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id, std::vector<BufferDescriptor>
      buffers, std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
      void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  sleep(3);
  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(3);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = camera_start_params_.zsl_width;
  image_param.height        = camera_start_params_.zsl_height;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer,
                              MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                 cb);
    assert(ret == NO_ERROR);
    sleep(3);
  }

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
  assert(ret == NO_ERROR);
  sleep(3);

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 4KZSL1080pYUVPreview: This case will test ZSL at 4K along with 1080p
* YUV preview.
* Api test sequence:
*   ------------------
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  *   loop Start {
*   ------------------
*  - CaptureImage (ZSL)
*  *   ------------------
*   } loop End
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - CaptureImage
*  - StopCamera
*   ------------------
*/
TEST_F(RecorderGtest, 4KZSL1080pYUVPreview) {

  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  camera_start_params_.zsl_mode = true;
  camera_start_params_.zsl_width = 3840;
  camera_start_params_.zsl_height = 2160;
  camera_start_params_.zsl_queue_depth = 4;
  camera_start_params_.frame_rate = 30;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void
      { SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam preview_track_param;
  memset(&preview_track_param, 0x0, sizeof preview_track_param);

  preview_track_param.camera_id      = camera_id_;
  preview_track_param.width          = 1920;
  preview_track_param.height         = 1080;
  preview_track_param.frame_rate     = camera_start_params_.frame_rate;
  preview_track_param.format_type    = VideoFormat::kYUV;
  preview_track_param.low_power_mode = true;
  uint32_t preview_track_id          = 1;

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id, std::vector<BufferDescriptor>
      buffers, std::vector<MetaData> meta_buffers) {
      VideoTrackYUVDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
      void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, preview_track_id,
                                    preview_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(preview_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(5);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = camera_start_params_.zsl_width;
  image_param.height        = camera_start_params_.zsl_height;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer,
                              MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                 cb);
    assert(ret == NO_ERROR);
    sleep(3);
  }

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, preview_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
  assert(ret == NO_ERROR);
  sleep(3);

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 4KZSL1080p480pYUVPreview: This case will test ZSL at 4K along with 1080p
* and 480p YUV preview.
* Api test sequence:
*   ------------------
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  *   loop Start {
*   ------------------
*  - CaptureImage (ZSL)
*  *   ------------------
*   } loop End
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - CaptureImage
*  - StopCamera
*   ------------------
*/
TEST_F(RecorderGtest, 4KZSL1080p480pYUVPreview) {

  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  camera_start_params_.zsl_mode = true;
  camera_start_params_.zsl_width = 3840;
  camera_start_params_.zsl_height = 2160;
  camera_start_params_.zsl_queue_depth = 4;
  camera_start_params_.frame_rate = 30;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void
      { SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam preview_track_param;
  memset(&preview_track_param, 0x0, sizeof preview_track_param);

  preview_track_param.camera_id      = camera_id_;
  preview_track_param.width          = 1920;
  preview_track_param.height         = 1080;
  preview_track_param.frame_rate     = camera_start_params_.frame_rate;
  preview_track_param.format_type    = VideoFormat::kYUV;
  preview_track_param.low_power_mode = true;
  uint32_t preview_track_id          = 1;

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id, std::vector<BufferDescriptor>
      buffers, std::vector<MetaData> meta_buffers) {
      VideoTrackYUVDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
      void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, preview_track_id,
                                    preview_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(preview_track_id);

  preview_track_param.width         = 720;
  preview_track_param.height        = 480;
  uint32_t preview_track480p_id = 2;

  ret = recorder_.CreateVideoTrack(session_id, preview_track480p_id,
                                    preview_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(preview_track480p_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(5);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = camera_start_params_.zsl_width;
  image_param.height        = camera_start_params_.zsl_height;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer,
                              MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                 cb);
    assert(ret == NO_ERROR);
    sleep(3);
  }

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, preview_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, preview_track480p_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 4KZSLTwo1080pVideo: This case will test ZSL at 4K along with two 1080p
* videos.
* Api test sequence:
*   ------------------
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  *   loop Start {
*   ------------------
*  - CaptureImage (ZSL)
*  *   ---------------
*   } loop End
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - CaptureImage
*  - StopCamera
*   ------------------
*/
TEST_F(RecorderGtest, 4KZSLTwo1080pVideo) {

  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  camera_start_params_.zsl_mode = true;
  camera_start_params_.zsl_width = 3840;
  camera_start_params_.zsl_height = 2160;
  camera_start_params_.zsl_queue_depth = 4;
  camera_start_params_.frame_rate = 30;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void
      { SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = 1920;
  video_track_param.height      = 1080;
  video_track_param.frame_rate  = 30;
  video_track_param.format_type = VideoFormat::kAVC;
  uint32_t video_track_id_1      = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      video_track_param.format_type,
      video_track_id_1,
      static_cast<int32_t>(video_track_param.width),
      static_cast<int32_t>(video_track_param.height) };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id, std::vector<BufferDescriptor>
      buffers, std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
      void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id_1,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id_1);

  uint32_t video_track_id_2 = 2;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      video_track_param.format_type,
      video_track_id_2,
      static_cast<int32_t>(video_track_param.width),
      static_cast<int32_t>(video_track_param.height) };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_cb.data_cb = [&] (uint32_t track_id, std::vector<BufferDescriptor>
      buffers, std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id_2,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track_id_2);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(5);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = camera_start_params_.zsl_width;
  image_param.height        = camera_start_params_.zsl_height;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer,
                              MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                 cb);
    assert(ret == NO_ERROR);
    sleep(3);
  }

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id_1);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id_2);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
  assert(ret == NO_ERROR);
  sleep(3);

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* StartStopCamera: This test case will test Start & StopCamera Api.
* Api test sequence:
*   loop Start {
*   ------------------
*  - StartCamera
*  - StopCamera
*   ------------------
*   } loop End
*/
TEST_F(RecorderGtest, StartStopCamera) {

  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);
  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.StartCamera(camera_id_, camera_start_params_);
    assert(ret == NO_ERROR);
    sleep(3);

    ret = recorder_.StopCamera(camera_id_);
    assert(ret == NO_ERROR);
  }
  ret = DeInit();
  assert(ret == NO_ERROR);
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* CreateDeleteSession: This test will test Create & Delete Session Api.
* Api test sequence:
*   loop Start {
*   ------------------
*  - StartCamera
*  - CreateSession
*  - DeleteSession
*  - StopCamera
*   ------------------
*   } loop End
*/
TEST_F(RecorderGtest, CreateDeleteSession) {

  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);
  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.StartCamera(camera_id_, camera_start_params_);
    assert(ret == NO_ERROR);

    SessionCb session_status_cb;
    session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                         size_t event_data_size) -> void
        { SessionCallbackHandler(event_type, event_data, event_data_size); };

    uint32_t session_id;
    ret = recorder_.CreateSession(session_status_cb, &session_id);
    assert(session_id > 0);
    assert(ret == NO_ERROR);
    sleep(2);
    ret = recorder_.DeleteSession(session_id);
    assert(ret == NO_ERROR);
    sleep(1);
    ret = recorder_.StopCamera(camera_id_);
    assert(ret == NO_ERROR);
  }
  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 4KSnapshot: This test will test 4K JPEG snapshot.
* Api test sequence:
*  - StartCamera
*   loop Start {
*   ------------------
*   - CaptureImage - JPEG
*   ------------------
*   } loop End
*  - StopCamera
*/
TEST_F(RecorderGtest, 4KSnapshot) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 3840;
  image_param.height        = 2160;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;
  // Check Supported JPEG snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                  static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true; // 3840x2160 JPEG supported.
          }
        }
      }
    }
  }
  assert (res_supported != false);

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                                BufferDescriptor buffer,
                                MetaData meta_data) -> void
        { SnapshotCb(camera_id, image_count, buffer, meta_data); };

    uint8_t awb_mode = ANDROID_CONTROL_AWB_MODE_INCANDESCENT;
    ret = meta.update(ANDROID_CONTROL_AWB_MODE, &awb_mode, 1);
    assert(ret == NO_ERROR);

    meta_array.push_back(meta);
    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                 cb);
    assert(ret == NO_ERROR);
    // Take snapshot after every 5 sec.
    sleep(5);
  }

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());

}

/*
* BurstSnapshot: This test will test 4K Burst jpg snapshot.
* Api test sequence:
*  - StartCamera
*  - CaptureImage - Burst Jpg
*  - StopCamera
*/
TEST_F(RecorderGtest, BurstSnapshot) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  camera_start_params_.frame_rate = 30;
  //camera_start_params_.setSensorVendorMode(6);
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 3840;
  image_param.height        = 2160;
  image_param.image_format  = ImageFormat::kJPEG;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;
  // Check Supported Raw YUV snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                  static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true; // 1920x1080 YUV res supported.
          }
        }
      }
    }
  }
  assert (res_supported != false);

  TEST_INFO("%s:%s: Running Test(%s)", TAG, __func__,
    test_info_->name());

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                                BufferDescriptor buffer,
                                MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  uint8_t awb_mode = ANDROID_CONTROL_AWB_MODE_INCANDESCENT;
  ret = meta.update(ANDROID_CONTROL_AWB_MODE, &awb_mode, 1);
  assert(ret == NO_ERROR);

  meta.update(ANDROID_JPEG_QUALITY, &kDefaultJpegQuality, 1);

  uint32_t num_images = 30;
  for (uint32_t i = 0; i < num_images; i++) {
    meta_array.push_back(meta);
  }
  ret = recorder_.CaptureImage(camera_id_, image_param, num_images, meta_array,
                               cb);
  assert(ret == NO_ERROR);

  sleep(5);

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* MaxSnapshotThumb: This test will test Max resolution JPEG snapshot
*                   with a max size thumbnail.
* Api test sequence:
*  - StartCamera
*   loop Start {
*   ------------------
*   - CaptureImage - JPEG with thumbnail
*   ------------------
*   } loop End
*  - StopCamera
*/
TEST_F(RecorderGtest, MaxSnapshotThumb) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  int32_t thumb_size[2] = {0,0};
  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  // Check Supported JPEG snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width < static_cast<uint32_t>(entry.data.i32[i+1])) {
            image_param.width = entry.data.i32[i+1];
            image_param.height = entry.data.i32[i+2];
          }

          fprintf(stderr,"Supported Size %dx%d\n",
              entry.data.i32[i+1], entry.data.i32[i+2]);
        }
      }
    }
  }
  assert (image_param.width > 0 && image_param.height > 0);

  if (meta.exists(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES)) {
    entry = meta.find(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES);
    for (uint32_t i = 0 ; i < entry.count; i += 2) {
      if (thumb_size[0] < entry.data.i32[i]) {
        thumb_size[0] = entry.data.i32[i];
        thumb_size[1] = entry.data.i32[i+1];
      }
    }
  }
  assert(thumb_size[0] > 0 && thumb_size[1] > 0);
  ret = meta.update(ANDROID_JPEG_THUMBNAIL_SIZE, thumb_size, 2);
  assert(ret == NO_ERROR);


  /* we have only capture stream which will by default disable WB and lead to
     broken picture */
  uint8_t intent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
  ret = meta.update(ANDROID_CONTROL_CAPTURE_INTENT, &intent, 1);
  assert(ret == NO_ERROR);

  fprintf(stderr,"Capturing %dx%d JPEG with %dx%d thumbnail\n",
      image_param.width, image_param.height, thumb_size[0], thumb_size[1]);
  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                                BufferDescriptor buffer,
                                MetaData meta_data) -> void
        { SnapshotCb(camera_id, image_count, buffer, meta_data); };


    meta_array.push_back(meta);
    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                 cb);
    assert(ret == NO_ERROR);
    // Take snapshot after every 5 sec.
    sleep(5);
  }

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());

}


/*
* 1080pRawYUVSnapshot: This test will test 1080p YUV snapshot.
* Api test sequence:
*  - StartCamera
*   loop Start {
*   ------------------
*   - CaptureImage - Raw YUV
*   ------------------
*   } loop End
*  - StopCamera
*/
TEST_F(RecorderGtest, 1080pRawYUVSnapshot) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 1920;
  image_param.height        = 1080;
  image_param.image_format  = ImageFormat::kNV12;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;
  // Check Supported Raw YUV snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                  static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true; // 1920x1080 YUV res supported.
          }
        }
      }
    }
  }
  assert (res_supported != false);

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                                BufferDescriptor buffer,
                                MetaData meta_data) -> void
        { SnapshotCb(camera_id, image_count, buffer, meta_data); };

    uint8_t awb_mode = ANDROID_CONTROL_AWB_MODE_INCANDESCENT;
    ret = meta.update(ANDROID_CONTROL_AWB_MODE, &awb_mode, 1);
    assert(ret == NO_ERROR);

    meta_array.push_back(meta);
    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                 cb);
    assert(ret == NO_ERROR);
    // Take snapshot after every 5 sec.
    sleep(5);
  }

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());

}

/*
* RawBayerRDI10Snapshot: This test will test BayerRDI (10 bits packed) snapshot.
* Api test sequence:
*  - StartCamera
*   loop Start {
*   ------------------
*   - CaptureImage - BayerRDI 10 bits
*   ------------------
*   } loop End
*  - StopCamera
*/
TEST_F(RecorderGtest, RawBayerRDI10Snapshot) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  camera_metadata_entry_t entry;
  CameraMetadata meta;
  int32_t w = 0, h = 0;
  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  // Check Supported bayer snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_RAW_SIZES)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_RAW_SIZES);
    for (uint32_t i = 0 ; i < entry.count; i += 2) {
      w = entry.data.i32[i+0];
      h = entry.data.i32[i+1];
      TEST_INFO("%s:%s: (%d) Supported RAW RDI W(%d):H(%d)", TAG,
          __func__, i, w, h);
    }
  }
  assert(w > 0 && h > 0);
  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width        = w; // 5344
  image_param.height       = h; // 4016
  image_param.image_format = ImageFormat::kBayerRDI;

  std::vector<CameraMetadata> meta_array;
  meta_array.push_back(meta);
  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                                BufferDescriptor buffer,
                                MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                 cb);
    assert(ret == NO_ERROR);
    // Take snapshot after every 5 sec.
    sleep(5);
  }

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());

}

/*
* SessionWith1080pYUVTrack: This test will test session with one 1080p YUV track.
* Api test sequence:
*  - StartCamera
*   loop Start {
*   ------------------
*   - CreateSession
*   - CreateVideoTrack
*   - StartVideoTrack
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   ------------------
*   } loop End
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith1080pYUVTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    SessionCb session_status_cb;
    session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                         size_t event_data_size) -> void
        { SessionCallbackHandler(event_type, event_data, event_data_size); };

    uint32_t session_id;
    ret = recorder_.CreateSession(session_status_cb, &session_id);
    assert(session_id > 0);
    assert(ret == NO_ERROR);

    VideoTrackCreateParam video_track_param;
    memset(&video_track_param, 0x0, sizeof video_track_param);

    video_track_param.camera_id   = camera_id_;
    video_track_param.width       = 1920;
    video_track_param.height      = 1080;
    video_track_param.frame_rate  = 30;
    video_track_param.format_type = VideoFormat::kYUV;
    uint32_t video_track_id       = 1;

    TrackCb video_track_cb;
    video_track_cb.data_cb = [&] (uint32_t track_id,
                                  std::vector<BufferDescriptor> buffers,
                                  std::vector<MetaData> meta_buffers) {
        VideoTrackYUVDataCb(track_id, buffers, meta_buffers); };

    video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
        void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
        event_type, event_data, event_data_size); };

    ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                      video_track_param, video_track_cb);
    assert(ret == NO_ERROR);

    std::vector<uint32_t> track_ids;
    track_ids.push_back(video_track_id);
    sessions_.insert(std::make_pair(session_id, track_ids));

    ret = recorder_.StartSession(session_id);
    assert(ret == NO_ERROR);

    // Let session run for time kRecordDuration, during this time buffer with
    // valid data would be received in track callback (VideoTrackYUVDataCb).
    sleep(kRecordDuration);

    ret = recorder_.StopSession(session_id, false);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteSession(session_id);
    assert(ret == NO_ERROR);

    ClearSessions();
  }

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());

}

/*
* MultiSessionsWith1080pEncTrack: This test will verify multiple sessions with
* 1080p tracks.
* Api test sequence:
*  - StartCamera
*   - CreateSession
*   - CreateVideoTrack
*   - StartSession
*   loop Start {
*   ------------------
*   - CreateSession
*   - CreateVideoTrack
*   - StartSession
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   ------------------
*   } loop End
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, MultiSessionsWith1080pEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void
      { SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id     = camera_id_;
  video_track_param.width         = width;
  video_track_param.height        = height;
  video_track_param.frame_rate    = 30;
  video_track_param.format_type   = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                            std::vector<BufferDescriptor> buffers,
                            std::vector<MetaData> meta_buffers) {
  VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
      void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    uint32_t session_id2;
    ret = recorder_.CreateSession(session_status_cb, &session_id2);
    assert(session_id2 > 0);
    assert(ret == NO_ERROR);

    uint32_t video_track_id2 = 2;
    TrackCb video_track_cb;
    video_track_cb.data_cb = [&] (uint32_t track_id,
                              std::vector<BufferDescriptor> buffers,
                              std::vector<MetaData> meta_buffers) {
     auto ret = recorder_.ReturnTrackBuffer(session_id2, track_id, buffers);
     assert(ret == NO_ERROR); };

    video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
        void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
        event_type, event_data, event_data_size); };

    ret = recorder_.CreateVideoTrack(session_id2, video_track_id2,
                                      video_track_param, video_track_cb);
    assert(ret == NO_ERROR);

    ret = recorder_.StartSession(session_id2);
    assert(ret == NO_ERROR);

    // Let session run for kRecordDuration, during this time buffer with valid
    // data would be received in track callback (VideoTrackDataCb).
    sleep(kRecordDuration);

    ret = recorder_.StopSession(session_id2, false);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(session_id2, video_track_id2);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteSession(session_id2);
    assert(ret == NO_ERROR);
  }

  sleep(kRecordDuration);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());

}

/*
* SessionWith1080pEncTrack: This test will test session with 1080p h264 track.
* Api test sequence:
*  - StartCamera
*   loop Start {
*   ------------------
*   - CreateSession
*   - CreateVideoTrack
*   - StartVideoTrack
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   ------------------
*   } loop End
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith1080pEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    SessionCb session_status_cb;
    session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                         size_t event_data_size) -> void
        { SessionCallbackHandler(event_type, event_data, event_data_size); };

    uint32_t session_id;
    ret = recorder_.CreateSession(session_status_cb, &session_id);
    assert(session_id > 0);
    assert(ret == NO_ERROR);

    VideoTrackCreateParam video_track_param;
    memset(&video_track_param, 0x0, sizeof video_track_param);

    video_track_param.camera_id     = camera_id_;
    video_track_param.width         = width;
    video_track_param.height        = height;
    video_track_param.frame_rate    = 30;
    video_track_param.format_type   = format_type;
    uint32_t video_track_id = 1;

    if (dump_bitstream_.IsEnabled()) {
      StreamDumpInfo dumpinfo = {
        format_type,
        video_track_id,
        width,
        height };
      ret = dump_bitstream_.SetUp(dumpinfo);
      assert(ret == NO_ERROR);
    }

    TrackCb video_track_cb;
    video_track_cb.data_cb = [&] (uint32_t track_id,
                              std::vector<BufferDescriptor> buffers,
                              std::vector<MetaData> meta_buffers) {
    VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

    video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
        void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
        event_type, event_data, event_data_size); };

    ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                      video_track_param, video_track_cb);
    assert(ret == NO_ERROR);

    std::vector<uint32_t> track_ids;
    track_ids.push_back(video_track_id);
    sessions_.insert(std::make_pair(session_id, track_ids));

    ret = recorder_.StartSession(session_id);
    assert(ret == NO_ERROR);

    // Let session run for kRecordDuration, during this time buffer with valid
    // data would be received in track callback (VideoTrackDataCb).
    sleep(kRecordDuration);

    ret = recorder_.StopSession(session_id, false);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteSession(session_id);
    assert(ret == NO_ERROR);

    ClearSessions();
  }
  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());

}

/*
* SessionWith4kp30fpsEncTrack: This test will test session with one 4k
* 30fps h264 track.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith4kp30fpsEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 3840;
  int32_t height = 2160;
  uint32_t fps = 30;

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // Let session run for kRecordDuration, during this time buffer with valid
  // data would be received in track callback (VideoTrackDataCb).
  sleep(kRecordDuration);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

   ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith4kp30fps4K1fpsSnapshotEncTrack: This test will test session with
*  one 4k 30fps h264 track and one 4K 1fps h264 track and snapshot.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - CaptureImage
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith4kp30fps4K1fpsSnapshotEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 3840;
  int32_t height = 2160;
  uint32_t fps = 30;

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;

  video_track_param.codec_param.avc.idr_interval = 1;
  video_track_param.codec_param.avc.bitrate      = 10000000;
  video_track_param.codec_param.avc.profile = AVCProfileType::kBaseline;
  video_track_param.codec_param.avc.level   = AVCLevelType::kLevel3;
  video_track_param.codec_param.avc.ratecontrol_type =
      VideoRateControlType::kVariableSkipFrames;
  video_track_param.codec_param.avc.qp_params.enable_init_qp = true;
  video_track_param.codec_param.avc.qp_params.init_qp.init_IQP = 51;
  video_track_param.codec_param.avc.qp_params.init_qp.init_PQP = 51;
  video_track_param.codec_param.avc.qp_params.init_qp.init_BQP = 51;
  video_track_param.codec_param.avc.qp_params.init_qp.init_QP_mode = 0x7;
  video_track_param.codec_param.avc.qp_params.enable_qp_range = true;
  video_track_param.codec_param.avc.qp_params.qp_range.min_QP = 26;
  video_track_param.codec_param.avc.qp_params.qp_range.max_QP = 51;
  video_track_param.codec_param.avc.qp_params.enable_qp_IBP_range = true;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.min_IQP = 26;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.max_IQP = 51;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.min_PQP = 26;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.max_PQP = 51;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.min_BQP = 26;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.max_BQP = 51;
  video_track_param.codec_param.avc.ltr_count = 4;
  video_track_param.codec_param.avc.insert_aud_delimiter = true;

  uint32_t video_track_id = 1;
  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);

  uint32_t video_track4K1fps_id = 2;
  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track4K1fps_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.frame_rate  = 1;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track4K1fps_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track4K1fps_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(30);

  //Take snapshot
  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 1920;
  image_param.height        = 1080;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;
  // Check Supported JPEG snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                  static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true;
          }
        }
      }
    }
  }
  assert (res_supported != false);

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer,
                              MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  meta_array.push_back(meta);
  ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
  assert(ret == NO_ERROR);

  sleep(5);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track4K1fps_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

   ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith4kp30fps4K1fps240p30fpsSnapshotEncTrack: This test will test
* session with one 4k 30fps h264 track, one 4K 1fps h264 track, one 432x240
*  30fps h264 track and snapshot.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - CaptureImage
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith4kp30fps4K1fps240p30fpsSnapshotEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 3840;
  int32_t height = 2160;
  uint32_t fps = 30;

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;

  video_track_param.codec_param.avc.idr_interval = 1;
  video_track_param.codec_param.avc.bitrate      = 10000000;
  video_track_param.codec_param.avc.profile = AVCProfileType::kBaseline;
  video_track_param.codec_param.avc.level   = AVCLevelType::kLevel3;
  video_track_param.codec_param.avc.ratecontrol_type =
      VideoRateControlType::kVariableSkipFrames;
  video_track_param.codec_param.avc.qp_params.enable_init_qp = true;
  video_track_param.codec_param.avc.qp_params.init_qp.init_IQP = 51;
  video_track_param.codec_param.avc.qp_params.init_qp.init_PQP = 51;
  video_track_param.codec_param.avc.qp_params.init_qp.init_BQP = 51;
  video_track_param.codec_param.avc.qp_params.init_qp.init_QP_mode = 0x7;
  video_track_param.codec_param.avc.qp_params.enable_qp_range = true;
  video_track_param.codec_param.avc.qp_params.qp_range.min_QP = 26;
  video_track_param.codec_param.avc.qp_params.qp_range.max_QP = 51;
  video_track_param.codec_param.avc.qp_params.enable_qp_IBP_range = true;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.min_IQP = 26;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.max_IQP = 51;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.min_PQP = 26;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.max_PQP = 51;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.min_BQP = 26;
  video_track_param.codec_param.avc.qp_params.qp_IBP_range.max_BQP = 51;
  video_track_param.codec_param.avc.ltr_count = 4;
  video_track_param.codec_param.avc.insert_aud_delimiter = true;

  uint32_t video_track_id = 1;
  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);

  uint32_t video_track4K1fps_id = 2;
  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track4K1fps_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.frame_rate  = 1;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track4K1fps_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track4K1fps_id);

  uint32_t video_track240p_id = 3;
  width = 432;
  height = 240;
  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track240p_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.width = width;
  video_track_param.height = height;
  video_track_param.frame_rate  = fps;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackThreeEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track240p_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track240p_id);

  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(30);

  //Take snapshot
  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 1920;
  image_param.height        = 1080;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;
  // Check Supported JPEG snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                  static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true;
          }
        }
      }
    }
  }
  assert (res_supported != false);

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer,
                              MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  meta_array.push_back(meta);
  ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
  assert(ret == NO_ERROR);

  sleep(5);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track4K1fps_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track240p_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith27Kp60fpsEncTrack: This test will test session with one 2.7K
* 60fps h264 track.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith27Kp60fpsEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 2704;
  int32_t height = 1520;
  uint32_t fps = 60;

  camera_start_params_.frame_rate = fps;
  camera_start_params_.setSensorVendorMode(6);
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // Let session run for kRecordDuration, during this time buffer with valid
  // data would be received in track callback (VideoTrackDataCb).
  sleep(kRecordDuration);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

   ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
 * SessionWith1080p120fpsSnapshotVSTABEncTrack: This test will test session with one 1080p
 * 120fps h264 track.
 * Api test sequence:
 *  - StartCamera
 *  - CreateSession
 *  - CreateVideoTrack
 *  - StartVideoTrack
 *  - Snapshot
 *  - StopSession
 *  - DeleteVideoTrack
 *  - DeleteSession
 *  - StopCamera
 */
TEST_F(RecorderGtest, SessionWith1080p120fpsSnapshotVSTABEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  uint32_t fps = 120;

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(10);
  //Enable VSTAB
  CameraMetadata meta;
  ret = recorder_.GetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  uint8_t vstab_mode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON;
  meta.update(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &vstab_mode, 1);
  ret = recorder_.SetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  //Continue recording
  sleep(20);

  //Take snapshot
  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 3840;
  image_param.height        = 2160;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;
  // Check Supported JPEG snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                    static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true;
          }
        }
      }
    }
  }
  assert (res_supported != false);

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer,
                              MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  meta_array.push_back(meta);
  ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
  assert(ret == NO_ERROR);

  sleep(5);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
 * SessionWith1080p120fps480p30fpsSnapshotEncTrack: This test will test session with one 1080p
 * 120fps h264 track.
 * Api test sequence:
 *  - StartCamera
 *  - CreateSession
 *  - CreateVideoTrack
 *  - StartVideoTrack
 *  - Snapshot
 *  - StopSession
 *  - DeleteVideoTrack
 *  - DeleteSession
 *  - StopCamera
 */
TEST_F(RecorderGtest, SessionWith1080p120fps480p30fpsSnapshotEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  uint32_t fps = 120;

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);

  memset(&video_track_param, 0x0, sizeof video_track_param);
  width  = 720;
  height = 480;
  fps = 30;
  uint32_t video_track480p_id = 2;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track480p_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  video_track_param.low_power_mode = true;

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track480p_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track480p_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(30);

  //Take snapshot
  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 3840;
  image_param.height        = 2160;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;
  // Check Supported JPEG snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                    static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true;
          }
        }
      }
    }
  }
  assert (res_supported != false);

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer,
                              MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  meta_array.push_back(meta);
  ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
  assert(ret == NO_ERROR);

  sleep(5);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track480p_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
 * SessionWith1080p120fps480p30fpsEncTrack: This test will test session with one 1080p
 * 120fps h264 track and preview 480p30fps.
 * Api test sequence:
 *  - StartCamera
 *  - CreateSession
 *  - CreateVideoTrack
 *  - StartVideoTrack
 *  - StopSession
 *  - DeleteVideoTrack
 *  - DeleteSession
 *  - StopCamera
 */
TEST_F(RecorderGtest, SessionWith1080p120fps480p30fpsEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  uint32_t fps = 120;

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);

  memset(&video_track_param, 0x0, sizeof video_track_param);
  width  = 720;
  height = 480;
  fps = 30;
  uint32_t video_track480p_id = 2;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track480p_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  video_track_param.low_power_mode = true;

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track480p_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track480p_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(30);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track480p_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith1080p120fpsEncTrack: This test will test session with one 1080p
* 120fps h264 track.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith1080p120fpsEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  uint32_t fps = 120;

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(30);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith1080p60fpsEncTrack: This test will test session with one 1080p
* 60fps h264 track.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith1080p60fpsEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  uint32_t fps = 60;

  camera_start_params_.frame_rate = fps;
  camera_start_params_.setSensorVendorMode(6);
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(30);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith4kp30fps480p30fpsEncTrack: This test will test session with one 4k
* 30fps h264 track and one 480p 30fps h264 track.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith4kp30fps480p30fpsEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 3840;
  int32_t height = 2160;
  uint32_t fps = 30;

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);

  memset(&video_track_param, 0x0, sizeof video_track_param);
  width  = 720;
  height = 480;
  uint32_t video_track480p_id = 2;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track480p_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track480p_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track480p_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(30);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track480p_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

   ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith4kp30fps480p30fpsVSTABEncTrack: This test will test session with one 4k
* 30fps h264 track and one 480p 30fps h264 track with VSTAB enabled.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith4kp30fps480p30fpsVSTABEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 3840;
  int32_t height = 2160;
  uint32_t fps = 30;

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);

  memset(&video_track_param, 0x0, sizeof video_track_param);
  width  = 720;
  height = 480;
  uint32_t video_track480p_id = 2;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track480p_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track480p_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track480p_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(10);
  //Enable VSTAB
  CameraMetadata meta;
  ret = recorder_.GetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  uint8_t vstab_mode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON;
  meta.update(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &vstab_mode, 1);
  ret = recorder_.SetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  //Continue recording
  sleep(20);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track480p_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

   ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith27Kp60fps480p30fpsEncTrack: This test will test session with one 2.7K
* 60fps h264 track and one 480p 30 fps h264 track.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith27Kp60fps480p30fpsEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 2704;
  int32_t height = 1520;
  uint32_t fps = 60;

  camera_start_params_.frame_rate = fps;
  camera_start_params_.setSensorVendorMode(6);
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);

  memset(&video_track_param, 0x0, sizeof video_track_param);
  width  = 720;
  height = 480;
  fps = 30;
  uint32_t video_track480p_id = 2;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track480p_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track480p_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track480p_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(30);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track480p_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

   ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith27Kp60fps480p30fpsVSTABEncTrack: This test will test session with
*  one 2.7K 60fps h264 track and one 480p 30 fps h264 track with VSTAB enabled.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith27Kp60fps480p30fpsVSTABEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 2704;
  int32_t height = 1520;
  uint32_t fps = 60;

  camera_start_params_.frame_rate = fps;
  camera_start_params_.setSensorVendorMode(6);
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);

  memset(&video_track_param, 0x0, sizeof video_track_param);
  width  = 720;
  height = 480;
  fps = 30;
  uint32_t video_track480p_id = 2;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track480p_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track480p_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track480p_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(10);
  //Enable VSTAB
  CameraMetadata meta;
  ret = recorder_.GetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  uint8_t vstab_mode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON;
  meta.update(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &vstab_mode, 1);
  ret = recorder_.SetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  //Continue recording
  sleep(20);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track480p_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

   ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith27Kp30fps480p30fpsEncTrack: This test will test session with one 2.7K
* 30fps h264 track and one 480p 30 fps h264 track.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith27Kp30fps480p30fpsEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 2704;
  int32_t height = 1520;
  uint32_t fps = 30;

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);

  memset(&video_track_param, 0x0, sizeof video_track_param);
  width  = 720;
  height = 480;
  uint32_t video_track480p_id = 2;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track480p_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track480p_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track480p_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(30);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track480p_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith1080p90fps480p30fpsEncTrack: This test will test session
* with one 1080p 90fps h264 track and one 480p 30fps h264 track.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartSession
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith1080p90fps480p30fpsEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  uint32_t fps = 90;

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);

  memset(&video_track_param, 0x0, sizeof video_track_param);
  width  = 720;
  height = 480;
  fps = 30;
  uint32_t video_track480p_id = 2;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track480p_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track480p_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track480p_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  //Record for some time
  sleep(30);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track480p_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith1080p60fps480p30fps4KSnapshotEncTrack: This test will test session
* with one 1080p 60fps h264 track and one 480p 30fps h264 track and a snapshot.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith1080p60fps480p30fpsSnapshotEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  uint32_t fps = 60;

  camera_start_params_.frame_rate = fps;
  camera_start_params_.setSensorVendorMode(6);
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);

  memset(&video_track_param, 0x0, sizeof video_track_param);
  width  = 720;
  height = 480;
  fps = 30;
  uint32_t video_track480p_id = 2;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track480p_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track480p_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(video_track480p_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  //Record for some time
  sleep(30);

  //Take snapshot
  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 1920;
  image_param.height        = 1080;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;
  // Check Supported JPEG snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                  static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true;
          }
        }
      }
    }
  }
  assert (res_supported != false);

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer,
                              MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  meta_array.push_back(meta);
  ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
  assert(ret == NO_ERROR);

  sleep(5);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track480p_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith480pEncTrack: This test will test session with one 480p
* 30fps h264 track.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith480pEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 720;
  int32_t height = 480;
  uint32_t fps = 30;

  camera_start_params_.frame_rate = fps;
  camera_start_params_.setSensorVendorMode(6);
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // Let session run for kRecordDuration, during this time buffer with valid
  // data would be received in track callback (VideoTrackDataCb).
  sleep(kRecordDuration);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

   ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith4KEncTrack: This test will test session with one 4K h264 track.
* Api test sequence:
*  - StartCamera
*   loop Start {
*   ------------------
*   - CreateSession
*   - CreateVideoTrack
*   - StartVideoTrack
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   ------------------
*   } loop End
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith4KEncTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 3840;
  int32_t height = 2160;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    SessionCb session_status_cb;
    session_status_cb.event_cb =
        [this] (EventType event_type, void *event_data,
                size_t event_data_size) -> void {
        SessionCallbackHandler(event_type,
        event_data, event_data_size); };

    uint32_t session_id;
    ret = recorder_.CreateSession(session_status_cb, &session_id);
    assert(session_id > 0);
    assert(ret == NO_ERROR);

    VideoTrackCreateParam video_track_param;
    memset(&video_track_param, 0x0, sizeof video_track_param);

    video_track_param.camera_id   = camera_id_;
    video_track_param.width       = width;
    video_track_param.height      = height;
    video_track_param.frame_rate  = 30;
    video_track_param.format_type = format_type;
    uint32_t video_track_id = 1;

    if (dump_bitstream_.IsEnabled()) {
      StreamDumpInfo dumpinfo = {
        format_type,
        video_track_id,
        width,
        height };
      ret = dump_bitstream_.SetUp(dumpinfo);
      assert(ret == NO_ERROR);
    }

    TrackCb video_track_cb;
    video_track_cb.data_cb = [&] (uint32_t track_id,
                                  std::vector<BufferDescriptor> buffers,
                                  std::vector<MetaData> meta_buffers) {
        VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

    video_track_cb.event_cb =
        [this] (uint32_t track_id, EventType event_type,
                void *event_data, size_t event_data_size) -> void
        { VideoTrackEventCb(track_id,
        event_type, event_data, event_data_size); };

    ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                      video_track_param, video_track_cb);
    assert(ret == NO_ERROR);

    std::vector<uint32_t> track_ids;
    track_ids.push_back(video_track_id);
    sessions_.insert(std::make_pair(session_id, track_ids));

    ret = recorder_.StartSession(session_id);
    assert(ret == NO_ERROR);

    // Let session run for kRecordDuration, during this time buffer with valid
    // data would be received in track callback (VideoTrackDataCb).
    sleep(kRecordDuration);

    ret = recorder_.StopSession(session_id, false);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteSession(session_id);
    assert(ret == NO_ERROR);

     ClearSessions();
  }
  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWithTwo1080pEncTracks: This test will test session with two 1080p
                                h264 tracks.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*   loop Start {
*   ------------------
*   - CreateVideoTrack 1
*   - CreateVideoTrack 2
*   - StartVideoTrack
*   - StopSession
*   - DeleteVideoTrack 1
*   - DeleteVideoTrack 2
*   ------------------
*   } loop End
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWithTwo1080pEncTracks) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  uint32_t video_track_id1 = 1;
  uint32_t video_track_id2 = 2;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void {
    SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id     = camera_id_;
  video_track_param.width         = width;
  video_track_param.height        = height;
  video_track_param.frame_rate    = 30;
  video_track_param.format_type   = format_type;

  TrackCb video_track_cb;
  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void {
      VideoTrackEventCb(track_id, event_type, event_data, event_data_size); };

  std::vector<uint32_t> track_ids;
  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    video_track_cb.data_cb = [&] (uint32_t track_id,
                                  std::vector<BufferDescriptor> buffers,
                                  std::vector<MetaData> meta_buffers) {
        VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

    // Create 1080p encode track.
    ret = recorder_.CreateVideoTrack(session_id, video_track_id1,
                                     video_track_param, video_track_cb);
    assert(ret == NO_ERROR);
    if (dump_bitstream_.IsEnabled()) {
      StreamDumpInfo dumpinfo = {
        format_type,
        video_track_id1,
        width,
        height };
      ret = dump_bitstream_.SetUp(dumpinfo);
      assert(ret == NO_ERROR);
    }
    track_ids.push_back(video_track_id1);

    video_track_cb.data_cb = [&] (uint32_t track_id,
                                  std::vector<BufferDescriptor> buffers,
                                  std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

    // Create another 1080p encode track.
    ret = recorder_.CreateVideoTrack(session_id, video_track_id2,
                                     video_track_param, video_track_cb);
    assert(ret == NO_ERROR);
    if (dump_bitstream_.IsEnabled()) {
      StreamDumpInfo dumpinfo = {
        format_type,
        video_track_id2,
        width,
        height };
      ret = dump_bitstream_.SetUp(dumpinfo);
    }

    track_ids.push_back(video_track_id2);

    sessions_.insert(std::make_pair(session_id, track_ids));

    ret = recorder_.StartSession(session_id);
    assert(ret == NO_ERROR);

    // Let session run for kRecordDuration, during this time buffer with valid
    // data would be received in track callback (VideoTrackDataCb).
    sleep(kRecordDuration);

    ret = recorder_.StopSession(session_id, false);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(session_id, video_track_id1);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(session_id, video_track_id2);
    assert(ret == NO_ERROR);
  }
  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith4KAnd1080pYUVTrack: This test will test session with 4k and 1080p
*                                YUV tracks.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  loop Start {
*   ------------------
*   - CreateVideoTrack 1
*   - CreateVideoTrack 2
*   - StartVideoTrack
*   - StopSession
*   - DeleteVideoTrack 1
*   - DeleteVideoTrack 2
*   ------------------
*   } loop End
*   - DeleteSession
*   - StopCamera
*/
TEST_F(RecorderGtest, SessionWith4KAnd1080pYUVTrack) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void {
      SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);
  uint32_t track1_id = 1;
  uint32_t track2_id = 2;

  std::vector<uint32_t> track_ids;
  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    VideoTrackCreateParam video_track_param;
    memset(&video_track_param, 0x0, sizeof video_track_param);

    video_track_param.camera_id   = camera_id_;
    video_track_param.width       = 3840;
    video_track_param.height      = 2160;
    video_track_param.frame_rate  = 30;
    video_track_param.format_type = VideoFormat::kYUV;

    TrackCb video_track_cb;
    video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackYUVDataCb(track_id, buffers, meta_buffers); };

    video_track_cb.event_cb =
        [this] (uint32_t track_id, EventType event_type,
                void *event_data, size_t event_data_size) -> void {
        VideoTrackEventCb(track_id, event_type, event_data, event_data_size); };

    ret = recorder_.CreateVideoTrack(session_id, track1_id,
                                      video_track_param, video_track_cb);
    assert(ret == NO_ERROR);
    track_ids.push_back(track1_id);

    video_track_param.width  = 1920;
    video_track_param.height = 1080;
    ret = recorder_.CreateVideoTrack(session_id, track2_id,
                                      video_track_param, video_track_cb);
    assert(ret == NO_ERROR);

    track_ids.push_back(track2_id);

    sessions_.insert(std::make_pair(session_id, track_ids));

    ret = recorder_.StartSession(session_id);
    assert(ret == NO_ERROR);

    // Let session run for kRecordDuration, during this time buffer with valid
    // data would be received in track callback (VideoTrackYUVDataCb).
    sleep(kRecordDuration);

    ret = recorder_.StopSession(session_id, false);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(session_id, track1_id);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(session_id, track2_id);
    assert(ret == NO_ERROR);
  }

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWithLPM1080pEncYUVSnapshot: This is a multi-session usecase that will
*                                    test LPM, Encode, Snapshot in parallel.
* Api test sequence:
*  - StartCamera
*   loop Start {
*   ------------------
*   - CaptureImage - 1080p Raw YUV
*   - StartSession - LPM (1080p YUV)
*   - CaptureImage - 1080p Raw YUV
*   - StartSession - 1080p Enc (AVC)
*   - CaptureImage - 1080p Raw YUV
*   - StopSession  - 1080p Enc
*   - CaptureImage - 1080p Raw YUV
*   - StopSession  - LPM
*   ------------------
*   } loop End
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWithLPM1080pEncYUVSnapshot) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  // Init and Start
  auto ret = Init();
  assert(ret == NO_ERROR);

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    // Take Snapshot
    fprintf(stderr, "Taking Snapshot\n");
    TEST_INFO("%s:%s: Taking Snapshot", TAG, __func__);

    ImageParam image_param;
    memset(&image_param, 0x0, sizeof image_param);
    image_param.width         = 1920;
    image_param.height        = 1080;
    image_param.image_format  = ImageFormat::kNV12;

    std::vector<CameraMetadata> meta_array;
    camera_metadata_entry_t entry;
    CameraMetadata meta;

    ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
    assert(ret == NO_ERROR);

    bool res_supported = false;
    // Check Supported Raw YUV snapshot resolutions.
    if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
      entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
      for (uint32_t i = 0 ; i < entry.count; i += 4) {
        if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
          if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
              entry.data.i32[i+3]) {
            if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
                && image_param.height ==
                    static_cast<uint32_t>(entry.data.i32[i+2])) {
              res_supported = true; // 1080p-YUV res supported.
            }
          }
        }
      }
    }
    assert (res_supported != false);

    ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                                BufferDescriptor buffer,
                                MetaData meta_data) -> void
        { SnapshotCb(camera_id, image_count, buffer, meta_data); };

    uint8_t awb_mode = ANDROID_CONTROL_AWB_MODE_INCANDESCENT;
    ret = meta.update(ANDROID_CONTROL_AWB_MODE, &awb_mode, 1);
    assert(ret == NO_ERROR);

    meta_array.push_back(meta);
    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                 cb);
    assert(ret == NO_ERROR);
    sleep(1);

    // Start 1080p YUV LPM Stream
    fprintf(stderr, "Starting LPM Stream\n");
    TEST_INFO("%s:%s: Starting LPM Stream", TAG, __func__);

    SessionCb s1_status_cb;
    s1_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                         size_t event_data_size) -> void
        { SessionCallbackHandler(event_type, event_data, event_data_size); };

    uint32_t s1_id;
    ret = recorder_.CreateSession(s1_status_cb, &s1_id);
    assert(s1_id > 0);
    assert(ret == NO_ERROR);

    VideoTrackCreateParam s1_video_t1_param;
    memset(&s1_video_t1_param, 0x0, sizeof s1_video_t1_param);

    s1_video_t1_param.camera_id      = camera_id_;
    s1_video_t1_param.width          = 1920;
    s1_video_t1_param.height         = 1080;
    s1_video_t1_param.frame_rate     = 30;
    s1_video_t1_param.format_type    = VideoFormat::kYUV;
    s1_video_t1_param.low_power_mode = true;  // LPM Stream

    uint32_t s1_video_t1_id = 1;

    TrackCb s1_video_t1_cb;
    s1_video_t1_cb.data_cb = [&] (uint32_t track_id,
                                  std::vector<BufferDescriptor> buffers,
                                  std::vector<MetaData> meta_buffers) {
        VideoTrackYUVDataCb(track_id, buffers, meta_buffers); };

    s1_video_t1_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
        void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
        event_type, event_data, event_data_size); };

    ret = recorder_.CreateVideoTrack(s1_id, s1_video_t1_id,
                                      s1_video_t1_param, s1_video_t1_cb);
    assert(ret == NO_ERROR);

    std::vector<uint32_t> s1_track_ids;
    s1_track_ids.push_back(s1_video_t1_id);
    sessions_.insert(std::make_pair(s1_id, s1_track_ids));

    ret = recorder_.StartSession(s1_id);
    assert(ret == NO_ERROR);
    sleep(5);

    // Take Snapshot
    fprintf(stderr, "Taking Snapshot\n");
    TEST_INFO("%s:%s: Taking Snapshot", TAG, __func__);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                   cb);
    assert(ret == NO_ERROR);
    sleep(1);

    // Start 1080p AVC Stream
    fprintf(stderr, "Starting Enc Stream\n");
    TEST_INFO("%s:%s: Starting Enc Stream", TAG, __func__);

    SessionCb s2_status_cb;
    s2_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                         size_t event_data_size) -> void
        { SessionCallbackHandler(event_type, event_data, event_data_size); };

    uint32_t s2_id;
    ret = recorder_.CreateSession(s2_status_cb, &s2_id);
    assert(s2_id > 0);
    assert(ret == NO_ERROR);

    VideoTrackCreateParam s2_video_t1_param;
    memset(&s2_video_t1_param, 0x0, sizeof s2_video_t1_param);

    s2_video_t1_param.camera_id      = camera_id_;
    s2_video_t1_param.width          = 1920;
    s2_video_t1_param.height         = 1080;
    s2_video_t1_param.frame_rate     = 30;
    s2_video_t1_param.format_type    = VideoFormat::kAVC;
    s2_video_t1_param.low_power_mode = false;

    uint32_t s2_video_t1_id = 1;

    TrackCb s2_video_t1_cb;
    s2_video_t1_cb.data_cb = [&] (uint32_t track_id,
                                  std::vector<BufferDescriptor> buffers,
                                  std::vector<MetaData> meta_buffers) {
        VideoTrackYUVDataCb(track_id, buffers, meta_buffers); };

    s2_video_t1_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
        void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
        event_type, event_data, event_data_size); };

    ret = recorder_.CreateVideoTrack(s2_id, s2_video_t1_id,
                                      s2_video_t1_param, s2_video_t1_cb);
    assert(ret == NO_ERROR);

    std::vector<uint32_t> s2_track_ids;
    s2_track_ids.push_back(s2_video_t1_id);
    sessions_.insert(std::make_pair(s2_id, s2_track_ids));

    ret = recorder_.StartSession(s2_id);
    assert(ret == NO_ERROR);
    sleep(5);

    // Take Snapshot
    fprintf(stderr, "Taking Snapshot\n");
    TEST_INFO("%s:%s: Taking Snapshot", TAG, __func__);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                   cb);
    assert(ret == NO_ERROR);
    sleep(1);

    // Delete 1080p AVC Stream
    fprintf(stderr, "Stopping Enc Stream\n");
    TEST_INFO("%s:%s: Stopping Enc Stream", TAG, __func__);

    ret = recorder_.StopSession(s2_id, false);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(s2_id, s2_video_t1_id);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteSession(s2_id);
    assert(ret == NO_ERROR);
    sleep(1);

    // Take Snapshot
    fprintf(stderr, "Taking Snapshot\n");
    TEST_INFO("%s:%s: Taking Snapshot", TAG, __func__);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                   cb);
    assert(ret == NO_ERROR);
    sleep(1);

    // Delete 1080p YUV LPM Stream
    fprintf(stderr, "Stopping LPM Stream\n");
    TEST_INFO("%s:%s: Starting LPM Stream", TAG, __func__);

    ret = recorder_.StopSession(s1_id, false);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(s1_id, s1_video_t1_id);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteSession(s1_id);
    assert(ret == NO_ERROR);
    sleep(1);

    // Take Snapshot
    fprintf(stderr, "Taking Snapshot\n");
    TEST_INFO("%s:%s: Taking Snapshot", TAG, __func__);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                                   cb);
    assert(ret == NO_ERROR);
    sleep(1);
  }  // End-for (iteration_count_)

  // Deinit and Stop
  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 1080pEncWithStaticImageOverlay: This test will apply static image overlay
*                                 ontop of 1080 video.
* Api test sequence:
*  - StartCamera
*   - CreateSession
*   - CreateVideoTrack
*   - StartVideoTrack
*   - CreateOverlayObject
*   - SetOverlay
*   loop Start {
*   ------------------
*    - GetOverlayObjectParams
*    - UpdateOverlayObjectParams
*   ------------------
*   } loop End
*   - RemoveOverlay
*   - DeleteOverlayObject
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   - StopCamera
*/
TEST_F(RecorderGtest, 1080pEncWithStaticImageOverlay) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = 30;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // Create Static Image type overlay.
  OverlayParam object_params;
  uint32_t static_img_id;
  memset(&object_params, 0x0, sizeof object_params);
  object_params.type = OverlayType::kStaticImage;
  object_params.location = OverlayLocationType::kBottomRight;
  std::string str("/etc/overlay_test.rgba");
  str.copy(object_params.image_info.image_location, str.length());
  object_params.image_info.width  = 451;
  object_params.image_info.height = 109;
  ret = recorder_.CreateOverlayObject(video_track_id, object_params,
                                      &static_img_id);
  assert(ret == 0);
  // Apply overlay object on video track.
  ret = recorder_.SetOverlay(video_track_id, static_img_id);
  assert(ret == 0);
  for(uint32_t i = 1, location = 0; i <= iteration_count_; ++i, ++location) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
      test_info_->name(), i);

    memset(&object_params, 0x0, sizeof object_params);
    ret = recorder_.GetOverlayObjectParams(video_track_id, static_img_id,
                                           object_params);
    assert(ret == 0);

    if (location == 0) {
      object_params.location = OverlayLocationType::kTopLeft;
    } else if (location == 1) {
      object_params.location = OverlayLocationType::kTopRight;
    } else if (location == 2) {
      object_params.location = OverlayLocationType::kCenter;
    } else if (location == 3) {
      object_params.location = OverlayLocationType::kBottomLeft;
    } else if (location == 4) {
      object_params.location = OverlayLocationType::kBottomRight;
    } else {
      location = -1;
    }

    ret = recorder_.UpdateOverlayObjectParams(video_track_id, static_img_id,
                                              object_params);
    assert(ret == 0);
    // Record video with overlay.
    sleep(5);
  }
  // Remove overlay object from video track.
  ret = recorder_.RemoveOverlay(video_track_id, static_img_id);
  assert(ret == 0);

  // Delete overlay object.
  ret = recorder_.DeleteOverlayObject(video_track_id, static_img_id);
  assert(ret == 0);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}


/*
* 1080pEncWithDateAndTimeOverlay: This test applies date and time overlay type
*                                 ontop of 1080 video.
* Api test sequence:
*  - StartCamera
*   - CreateSession
*   - CreateVideoTrack
*   - StartVideoTrack
*   - CreateOverlayObject
*   - SetOverlay
*   loop Start {
*   ------------------
*    - GetOverlayObjectParams
*    - UpdateOverlayObjectParams
*   ------------------
*   } loop End
*   - RemoveOverlay
*   - DeleteOverlayObject
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   - StopCamera
*/
TEST_F(RecorderGtest, 1080pEncWithDateAndTimeOverlay) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = 30;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // Create Date & Time type overlay.
  OverlayParam object_params;
  memset(&object_params, 0x0, sizeof object_params);
  object_params.type = OverlayType::kDateType;
  object_params.location = OverlayLocationType::kBottomLeft;
  object_params.color    = COLOR_DARK_GRAY;
  object_params.date_time.time_format = OverlayTimeFormatType::kHHMMSS_AMPM;
  object_params.date_time.date_format = OverlayDateFormatType::kMMDDYYYY;

  uint32_t date_time_id;
  ret = recorder_.CreateOverlayObject(video_track_id, object_params,
                                       &date_time_id);
  assert(ret == 0);
  // One track can have multiple types of overlay.
  ret = recorder_.SetOverlay(video_track_id, date_time_id);
  assert(ret == 0);
  for(uint32_t i = 1, location = 0; i <= iteration_count_; ++i, ++location) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
      test_info_->name(), i);

    memset(&object_params, 0x0, sizeof object_params);
    ret = recorder_.GetOverlayObjectParams(video_track_id, date_time_id,
                                           object_params);
    assert(ret == 0);

    // Update different types of Time & Date formats along with text color
    // and location on video.
    if (location == 0) {
      object_params.location = OverlayLocationType::kTopLeft;
      object_params.date_time.time_format = OverlayTimeFormatType::kHHMMSS_AMPM;
      object_params.date_time.date_format = OverlayDateFormatType::kMMDDYYYY;
      object_params.color    = COLOR_DARK_GRAY;
    } else if (location == 1) {
      object_params.location = OverlayLocationType::kTopRight;
      object_params.date_time.time_format = OverlayTimeFormatType::kHHMMSS_24HR;
      object_params.date_time.date_format = OverlayDateFormatType::kMMDDYYYY;
      object_params.color    = COLOR_YELLOW;
    } else if (location == 2) {
      object_params.location = OverlayLocationType::kCenter;
      object_params.date_time.time_format = OverlayTimeFormatType::kHHMM_24HR;
      object_params.date_time.date_format = OverlayDateFormatType::kYYYYMMDD;
      object_params.color    = COLOR_BLUE;
    } else if (location == 3) {
      object_params.location = OverlayLocationType::kBottomLeft;
      object_params.date_time.time_format = OverlayTimeFormatType::kHHMM_AMPM;
      object_params.date_time.date_format = OverlayDateFormatType::kYYYYMMDD;
      object_params.color    = COLOR_WHITE;
    } else if (location == 4) {
      object_params.location = OverlayLocationType::kBottomRight;
      object_params.date_time.time_format = OverlayTimeFormatType::kHHMMSS_AMPM;
      object_params.date_time.date_format = OverlayDateFormatType::kYYYYMMDD;
      object_params.color    = COLOR_ORANGE;
    } else {
      location = -1;
    }

    ret = recorder_.UpdateOverlayObjectParams(video_track_id, date_time_id,
                                              object_params);
    assert(ret == 0);
    // Record video with overlay.
    sleep(5);
  }

  ret = recorder_.RemoveOverlay(video_track_id, date_time_id);
  assert(ret == 0);

  // Delete overlay object.
  ret = recorder_.DeleteOverlayObject(video_track_id, date_time_id);
  assert(ret == 0);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}


/*
* 1080pEncWithBoundingBoxOverlay: This test applies bounding box overlay type
*                                 ontop of 1080 video.
* Api test sequence:
*  - StartCamera
*   - CreateSession
*   - CreateVideoTrack
*   - StartVideoTrack
*   - CreateOverlayObject
*   - SetOverlay
*   loop Start {
*   ------------------
*    - GetOverlayObjectParams
*    - UpdateOverlayObjectParams
*   ------------------
*   } loop End
*   - RemoveOverlay
*   - DeleteOverlayObject
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   - StopCamera
*/
TEST_F(RecorderGtest, 1080pEncWithBoundingBoxOverlay) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = 30;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // Create BoundingBox type overlay.
  OverlayParam object_params;
  memset(&object_params, 0x0, sizeof object_params);
  object_params.type  = OverlayType::kBoundingBox;
  object_params.color = COLOR_LIGHT_GREEN;
  // Dummy coordinates for test purpose.
  object_params.bounding_box.start_x = 20;
  object_params.bounding_box.start_y = 20;
  object_params.bounding_box.width   = 400;
  object_params.bounding_box.height  = 200;
  std::string bb_text("Test BBox..");
  bb_text.copy(object_params.bounding_box.box_name, bb_text.length());

  uint32_t bbox_id;
  ret = recorder_.CreateOverlayObject(video_track_id, object_params,
                                       &bbox_id);
  assert(ret == 0);
  ret = recorder_.SetOverlay(video_track_id, bbox_id);
  assert(ret == 0);

  //Mimic moving bounding box.
  for (uint32_t j = 0; j < 100; ++j) {
    ret = recorder_.GetOverlayObjectParams(video_track_id, bbox_id,
                                           object_params);
    assert(ret == 0);

    object_params.bounding_box.start_x = ((object_params.bounding_box.start_x +
        object_params.bounding_box.width) < width) ?
        object_params.bounding_box.start_x + 5 : 20;

    object_params.bounding_box.width = ((object_params.bounding_box.start_x +
        object_params.bounding_box.width) < width) ?
        object_params.bounding_box.width + 5 : 200;

    object_params.bounding_box.start_y = ((object_params.bounding_box.start_y +
        object_params.bounding_box.height) < height) ?
        object_params.bounding_box.start_y + 2 : 20;

    object_params.bounding_box.height = ((object_params.bounding_box.start_y +
        object_params.bounding_box.height) < height) ?
        object_params.bounding_box.height + 2 : 100;

    ret = recorder_.UpdateOverlayObjectParams(video_track_id, bbox_id,
                                              object_params);
    assert(ret == 0);
    usleep(250000);
  }

  // Remove overlay object from video track.
  ret = recorder_.RemoveOverlay(video_track_id, bbox_id);
  assert(ret == 0);

  // Delete overlay object.
  ret = recorder_.DeleteOverlayObject(video_track_id, bbox_id);
  assert(ret == 0);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 4KEncWithBoundingBoxOverlay: This test applies bounding box overlay type
*                              ontop of 4K video.
* Api test sequence:
*  - StartCamera
*   - CreateSession
*   - CreateVideoTrack
*   - StartVideoTrack
*   - CreateOverlayObject
*   - SetOverlay
*   loop Start {
*   ------------------
*    - GetOverlayObjectParams
*    - UpdateOverlayObjectParams
*   ------------------
*   } loop End
*   - RemoveOverlay
*   - DeleteOverlayObject
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   - StopCamera
*/
TEST_F(RecorderGtest, 4KEncWithBoundingBoxOverlay) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 3840;
  int32_t height = 2160;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = 30;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // Create BoundingBox type overlay.
  OverlayParam object_params;
  memset(&object_params, 0x0, sizeof object_params);
  object_params.type  = OverlayType::kBoundingBox;
  object_params.color = COLOR_LIGHT_GREEN;
  // Dummy coordinates for test purpose.
  object_params.bounding_box.start_x = 40;
  object_params.bounding_box.start_y = 40;
  object_params.bounding_box.width   = 800;
  object_params.bounding_box.height  = 400;
  std::string bb_text("Test BBox..");
  bb_text.copy(object_params.bounding_box.box_name, bb_text.length());

  uint32_t bbox_id;
  ret = recorder_.CreateOverlayObject(video_track_id, object_params,
                                       &bbox_id);
  assert(ret == 0);
  ret = recorder_.SetOverlay(video_track_id, bbox_id);
  assert(ret == 0);

  //Mimic moving bounding box.
  for (uint32_t j = 0; j < 100; ++j) {
    ret = recorder_.GetOverlayObjectParams(video_track_id, bbox_id,
                                           object_params);
    assert(ret == 0);

    object_params.bounding_box.start_x = ((object_params.bounding_box.start_x +
        object_params.bounding_box.width) < width) ?
        object_params.bounding_box.start_x + 5 : 20;

    object_params.bounding_box.width = ((object_params.bounding_box.start_x +
        object_params.bounding_box.width) < width) ?
        object_params.bounding_box.width + 5 : 200;

    object_params.bounding_box.start_y = ((object_params.bounding_box.start_y +
        object_params.bounding_box.height) < height) ?
        object_params.bounding_box.start_y + 2 : 20;

    object_params.bounding_box.height = ((object_params.bounding_box.start_y +
        object_params.bounding_box.height) < height) ?
        object_params.bounding_box.height + 2 : 100;

    ret = recorder_.UpdateOverlayObjectParams(video_track_id, bbox_id,
                                              object_params);
    assert(ret == 0);
    usleep(250000);
  }

  // Remove overlay object from video track.
  ret = recorder_.RemoveOverlay(video_track_id, bbox_id);
  assert(ret == 0);

  // Delete overlay object.
  ret = recorder_.DeleteOverlayObject(video_track_id, bbox_id);
  assert(ret == 0);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}
/*
* 1080pEncWithUserTextOverlay: This test applies custom user text ontop of 1080
*                              video.
* Api test sequence:
*  - StartCamera
*   - CreateSession
*   - CreateVideoTrack
*   - StartVideoTrack
*   - CreateOverlayObject
*   - SetOverlay
*   loop Start {
*   ------------------
*    - GetOverlayObjectParams
*    - UpdateOverlayObjectParams
*   ------------------
*   } loop End
*   - RemoveOverlay
*   - DeleteOverlayObject
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   - StopCamera
*/
TEST_F(RecorderGtest, 1080pEncWithUserTextOverlay) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = 30;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // Create UserText type overlay.
  OverlayParam object_params;
  memset(&object_params, 0x0, sizeof object_params);
  object_params.type = OverlayType::kUserText;
  object_params.location = OverlayLocationType::kTopRight;
  object_params.color    = COLOR_LIGHT_BLUE;
  std::string user_text("Simple User Text For Testing!!");
  user_text.copy(object_params.user_text, user_text.length());

  uint32_t user_text_id;
  ret = recorder_.CreateOverlayObject(video_track_id, object_params,
                                       &user_text_id);
  assert(ret == 0);
  ret = recorder_.SetOverlay(video_track_id, user_text_id);
  assert(ret == 0);

  for(uint32_t i = 1, location = 0; i <= iteration_count_; ++i, ++location) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
      test_info_->name(), i);

    memset(&object_params, 0x0, sizeof object_params);
    ret = recorder_.GetOverlayObjectParams(video_track_id, user_text_id,
                                           object_params);
    assert(ret == 0);

    // Update custom with text color and location on video.
    if (location == 0) {
      object_params.location = OverlayLocationType::kTopLeft;
      object_params.color    = COLOR_LIGHT_BLUE;
      std::string user_text("TopLeft:Simple User Text!!");
      user_text.copy(object_params.user_text, user_text.length());
    } else if (location == 1) {
      object_params.location = OverlayLocationType::kTopRight;
      object_params.color    = COLOR_YELLOW;
      std::string user_text("TopRight:Simple User Text!!");
      user_text.copy(object_params.user_text, user_text.length());
    } else if (location == 2) {
      object_params.location = OverlayLocationType::kCenter;
      object_params.color    = COLOR_BLUE;
      std::string user_text("Center:Simple User Text!!");
      user_text.copy(object_params.user_text, user_text.length());
    } else if (location == 3) {
      object_params.location = OverlayLocationType::kBottomLeft;
      object_params.color    = COLOR_WHITE;
      std::string user_text("BottomLeft:Simple User Text!!");
      user_text.copy(object_params.user_text, user_text.length());
    } else if (location == 4) {
      object_params.location = OverlayLocationType::kBottomRight;
      object_params.color    = COLOR_ORANGE;
      std::string user_text("BottomRight:Simple User Text!!");
      user_text.copy(object_params.user_text, user_text.length());
    } else {
      location = -1;
    }

    ret = recorder_.UpdateOverlayObjectParams(video_track_id, user_text_id,
                                              object_params);
    assert(ret == 0);
    // Record video with overlay.
    sleep(5);
  }

  // Remove overlay object from video track.
  ret = recorder_.RemoveOverlay(video_track_id, user_text_id);
  assert(ret == 0);

  // Delete overlay object.
  ret = recorder_.DeleteOverlayObject(video_track_id, user_text_id);
  assert(ret == 0);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 1080pEncWithPrivacyMaskOverlay: This test applies privacy mask overlay type
*                                 ontop of 1080 video.
* Api test sequence:
*  - StartCamera
*   - CreateSession
*   - CreateVideoTrack
*   - StartVideoTrack
*   - CreateOverlayObject
*   - SetOverlay
*   loop Start {
*   ------------------
*    - GetOverlayObjectParams
*    - UpdateOverlayObjectParams
*   ------------------
*   } loop End
*   - RemoveOverlay
*   - DeleteOverlayObject
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   - StopCamera
*/
TEST_F(RecorderGtest, 1080pEncWithPrivacyMaskOverlay) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = 30;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // Create BoundingBox type overlay.
  OverlayParam object_params;
  memset(&object_params, 0x0, sizeof object_params);
  object_params.type  = OverlayType::kPrivacyMask;
  object_params.color = 0xFF9933FF; //Fill mask with color.
  // Dummy coordinates for test purpose.
  object_params.bounding_box.start_x = 20;
  object_params.bounding_box.start_y = 40;
  object_params.bounding_box.width   = 1920/8;
  object_params.bounding_box.height  = 1080/8;

  uint32_t mask_id;
  ret = recorder_.CreateOverlayObject(video_track_id, object_params,
                                       &mask_id);
  assert(ret == 0);
  ret = recorder_.SetOverlay(video_track_id, mask_id);
  assert(ret == 0);

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
      test_info_->name(), i);

    for (uint32_t j = 0; j < 20; ++j) {
      ret = recorder_.GetOverlayObjectParams(video_track_id, mask_id,
                                             object_params);
      assert(ret == 0);

      object_params.bounding_box.start_x = (object_params.bounding_box.start_x +
          object_params.bounding_box.width < 1920) ?
          object_params.bounding_box.start_x + 20 : 20;

      object_params.bounding_box.width = (object_params.bounding_box.start_x +
          object_params.bounding_box.width < 1920) ?
          object_params.bounding_box.width + 50 : 1920/8;

      object_params.bounding_box.start_y = (object_params.bounding_box.start_y +
          object_params.bounding_box.height < 1080) ?
          object_params.bounding_box.start_y + 10 : 40;

      object_params.bounding_box.height = (object_params.bounding_box.start_y +
          object_params.bounding_box.height < 1080) ?
          object_params.bounding_box.height + 50 : 1080/8;

      ret = recorder_.UpdateOverlayObjectParams(video_track_id, mask_id,
                                                object_params);
      assert(ret == 0);
      usleep(250000);
    }

  }
  // Remove overlay object from video track.
  ret = recorder_.RemoveOverlay(video_track_id, mask_id);
  assert(ret == 0);

  // Delete overlay object.
  ret = recorder_.DeleteOverlayObject(video_track_id, mask_id);
  assert(ret == 0);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith1080pEncTrackStartStop: This test will test session with 1080p
* h264 track.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*   loop Start {
*   ------------------
*   - StartVideoTrack
*   - StopSession
*   ------------------
*   } loop End
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith1080pEncTrackStartStop) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void
      { SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id     = camera_id_;
  video_track_param.width         = width;
  video_track_param.height        = height;
  video_track_param.frame_rate    = 30;
  video_track_param.format_type   = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void {
      VideoTrackEventCb(track_id, event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.StartSession(session_id);
    assert(ret == NO_ERROR);

    // Let session run for kRecordDuration, during this time buffer with valid
    // data would be received in track callback (VideoTrackDataCb).
    sleep(kRecordDuration);

    ret = recorder_.StopSession(session_id, false);
    assert(ret == NO_ERROR);
  }

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith4KEncTrackStartStop: This test will test session with one 4K h264
*                                 track.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*   loop Start {
*   ------------------
*   - StartVideoTrack
*   - StopSession
*   ------------------
*   } loop End
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWith4KEncTrackStartStop) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 3840;
  int32_t height = 2160;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void
      { SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = 30;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void {
      VideoTrackEventCb(track_id, event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.StartSession(session_id);
    assert(ret == NO_ERROR);

    // Let session run for kRecordDuration, during this time buffer with valid
    // data would be received in track callback (VideoTrackDataCb).
    sleep(kRecordDuration);

    ret = recorder_.StopSession(session_id, false);
    assert(ret == NO_ERROR);
  }
  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWith4KAnd1080pYUVTrackStartStop: This test will test session with 4k
*                                         and 1080p YUV tracks.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack 1
*  - CreateVideoTrack 2
*  loop Start {
*   ------------------
*   - StartVideoTrack
*   - StopSession
*   ------------------
*   } loop End
*   - DeleteVideoTrack 1
*   - DeleteVideoTrack 2
*   - DeleteSession
*   - StopCamera
*/
TEST_F(RecorderGtest, SessionWith4KAnd1080pYUVTrackStartStop) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void {
      SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);
  uint32_t track1_id = 1;
  uint32_t track2_id = 2;

  std::vector<uint32_t> track_ids;
  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = 3840;
  video_track_param.height      = 2160;
  video_track_param.frame_rate  = 30;
  video_track_param.format_type = VideoFormat::kYUV;

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackYUVDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void {
      VideoTrackEventCb(track_id, event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, track1_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);
  track_ids.push_back(track1_id);

  video_track_param.width  = 1920;
  video_track_param.height = 1080;
  ret = recorder_.CreateVideoTrack(session_id, track2_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  track_ids.push_back(track2_id);

  sessions_.insert(std::make_pair(session_id, track_ids));

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.StartSession(session_id);
    assert(ret == NO_ERROR);

    // Let session run for kRecordDuration, during this time buffer with valid
    // data would be received in track callback (VideoTrackYUVDataCb).
    sleep(kRecordDuration);

    ret = recorder_.StopSession(session_id, false);
    assert(ret == NO_ERROR);
  }
  ret = recorder_.DeleteVideoTrack(session_id, track1_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, track2_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* SessionWithTwo1080pEncTracksStartStop: This test will test session with two
*                                        1080p h264 tracks.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack 1
*  - CreateVideoTrack 2
*   loop Start {
*   ------------------
*   - StartVideoTrack
*   - StopSession
*   ------------------
*   } loop End
*  - DeleteVideoTrack 1
*  - DeleteVideoTrack 2
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SessionWithTwo1080pEncTracksStartStop) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  uint32_t video_track_id1 = 1;
  uint32_t video_track_id2 = 2;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void {
      SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id     = camera_id_;
  video_track_param.width         = width;
  video_track_param.height        = height;
  video_track_param.frame_rate    = 30;
  video_track_param.format_type   = format_type;

  TrackCb video_track_cb;
  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void {
      VideoTrackEventCb(track_id, event_type, event_data, event_data_size); };

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  // Create 1080p encode track.
  ret = recorder_.CreateVideoTrack(session_id, video_track_id1,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);
  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id1,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }
  track_ids.push_back(video_track_id1);

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  // Create another 1080p encode track.
  ret = recorder_.CreateVideoTrack(session_id, video_track_id2,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id2,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }
  track_ids.push_back(video_track_id1);

  sessions_.insert(std::make_pair(session_id, track_ids));

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.StartSession(session_id);
    assert(ret == NO_ERROR);

    // Let session run for kRecordDuration, during this time buffer with valid
    // data would be received in track callback (VideoTrackDataCb).
    sleep(kRecordDuration);

    ret = recorder_.StopSession(session_id, false);
    assert(ret == NO_ERROR);
  }

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id1);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id2);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* SingleSessionCameraParamTest: This test will test camera parameter setting.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartSessiom
*  - GetCameraParam
*  - SetCameraParam - Switch AWB mode
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, SingleSessionCameraParamTest) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  CameraResultCb result_cb = [&] (uint32_t camera_id,
      const CameraMetadata &result) {
    CameraResultCallbackHandler(camera_id, result); };
  ret = recorder_.StartCamera(camera_id_, camera_start_params_, result_cb);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [&] ( EventType event_type, void *event_data,
      size_t event_data_size) { SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = 1920;
  video_track_param.height      = 1080;
  video_track_param.frame_rate  = 30;
  video_track_param.format_type = VideoFormat::kYUV;
  uint32_t video_track_id = 1;

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackYUVDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
      void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // run session for some time
  sleep(5);

  int dump_fd = open(DUMP_META_PATH, O_WRONLY|O_CREAT);
  assert(0 <= dump_fd);

  CameraMetadata meta;
  ret = recorder_.GetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  meta.dump(dump_fd, 2);
  close(dump_fd);

  // Switch AWB mode
  uint8_t awb_mode = ANDROID_CONTROL_AWB_MODE_INCANDESCENT;
  ret = meta.update(ANDROID_CONTROL_AWB_MODE, &awb_mode, 1);
  assert(ret == NO_ERROR);

  ret = recorder_.SetCameraParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  // run session for some time
  sleep(5);

  ret = recorder_.StopSession(session_id, true);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* MultiSessionCameraParamTest: This testcase will verify camera parameter
* setting in multi session senarios.
* Api test sequence:
*  - StartCamera
*   - CreateSession
*   - CreateVideoTrack - 1080p
*   - StartSession
*   - SetCameraParameter - TNR
*   loop Start {
*   ------------------
*   - CreateSession
*   - CreateVideoTrack
*   - StartSession
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   - SetCameraParameter - TNR
*   ------------------
*   } loop End
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, MultiSessionCameraParamTest) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void
      { SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id1;
  ret = recorder_.CreateSession(session_status_cb, &session_id1);
  assert(session_id1 > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id     = camera_id_;
  video_track_param.width         = width;
  video_track_param.height        = height;
  video_track_param.frame_rate    = 30;
  video_track_param.format_type   = format_type;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                            std::vector<BufferDescriptor> buffers,
                            std::vector<MetaData> meta_buffers) {
  VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
      void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id1, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id1, track_ids));

  ret = recorder_.StartSession(session_id1);
  assert(ret == NO_ERROR);

  //Enable TNR - Fast mode.
  CameraMetadata meta;
  auto status = recorder_.GetCameraParam(camera_id_, meta);
  if (NO_ERROR == status) {
    if (meta.exists(ANDROID_NOISE_REDUCTION_MODE)) {
      const uint8_t tnr_mode = ANDROID_NOISE_REDUCTION_MODE_FAST;
      TEST_INFO("%s:%s Enable TNR mode(%d)", TAG, __func__, tnr_mode);
      meta.update(ANDROID_NOISE_REDUCTION_MODE, &tnr_mode, 1);
      status = recorder_.SetCameraParam(camera_id_, meta);
      assert(ret == NO_ERROR);
    }
  }

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    uint32_t session_id2;
    ret = recorder_.CreateSession(session_status_cb, &session_id2);
    assert(session_id2 > 0);
    assert(ret == NO_ERROR);

    uint32_t video_track_id2 = 2;
    TrackCb video_track_cb;
    video_track_cb.data_cb = [&] (uint32_t track_id,
                              std::vector<BufferDescriptor> buffers,
                              std::vector<MetaData> meta_buffers) {
     auto ret = recorder_.ReturnTrackBuffer(session_id2, track_id, buffers);
     assert(ret == NO_ERROR); };

    video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
        void *event_data, size_t event_data_size) { VideoTrackEventCb(track_id,
        event_type, event_data, event_data_size); };

    ret = recorder_.CreateVideoTrack(session_id2, video_track_id2,
                                      video_track_param, video_track_cb);
    assert(ret == NO_ERROR);

    ret = recorder_.StartSession(session_id2);
    assert(ret == NO_ERROR);

    // Let session run for kRecordDuration, during this time buffer with valid
    // data would be received in track callback (VideoTrackDataCb).
    sleep(kRecordDuration);

    ret = recorder_.StopSession(session_id2, false);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(session_id2, video_track_id2);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteSession(session_id2);
    assert(ret == NO_ERROR);

    //Enable TNR - High quality mode.
    CameraMetadata meta;
    auto status = recorder_.GetCameraParam(camera_id_, meta);
    if (NO_ERROR == status) {
      if (meta.exists(ANDROID_NOISE_REDUCTION_MODE)) {
        const uint8_t tnr_mode = ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY;
        TEST_INFO("%s:%s Enable TNR mode(%d)", TAG, __func__, tnr_mode);
        meta.update(ANDROID_NOISE_REDUCTION_MODE, &tnr_mode, 1);
        status = recorder_.SetCameraParam(camera_id_, meta);
        assert(ret == NO_ERROR);
      }
    }
  }

  sleep(kRecordDuration);

  ret = recorder_.StopSession(session_id1, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id1, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id1);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());

}

/*
* CancelCapture: This test will exercise CancelCapture Api, it will trigger
* CancelCaptureRequest api after submitting Burst Capture request.
* Api test sequence:
*  - StartCamera
*   loop Start {
*   --------------------
*  - CaptureImage - Burst of 30 images.
*  - CancelCaptureImage
*   ---------------------
*   } loop End
*  - StopCamera
*/
TEST_F(RecorderGtest, CancelCaptureImage) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  camera_start_params_.frame_rate = 30;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 3840;
  image_param.height        = 2160;
  image_param.image_format  = ImageFormat::kJPEG;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;
  // Check Supported Raw YUV snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                  static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true;
          }
        }
      }
    }
  }
  assert (res_supported != false);

  TEST_INFO("%s:%s: Running Test(%s)", TAG, __func__,
    test_info_->name());

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                                BufferDescriptor buffer,
                                MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  meta.update(ANDROID_JPEG_QUALITY, &kDefaultJpegQuality, 1);

  uint32_t num_images = 30;
  for (uint32_t i = 0; i < num_images; i++) {
    meta_array.push_back(meta);
  }

  for(uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);
    ret = recorder_.CaptureImage(camera_id_, image_param, num_images, meta_array,
                               cb);
    assert(ret == NO_ERROR);

    auto ran = std::rand() % 4;
    auto sleep_time = (ran == 0) ? ran : ran + 1;
    sleep(sleep_time);

    ret = recorder_.CancelCaptureImage(camera_id_);
    assert(ret == NO_ERROR);
  }

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 4KEncCancelCaptureImage: This test will exercise CancelCapture Api during 4K video
* record.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack - 4K
*  - StartSession
*   loop Start {
*   --------------------
*   - CaptureImage
*   - CancelCaptureImage
*   ---------------------
*   } loop End
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, 4KEncCancelCaptureImage) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  camera_start_params_.frame_rate = 30;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 3840;
  int32_t height = 2160;
  uint32_t fps = 30;

  SessionCb session_status_cb;
  session_status_cb.event_cb = [&] (EventType event_type, void *event_data,
      size_t event_data_size) { SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  video_track_param.low_power_mode = false;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
                                 void *event_data, size_t event_data_size) {
      VideoTrackEventCb(track_id, event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(3);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 3840;
  image_param.height        = 2160;
  image_param.image_format  = ImageFormat::kJPEG;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;
  // Check Supported Raw YUV snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                  static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true;
          }
        }
      }
    }
  }
  assert (res_supported != false);

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                                BufferDescriptor buffer,
                                MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  meta.update(ANDROID_JPEG_QUALITY, &kDefaultJpegQuality, 1);

  meta_array.push_back(meta);

  // take snapshot and cancel it in loop.
  for(uint32_t i = 1; i <= iteration_count_; i++) {

    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
    assert(ret == NO_ERROR);

    auto ran = std::rand() % 3;
    auto sleep_time = (ran == 0) ? ran : ran + 1;
    sleep(sleep_time);

    ret = recorder_.CancelCaptureImage(camera_id_);
    assert(ret == NO_ERROR);
  }

  ret = recorder_.StopSession(session_id, true);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 1080pEncCancelCaptureImage: This test will exercise CancelCapture Api during
*  two 1080p video record.
* record.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack - 1080p
*  - StartSession
*   loop Start {
*   --------------------
*   - CaptureImage
*   - CancelCaptureImage
*   ---------------------
*   } loop End
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, 1080pEncCanceCaptureImage) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  camera_start_params_.frame_rate = 30;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  uint32_t fps = 30;

  SessionCb session_status_cb;
  session_status_cb.event_cb = [&] (EventType event_type, void *event_data,
      size_t event_data_size) { SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  video_track_param.low_power_mode = false;
  uint32_t video_track_id = 1;

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id,
      width,
      height };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  TrackCb video_track_cb;
  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  video_track_cb.event_cb = [&] (uint32_t track_id, EventType event_type,
                                 void *event_data, size_t event_data_size) {
      VideoTrackEventCb(track_id, event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  sleep(3);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 3840;
  image_param.height        = 2160;
  image_param.image_format  = ImageFormat::kJPEG;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;
  // Check Supported Raw YUV snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                  static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true;
          }
        }
      }
    }
  }
  assert (res_supported != false);

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                                BufferDescriptor buffer,
                                MetaData meta_data) -> void
      { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  meta.update(ANDROID_JPEG_QUALITY, &kDefaultJpegQuality, 1);

  meta_array.push_back(meta);

  // take snapshot and cancel it in loop.
  for(uint32_t i = 1; i <= iteration_count_; i++) {

    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
    assert(ret == NO_ERROR);

    auto ran = std::rand() % 3;
    auto sleep_time = (ran == 0) ? ran : ran + 1;
    sleep(sleep_time);

    ret = recorder_.CancelCaptureImage(camera_id_);
    assert(ret == NO_ERROR);
  }

  ret = recorder_.StopSession(session_id, true);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* 4KVideo480pVideoAnd4KSnapshot: This test will test session with 4K and 480p
*     Video tracks and parallel 4K Snapshot.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack : 4K
*  - CreateVideoTrack : 480p
*  - StartSession
*   loop Start {
*   ------------------
*   - CaptureImage : 4K
*   ------------------
*   } loop End
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteVideoTrack
*  - DeleteSession
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, 4KVideo480pVideoAnd4KSnapshot) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 3840;
  int32_t height = 2160;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void {
    SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  uint32_t video_track_id1 = 1;
  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id      = camera_id_;
  video_track_param.width          = width;
  video_track_param.height         = height;
  video_track_param.frame_rate     = 30;
  video_track_param.format_type    = format_type;
  video_track_param.low_power_mode = false;

  TrackCb video_track_cb;
  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void {
      VideoTrackEventCb(track_id, event_type, event_data, event_data_size); };

  std::vector<uint32_t> track_ids;

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                  std::vector<BufferDescriptor> buffers,
                                  std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  // Create 4K encode track.
  ret = recorder_.CreateVideoTrack(session_id, video_track_id1,
                                     video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id1,
      static_cast<int32_t>(video_track_param.width),
      static_cast<int32_t>(video_track_param.height) };
    ret = dump_bitstream_.SetUp(dumpinfo);
    assert(ret == NO_ERROR);
  }

  track_ids.push_back(video_track_id1);

  uint32_t video_track_id2 = 2;
  video_track_param.width         = 640;
  video_track_param.height        = 480;
  video_track_param.low_power_mode = false;

  video_track_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
    VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  // Create 480p encode track.
  ret = recorder_.CreateVideoTrack(session_id, video_track_id2,
                                   video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  if (dump_bitstream_.IsEnabled()) {
    StreamDumpInfo dumpinfo = {
      format_type,
      video_track_id2,
      static_cast<int32_t>(video_track_param.width),
      static_cast<int32_t>(video_track_param.height) };
    ret = dump_bitstream_.SetUp(dumpinfo);
  }

  track_ids.push_back(video_track_id2);

  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // Record for sometime
  sleep(5);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = 3840;
  image_param.height        = 2160;
  image_param.image_format  = ImageFormat::kJPEG;

  std::vector<CameraMetadata> meta_array;
  camera_metadata_entry_t entry;
  CameraMetadata meta;

  ret = recorder_.GetDefaultCaptureParam(camera_id_, meta);
  assert(ret == NO_ERROR);

  bool res_supported = false;

  // Check Supported Raw YUV snapshot resolutions.
  if (meta.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
    entry = meta.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);
    for (uint32_t i = 0 ; i < entry.count; i += 4) {
      if (HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED == entry.data.i32[i]) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i+3]) {
          if (image_param.width == static_cast<uint32_t>(entry.data.i32[i+1])
              && image_param.height ==
                  static_cast<uint32_t>(entry.data.i32[i+2])) {
            res_supported = true;
          }
        }
      }
    }
  }
  assert (res_supported != false);

  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer,
                              MetaData meta_data) -> void
    { SnapshotCb(camera_id, image_count, buffer, meta_data); };

  meta.update(ANDROID_JPEG_QUALITY, &kDefaultJpegQuality, 1);

  meta_array.push_back(meta);

  // take 4K snapshots after every 2 seconds while 4K & 480p recording
  // is going on.
  for(uint32_t i = 1; i <= iteration_count_; i++) {

    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
    assert(ret == NO_ERROR);

    sleep(2);
  }

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id1);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id2);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

/*
* EcodingPreBuffer1080p: This test will start caching 5 seconds
* of 1080p video ES. After an event(image capture) is triggered it will
* store the accumulated history along with 5 seconds of video after
* the event.
* Api test sequence:
*  - StartCamera
*  - CreateSession
*  - CreateVideoTrack
*  - StartVideoTrack
*  - CaptureImage
*  - StopSession
*  - DeleteVideoTrack
*  - DeleteSession
*  - StopCamera
*/
TEST_F(RecorderGtest, EncodingPreBuffer1080p) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 1920;
  int32_t height = 1080;
  uint32_t fps = 30;
  AVQueue *av_queue = NULL;
  size_t history_length_ms = 5000;
  size_t frame_duration_ms = 1000 / fps;
  size_t queue_size = (history_length_ms / frame_duration_ms) * 2;

  assert(0 < AVQueueInit(&av_queue, REALTIME, queue_size + 1, queue_size));

  camera_start_params_.frame_rate = fps;
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  SessionCb session_status_cb;
  session_status_cb.event_cb =
      [this] (EventType event_type, void *event_data,
              size_t event_data_size) -> void {
      SessionCallbackHandler(event_type,
      event_data, event_data_size); };

  uint32_t session_id;
  ret = recorder_.CreateSession(session_status_cb, &session_id);
  assert(session_id > 0);
  assert(ret == NO_ERROR);

  ImageParam image_param;
  memset(&image_param, 0x0, sizeof image_param);
  image_param.width         = width;
  image_param.height        = height;
  image_param.image_format  = ImageFormat::kJPEG;
  image_param.image_quality = 95;

  std::vector<CameraMetadata> meta_array;
  ImageCaptureCb cb = [this] (uint32_t camera_id, uint32_t image_count,
                              BufferDescriptor buffer, MetaData meta_data) ->
                              void { SnapshotCb(camera_id, image_count,
                                                buffer, meta_data); };

  VideoTrackCreateParam video_track_param;
  memset(&video_track_param, 0x0, sizeof video_track_param);

  video_track_param.camera_id   = camera_id_;
  video_track_param.width       = width;
  video_track_param.height      = height;
  video_track_param.frame_rate  = fps;
  video_track_param.format_type = format_type;
  uint32_t video_track_id = 1;

  StreamDumpInfo dumpinfo = {
    format_type,
    video_track_id,
    width,
    height };
  ret = dump_bitstream_.SetUp(dumpinfo);
  assert(ret == NO_ERROR);

  TrackCb video_track_cb;
  video_track_cb.data_cb =
      [&] (uint32_t track_id, std::vector<BufferDescriptor>
             buffers, std::vector<MetaData> meta_buffers) ->
             void { VideoCachedDataCb(track_id, buffers, meta_buffers,
                                      format_type, av_queue); };

  video_track_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void
      { VideoTrackEventCb(track_id,
      event_type, event_data, event_data_size); };

  ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                    video_track_param, video_track_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> track_ids;
  track_ids.push_back(video_track_id);
  sessions_.insert(std::make_pair(session_id, track_ids));

  ret = recorder_.StartSession(session_id);
  assert(ret == NO_ERROR);

  // Start filling in video cache
  sleep((history_length_ms / 1000) * 2);

  //Event trigger
  ret = recorder_.CaptureImage(camera_id_, image_param, 1, meta_array,
                               cb);
  assert(ret == NO_ERROR);

  //Cache history length of video after event trigger
  sleep(history_length_ms / 1000);

  ret = recorder_.StopSession(session_id, false);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
  assert(ret == NO_ERROR);

  ret = recorder_.DeleteSession(session_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DumpQueue(av_queue, dump_bitstream_.GetFileFd(1));
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();

  if (NULL != av_queue) {
    AVQueueFree(&av_queue, AVFreePacket);
    av_queue = NULL;
  }
  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* DynamicSessionAndTracksUpdateWithCamParams: This is a usecase which will test
*                                    adding or deleting tracks and sessions,
*                                    along with changing camera parameters such as
*                                    TNR and SHDR.
*   Properties              : [Default Values]
*   -------------------------------------------
*   PROP_TRACK1_WIDTH       : [3840]
*   PROP_TRACK1_HEIGHT      : [2160]
*   PROP_TRACK1_FPS         : [30]
*   PROP_TRACK2_WIDTH       : [1920]
*   PROP_TRACK2_HEIGHT      : [1080]
*   PROP_TRACK2_FPS         : [24]
*   PROP_CAM_PARAMS1        : bit 0: SHDR [0], bit 1: TNR [0]
*   PROP_CAM_PARAMS2        : bit 0: SHDR [0], bit 1: TNR [0]
*   PROP_TRACK1_DELETE      : [0]
*   PROP_SESSION2_CREATE    : [0]
*
* Api test sequence:
*  - StartCamera
*   ------------------
*   - StartSession - 4k AVC
*   - [Enable/Disable SHDR, TNR]
*   - [StartSession - 1080p AVC]
*   - [Enable/Disable SHDR, TNR]
*   - [StopSession  - 1080p]
*   - StopSession - 4k
*   ------------------
*   - Stop Camera
*/
TEST_F(RecorderGtest, DynamicSessionAndTracksUpdateWithCamParams) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  // Init
  auto ret = Init();
  assert(ret == NO_ERROR);

  // Fetch Properties
  uint32_t w1, h1, w2, h2;
  uint32_t fps1, fps2;
  bool shdr1, shdr2, is_shdr_supported;
  bool tnr1, tnr2, is_tnr_supported;
  bool is_t1_delete, is_s2_create;
  char prop_val[PROPERTY_VALUE_MAX];

  property_get(PROP_TRACK1_WIDTH, prop_val, "3840");
  w1 = atoi(prop_val);
  assert(0 != w1);
  property_get(PROP_TRACK1_HEIGHT, prop_val, "2160");
  h1 = atoi(prop_val);
  assert(0 != h1);
  property_get(PROP_TRACK2_WIDTH, prop_val, "1920");
  w2 = atoi(prop_val);
  assert(0 != w2);
  property_get(PROP_TRACK2_HEIGHT, prop_val, "1080");
  h2 = atoi(prop_val);
  assert(0 != h2);

  property_get(PROP_TRACK1_FPS, prop_val, "30");
  fps1 = atoi(prop_val);
  assert(0 != fps1);
  property_get(PROP_TRACK2_FPS, prop_val, "24");
  fps2 = atoi(prop_val);
  assert(0 != fps2);

  property_get(PROP_CAM_PARAMS1, prop_val, "0");
  shdr1 = atoi(prop_val) & 0x1;
  tnr1 = atoi(prop_val) & 0x2;
  property_get(PROP_CAM_PARAMS2, prop_val, "0");
  shdr2 = atoi(prop_val) & 0x1;
  tnr2 = atoi(prop_val) & 0x2;

  property_get(PROP_TRACK1_DELETE, prop_val, "0");
  is_t1_delete = (atoi(prop_val) == 0) ? false : true;
  property_get(PROP_SESSION2_CREATE, prop_val, "0");
  is_s2_create = (atoi(prop_val) == 0) ? false : true;

  fprintf(stderr, "\nw1:%d h1:%d fps1:%d w2:%d h2:%d fps2:%d "
          "shdr1:%d tnr1:%d shdr2:%d tnr2:%d\n",
          w1, h1, fps1, w2, h2, fps2, shdr1, tnr1, shdr2, tnr2);

  bool is_allowed = (!(is_t1_delete && is_s2_create));
  assert(true == is_allowed);

  // Start
  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  ret = recorder_.GetDefaultCaptureParam(camera_id_, static_info_);
  assert(ret == NO_ERROR);

  InitSupportedVHDRModes();
  is_shdr_supported = IsVHDRSupported();
  InitSupportedNRModes();
  is_tnr_supported = IsNRSupported();

  fprintf(stderr, "\nis_shdr_supported:%d is_tnr_supported:%d\n",
          is_shdr_supported, is_tnr_supported);

  // Create Session1
  SessionCb s1_status_cb;
  s1_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                       size_t event_data_size) -> void
      { SessionCallbackHandler(event_type, event_data, event_data_size); };

  uint32_t s1_id;
  ret = recorder_.CreateSession(s1_status_cb, &s1_id);
  assert(s1_id > 0);
  assert(ret == NO_ERROR);

  // Create 4k AVC Stream
  VideoTrackCreateParam video_track1;
  memset(&video_track1, 0x0, sizeof video_track1);

  video_track1.camera_id      = camera_id_;
  video_track1.width          = w1;
  video_track1.height         = h1;
  video_track1.frame_rate     = fps1;
  video_track1.format_type    = VideoFormat::kAVC;
  video_track1.low_power_mode = false;

  TrackCb video_track1_cb;
  video_track1_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void {
      VideoTrackEventCb(track_id, event_type, event_data, event_data_size); };

  video_track1_cb.data_cb = [&] (uint32_t track_id,
                                std::vector<BufferDescriptor> buffers,
                                std::vector<MetaData> meta_buffers) {
      VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

  uint32_t video_track1_id = 1;
  ret = recorder_.CreateVideoTrack(s1_id, video_track1_id,
                                   video_track1, video_track1_cb);
  assert(ret == NO_ERROR);

  std::vector<uint32_t> s1_track_ids;
  s1_track_ids.push_back(video_track1_id);
  sessions_.insert(std::make_pair(s1_id, s1_track_ids));

  // Start Session1
  ret = recorder_.StartSession(s1_id);
  assert(ret == NO_ERROR);
  sleep(5);

  // Camera Params
  CameraMetadata meta;
  ret = recorder_.GetCameraParam(camera_id_, meta);
  assert(NO_ERROR == ret);

  // SHDR
  if (is_shdr_supported) {
    if (shdr1) {
      fprintf(stderr, "\nSetting shdr1 ON..\n");
      const int32_t vhdrMode = QCAMERA3_VIDEO_HDR_MODE_ON;
      meta.update(QCAMERA3_VIDEO_HDR_MODE, &vhdrMode, 1);
    } else {
      fprintf(stderr, "\nSetting shdr1 OFF..\n");
      const int32_t vhdrMode = QCAMERA3_VIDEO_HDR_MODE_OFF;
      meta.update(QCAMERA3_VIDEO_HDR_MODE, &vhdrMode, 1);
    }
  }

  // TNR
  if (is_tnr_supported) {
    if (tnr1) {
      fprintf(stderr, "\nSetting TNR1 ON..\n");
      const uint8_t tnrMode = ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY;
      meta.update(ANDROID_NOISE_REDUCTION_MODE, &tnrMode, 1);
    } else {
      fprintf(stderr, "\nSetting TNR1 OFF..\n");
      const uint8_t tnrMode = ANDROID_NOISE_REDUCTION_MODE_OFF;
      meta.update(ANDROID_NOISE_REDUCTION_MODE, &tnrMode, 1);
    }
  }

  ret = recorder_.SetCameraParam(camera_id_, meta);
  assert(NO_ERROR == ret);

  sleep(15);

  if (!is_s2_create) {
    ret = recorder_.StopSession(s1_id, false);
    assert(ret == NO_ERROR);
  }

  if (is_t1_delete) {
    ret = recorder_.DeleteVideoTrack(s1_id, video_track1_id);
    assert(ret == NO_ERROR);
  }

  // Create 1080p AVC Stream
  VideoTrackCreateParam video_track2;
  memset(&video_track2, 0x0, sizeof video_track2);

  video_track2.camera_id      = camera_id_;
  video_track2.width          = w2;
  video_track2.height         = h2;
  video_track2.frame_rate     = fps2;
  video_track2.format_type    = VideoFormat::kAVC;
  video_track2.low_power_mode = false;

  TrackCb video_track2_cb;
  video_track2_cb.event_cb =
      [this] (uint32_t track_id, EventType event_type,
              void *event_data, size_t event_data_size) -> void {
      VideoTrackEventCb(track_id, event_type, event_data, event_data_size); };

  video_track2_cb.data_cb = [&] (uint32_t track_id,
        std::vector<BufferDescriptor> buffers,
        std::vector<MetaData> meta_buffers) {
      VideoTrackTwoEncDataCb(track_id, buffers, meta_buffers); };

  uint32_t video_track2_id = 2;

  // Create Session2
  uint32_t s2_id;
  if (is_s2_create) {
    SessionCb s2_status_cb;
    s2_status_cb.event_cb = [this] (EventType event_type, void *event_data,
                                         size_t event_data_size) -> void
        { SessionCallbackHandler(event_type, event_data, event_data_size); };

    ret = recorder_.CreateSession(s2_status_cb, &s2_id);
    assert(s2_id > 0);
    assert(ret == NO_ERROR);

    ret = recorder_.CreateVideoTrack(s2_id, video_track2_id,
                                     video_track2, video_track2_cb);
    assert(ret == NO_ERROR);

    std::vector<uint32_t> s2_track_ids;
    s2_track_ids.push_back(video_track2_id);
    sessions_.insert(std::make_pair(s2_id, s2_track_ids));

    ret = recorder_.StartSession(s2_id);
    assert(ret == NO_ERROR);

  } else {
    // Add T2 to the existing Session and restart Session
    ret = recorder_.CreateVideoTrack(s1_id, video_track2_id,
                                     video_track2, video_track2_cb);
    assert(ret == NO_ERROR);

    ClearSessions();
    std::vector<uint32_t> tracks;
    if (!is_t1_delete) {
      tracks.push_back(video_track1_id);
    }
    tracks.push_back(video_track2_id);
    sessions_.insert(std::make_pair(s1_id, tracks));

    ret = recorder_.StartSession(s1_id);
    assert(ret == NO_ERROR);
  }

  // Camera Params
  ret = recorder_.GetCameraParam(camera_id_, meta);
  assert(NO_ERROR == ret);

  // SHDR
  if (is_shdr_supported) {
    if (shdr2) {
      fprintf(stderr, "\nSetting shdr2 ON..\n");
      const int32_t vhdrMode = QCAMERA3_VIDEO_HDR_MODE_ON;
      meta.update(QCAMERA3_VIDEO_HDR_MODE, &vhdrMode, 1);
    } else {
      fprintf(stderr, "\nSetting shdr2 OFF..\n");
      const int32_t vhdrMode = QCAMERA3_VIDEO_HDR_MODE_OFF;
      meta.update(QCAMERA3_VIDEO_HDR_MODE, &vhdrMode, 1);
    }
  }

  // TNR
  if (is_tnr_supported) {
    if (tnr2) {
      fprintf(stderr, "\nSetting TNR2 ON..\n");
      const uint8_t tnrMode = ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY;
      meta.update(ANDROID_NOISE_REDUCTION_MODE, &tnrMode, 1);
    } else {
      fprintf(stderr, "\nSetting TNR2 OFF..\n");
      const uint8_t tnrMode = ANDROID_NOISE_REDUCTION_MODE_OFF;
      meta.update(ANDROID_NOISE_REDUCTION_MODE, &tnrMode, 1);
    }
  }

  ret = recorder_.SetCameraParam(camera_id_, meta);
  assert(NO_ERROR == ret);
  sleep(15);

  // Stop Session-2
  if (is_s2_create) {
    ret = recorder_.StopSession(s2_id, false);
    assert(ret == NO_ERROR);
    ret = recorder_.DeleteVideoTrack(s2_id, video_track2_id);
    assert(ret == NO_ERROR);
    ret = recorder_.DeleteSession(s2_id);
    assert(ret == NO_ERROR);
  }

  // Stop Session-1
  ret = recorder_.StopSession(s1_id, false);
  assert(ret == NO_ERROR);

  if (!is_t1_delete) {
    ret = recorder_.DeleteVideoTrack(s1_id, video_track1_id);
    assert(ret == NO_ERROR);
  }

  if (!is_s2_create) {
    ret = recorder_.DeleteVideoTrack(s1_id, video_track2_id);
    assert(ret == NO_ERROR);
  }

  ret = recorder_.DeleteSession(s1_id);
  assert(ret == NO_ERROR);

  ClearSessions();

  // Deinit and Stop
  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
          test_info_->test_case_name(), test_info_->name());
}

/*
* DynamicFloatingFrameRate: This test will test session with one 4K h264 track
* with dynaminc change of frame rate .
* Api test sequence:
*  - StartCamera
*   loop Start {
*   ------------------
*   - CreateSession
*   - CreateVideoTrack
*   - StartVideoTrack
*   - SetVideoTrackParam (change frame rate)
*   - StopSession
*   - DeleteVideoTrack
*   - DeleteSession
*   ------------------
*   } loop End
*  - StopCamera
*/
TEST_F(RecorderGtest, DynamicFloatingFrameRate) {
  fprintf(stderr,"\n---------- Run Test %s.%s ------------\n",
      test_info_->test_case_name(),test_info_->name());

  auto ret = Init();
  assert(ret == NO_ERROR);

  VideoFormat format_type = VideoFormat::kAVC;
  int32_t width  = 3840;
  int32_t height = 2160;
  CodecParamType param_type = CodecParamType::kFrameRateType;
  float fps = 30.0;

  ret = recorder_.StartCamera(camera_id_, camera_start_params_);
  assert(ret == NO_ERROR);

  for (uint32_t i = 1; i <= iteration_count_; i++) {
    fprintf(stderr,"test iteration = %d/%d\n", i, iteration_count_);
    TEST_INFO("%s:%s: Running Test(%s) iteration = %d ", TAG, __func__,
        test_info_->name(), i);

    SessionCb session_status_cb;
    session_status_cb.event_cb =
        [this] (EventType event_type, void *event_data,
                size_t event_data_size) -> void {
        SessionCallbackHandler(event_type,
        event_data, event_data_size); };

    uint32_t session_id;
    ret = recorder_.CreateSession(session_status_cb, &session_id);
    assert(session_id > 0);
    assert(ret == NO_ERROR);

    VideoTrackCreateParam video_track_param;
    memset(&video_track_param, 0x0, sizeof video_track_param);

    video_track_param.camera_id   = camera_id_;
    video_track_param.width       = width;
    video_track_param.height      = height;
    video_track_param.frame_rate  = 30.0;
    video_track_param.format_type = format_type;
    uint32_t video_track_id = 1;

    if (dump_bitstream_.IsEnabled()) {
      StreamDumpInfo dumpinfo = {
        format_type,
        video_track_id,
        width,
        height };
      ret = dump_bitstream_.SetUp(dumpinfo);
      assert(ret == NO_ERROR);
    }

    TrackCb video_track_cb;
    video_track_cb.data_cb = [&] (uint32_t track_id,
                                  std::vector<BufferDescriptor> buffers,
                                  std::vector<MetaData> meta_buffers) {
        VideoTrackOneEncDataCb(track_id, buffers, meta_buffers); };

    video_track_cb.event_cb =
        [this] (uint32_t track_id, EventType event_type,
                void *event_data, size_t event_data_size) -> void
        { VideoTrackEventCb(track_id,
        event_type, event_data, event_data_size); };

    ret = recorder_.CreateVideoTrack(session_id, video_track_id,
                                      video_track_param, video_track_cb);
    assert(ret == NO_ERROR);

    std::vector<uint32_t> track_ids;
    track_ids.push_back(video_track_id);
    sessions_.insert(std::make_pair(session_id, track_ids));

    ret = recorder_.StartSession(session_id);
    assert(ret == NO_ERROR);

    // Let session run for 10s, than change frame rate.
    // During this time buffer with valid
    // data would be received in track callback (VideoTrackDataCb).
    sleep(10);

    fps = 12.5;
    ret = recorder_.SetVideoTrackParam(session_id, video_track_id, param_type,
      &fps, sizeof(fps));
    assert(ret == NO_ERROR);

    sleep(10);
    fps = 14.99;
    ret = recorder_.SetVideoTrackParam(session_id, video_track_id, param_type,
      &fps, sizeof(fps));
    assert(ret == NO_ERROR);

    sleep(10);
    fps = 23.98;
    ret = recorder_.SetVideoTrackParam(session_id, video_track_id, param_type,
      &fps, sizeof(fps));
    assert(ret == NO_ERROR);

    sleep(10);
    fps = 29.97;
    ret = recorder_.SetVideoTrackParam(session_id, video_track_id, param_type,
      &fps, sizeof(fps));
    assert(ret == NO_ERROR);

    sleep(10);

    ret = recorder_.StopSession(session_id, false);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteVideoTrack(session_id, video_track_id);
    assert(ret == NO_ERROR);

    ret = recorder_.DeleteSession(session_id);
    assert(ret == NO_ERROR);

     ClearSessions();
  }
  ret = recorder_.StopCamera(camera_id_);
  assert(ret == NO_ERROR);

  ret = DeInit();
  assert(ret == NO_ERROR);

  dump_bitstream_.CloseAll();

  fprintf(stderr,"---------- Test Completed %s.%s ----------\n",
      test_info_->test_case_name(), test_info_->name());
}

status_t RecorderGtest::QueueVideoFrame(VideoFormat format_type,
                                        const uint8_t *buffer, size_t size,
                                        int64_t timestamp, AVQueue *que) {
  int buffer_size = 0;
  uint8_t *tmp_buffer = NULL;
  AVPacket *packet = NULL;

  if ((size <= 5) || (NULL == que)) {
    return BAD_VALUE;
  }

  switch(format_type) {
    case VideoFormat::kAVC:
      if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x00 &&
          buffer[3] == 0x01 && buffer[4] == 0x67) {/* SPS,PPS*/
        if (que->pps != NULL) {
          free(que->pps);
          que->pps = NULL;
        }
        que->pps = (char *)malloc(sizeof(char) * (size));
        memcpy(que->pps, buffer, size);
        que->pps_size = size;
        que->is_pps = true;
        return NO_ERROR;
      }
      if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x00 &&
          buffer[3] == 0x01 && buffer[4] == 0x65) {
        if (que->is_pps != true) {
          buffer_size = que->pps_size;
        }
        que->is_pps = false;
      }
      break;
    case VideoFormat::kHEVC:
      if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x00 &&
          buffer[3] == 0x01 && buffer[4] == 0x40) {/* VPS,SPS,PPS*/
        if (que->pps != NULL) {
          free(que->pps);
          que->pps = NULL;
        }
        que->pps = (char *)malloc(sizeof(char) * (size));
        memcpy(que->pps, buffer, size);
        que->pps_size = size;
        que->is_pps = true;
        return NO_ERROR;
      }

      if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x00 &&
          buffer[3] == 0x01 && buffer[4] == 0x26) {
        if (que->is_pps != true) {
          buffer_size = que->pps_size;
        }
        que->is_pps = false;
      }
      break;
    default:
      TEST_ERROR("%s: Unsupported format type: %d", __func__, format_type);
      return BAD_VALUE;
  }

  /* Set pointer to start address */
  packet = (AVPacket *)malloc(sizeof(AVPacket));
  if ((NULL == packet)) {
    return NO_MEMORY;
  }

  /* Allocate a new frame object. */
  packet->data = tmp_buffer = (uint8_t *)malloc((size + buffer_size));
  if ((NULL == packet->data)) {
    free(packet);
    return NO_MEMORY;
  }

  if ((0 != buffer_size)) {
    memcpy(tmp_buffer, que->pps, que->pps_size);
    tmp_buffer += que->pps_size;
  }
  memcpy(tmp_buffer, buffer, size);
  packet->size = size + buffer_size;
  packet->timestamp = timestamp;
  AVQueuePushHead(que, packet);

  return NO_ERROR;
}

void RecorderGtest::VideoCachedDataCb(uint32_t track_id,
                                      std::vector<BufferDescriptor> buffers,
                                      std::vector<MetaData> meta_buffers,
                                      VideoFormat format_type,
                                      AVQueue *que) {

  for ( auto &iter : buffers) {
    if(iter.flag & static_cast<uint32_t>(BufferFlags::kFlagEOS)) {
      break;
    }

    auto ret = QueueVideoFrame(format_type, (uint8_t *) iter.data, iter.size,
                               iter.timestamp, que);
    if (NO_ERROR != ret) {
      TEST_ERROR("%s: Failed to cache video frame: %d\n", __func__, ret);
    }
  }

  // Return buffers back to service.
  std::map <uint32_t , std::vector<uint32_t> >::iterator it = sessions_.begin();
  uint32_t session_id = it->first;
  auto ret = recorder_.ReturnTrackBuffer(session_id, track_id, buffers);
  assert(ret == NO_ERROR);
}

status_t RecorderGtest::DumpQueue(AVQueue *queue, int32_t file_fd) {
  if ((NULL == queue) || (0 >= file_fd)) {
    return BAD_VALUE;
  }

  ssize_t q_size = AVQueueSize(queue);
  if (0 >= q_size) {
    TEST_ERROR("%s: Queue invalid or empty!", __func__);
    return BAD_VALUE;
  }

  AVPacket *pkt;
  for (ssize_t i = 0; i < q_size; i++) {
    pkt = (AVPacket *)AVQueuePopTail(queue);
    if (NULL != pkt) {
      if ((NULL != pkt->data)) {
        uint32_t written_length = write(file_fd, pkt->data, pkt->size);
        if (written_length != pkt->size) {
          TEST_ERROR("%s:%s: Bad Write error (%d) %s", TAG, __func__, errno,
                     strerror(errno));
          free(pkt->data);
          free(pkt);

          return -errno;
        }
        free(pkt->data);
      } else {
        TEST_ERROR("%s: AV packet empty!", __func__);
      }
      free(pkt);
    } else {
      TEST_ERROR("%s: Invalid AV packet popped!", __func__);
    }
  }

  return NO_ERROR;
}

void RecorderGtest::ClearSessions() {

  TEST_INFO("%s:%s Enter ", TAG, __func__);
  std::map <uint32_t , std::vector<uint32_t> >::iterator it = sessions_.begin();
  for (; it != sessions_.end(); ++it) {
    it->second.clear();
  }
  sessions_.clear();
  TEST_INFO("%s:%s Exit ", TAG, __func__);
}

void RecorderGtest::RecorderCallbackHandler(EventType event_type,
                                            void *event_data,
                                            size_t event_data_size) {
  TEST_INFO("%s:%s Enter ", TAG, __func__);
  TEST_INFO("%s:%s Exit ", TAG, __func__);
}

void RecorderGtest::SessionCallbackHandler(EventType event_type,
                                          void *event_data,
                                          size_t event_data_size) {
  TEST_INFO("%s:%s: Enter", TAG, __func__);
  TEST_INFO("%s:%s: Exit", TAG, __func__);
}

void RecorderGtest::CameraResultCallbackHandler(uint32_t camera_id,
                                                const CameraMetadata &result) {
  fprintf(stderr,"%s: camera_id: %d\n", __func__, camera_id);
  camera_metadata_ro_entry entry;
  entry = result.find(ANDROID_CONTROL_AWB_MODE);
  if (0 < entry.count) {
    fprintf(stderr,"%s: AWB mode: %d\n", __func__, *entry.data.u8);
  } else {
    fprintf(stderr,"%s: No AWB mode tag\n", __func__);
  }
}

void RecorderGtest::VideoTrackYUVDataCb(uint32_t track_id,
                                        std::vector<BufferDescriptor> buffers,
                                        std::vector<MetaData> meta_buffers) {

  TEST_DBG("%s:%s: Enter", TAG, __func__);

  if (is_dump_yuv_enabled_) {
    static uint32_t id = 0;
    ++id;
    if (id == dump_yuv_freq_) {
      String8 file_path;
      size_t written_len;
      file_path.appendFormat("/data/misc/qmmf/gtest_track_%d_%lld.yuv", track_id,
          buffers[0].timestamp);

      FILE *file = fopen(file_path.string(), "w+");
      if (!file) {
        ALOGE("%s:%s: Unable to open file(%s)", TAG, __func__,
            file_path.string());
        goto FAIL;
      }

      written_len = fwrite(buffers[0].data, sizeof(uint8_t),
                           buffers[0].size, file);
      TEST_DBG("%s:%s: written_len =%d", TAG, __func__, written_len);
      if (buffers[0].size != written_len) {
        TEST_ERROR("%s:%s: Bad Write error (%d):(%s)\n", TAG, __func__, errno,
            strerror(errno));
        goto FAIL;
      }
      TEST_INFO("%s:%s: Buffer(0x%p) Size(%u) Stored@(%s)\n", TAG, __func__,
        buffers[0].data, written_len, file_path.string());

  FAIL:
      if (file != NULL) {
        fclose(file);
      }
      id = 0;
    }
  }

  // Return buffers back to service.
  std::map <uint32_t , std::vector<uint32_t> >::iterator it = sessions_.begin();
  uint32_t session_id = it->first;
  auto ret = recorder_.ReturnTrackBuffer(session_id, track_id, buffers);
  assert(ret == NO_ERROR);
  TEST_DBG("%s:%s: Exit", TAG, __func__);
}

void RecorderGtest::VideoTrackOneEncDataCb(uint32_t track_id,
                                        std::vector<BufferDescriptor> buffers,
                                        std::vector<MetaData> meta_buffers) {

  TEST_DBG("%s:%s: Enter", TAG, __func__);
  if (dump_bitstream_.IsUsed()) {
    int32_t file_fd = dump_bitstream_.GetFileFd(1);
    dump_bitstream_.Dump(buffers, file_fd);
  }
  // Return buffers back to service.
  std::map <uint32_t , std::vector<uint32_t> >::iterator it = sessions_.begin();
  uint32_t session_id = it->first;
  auto ret = recorder_.ReturnTrackBuffer(session_id, track_id, buffers);
  assert(ret == NO_ERROR);

  TEST_DBG("%s:%s: Exit", TAG, __func__);
}

void RecorderGtest::VideoTrackTwoEncDataCb(uint32_t track_id,
                                          std::vector<BufferDescriptor> buffers,
                                          std::vector<MetaData> meta_buffers) {

  TEST_DBG("%s:%s: Enter", TAG, __func__);
  if (dump_bitstream_.IsUsed()) {
    int32_t file_fd = dump_bitstream_.GetFileFd(2);
    dump_bitstream_.Dump(buffers, file_fd);
  }
  // Return buffers back to service.
  std::map <uint32_t , std::vector<uint32_t> >::iterator it = sessions_.begin();
  uint32_t session_id = it->first;
  auto ret = recorder_.ReturnTrackBuffer(session_id, track_id, buffers);
  assert(ret == NO_ERROR);

  TEST_DBG("%s:%s: Exit", TAG, __func__);
}

void RecorderGtest::VideoTrackThreeEncDataCb(uint32_t track_id,
                                             std::vector<BufferDescriptor>
                                             buffers, std::vector<MetaData>
                                             meta_buffers) {

  TEST_DBG("%s:%s: Enter", TAG, __func__);
  if (dump_bitstream_.IsUsed()) {
    int32_t file_fd = dump_bitstream_.GetFileFd(3);
    dump_bitstream_.Dump(buffers, file_fd);
  }
  // Return buffers back to service.
  std::map <uint32_t , std::vector<uint32_t> >::iterator it = sessions_.begin();
  uint32_t session_id = it->first;
  auto ret = recorder_.ReturnTrackBuffer(session_id, track_id, buffers);
  assert(ret == NO_ERROR);

  TEST_DBG("%s:%s: Exit", TAG, __func__);
}

void RecorderGtest::VideoTrackEventCb(uint32_t track_id, EventType event_type,
                                      void *event_data, size_t data_size) {
    TEST_DBG("%s:%s: Enter", TAG, __func__);
    TEST_DBG("%s:%s: Exit", TAG, __func__);
}

void RecorderGtest::SnapshotCb(uint32_t camera_id,
                               uint32_t image_sequence_count,
                               BufferDescriptor buffer, MetaData meta_data) {

  TEST_INFO("%s:%s Enter", TAG, __func__);
  String8 file_path;
  size_t written_len;

  if (meta_data.meta_flag  &
      static_cast<uint32_t>(MetaParamType::kCamBufMetaData)) {
    CameraBufferMetaData cam_buf_meta = meta_data.cam_buffer_meta_data;
    TEST_DBG("%s:%s: format(0x%x)", TAG, __func__, cam_buf_meta.format);
    TEST_DBG("%s:%s: num_planes=%d", TAG, __func__, cam_buf_meta.num_planes);
    for (uint8_t i = 0; i < cam_buf_meta.num_planes; ++i) {
      TEST_DBG("%s:%s: plane[%d]:stride(%d)", TAG, __func__, i,
          cam_buf_meta.plane_info[i].stride);
      TEST_DBG("%s:%s: plane[%d]:scanline(%d)", TAG, __func__, i,
          cam_buf_meta.plane_info[i].scanline);
      TEST_DBG("%s:%s: plane[%d]:width(%d)", TAG, __func__, i,
          cam_buf_meta.plane_info[i].width);
      TEST_DBG("%s:%s: plane[%d]:height(%d)", TAG, __func__, i,
          cam_buf_meta.plane_info[i].height);
    }

    bool dump_file;
    if (cam_buf_meta.format == BufferFormat::kBLOB) {
      dump_file = (is_dump_jpeg_enabled_) ? true : false;
    } else {
      dump_file = (is_dump_raw_enabled_) ? true : false;
      fprintf(stderr, "\nRaw snapshot dumping enabled; "
              "keep track of free storage space.\n");
    }

    if (dump_file) {
      const char* ext_str;
      switch (cam_buf_meta.format) {
        case BufferFormat::kNV12:
        ext_str = "nv12";
        break;
        case BufferFormat::kNV21:
        ext_str = "nv21";
        break;
        case BufferFormat::kBLOB:
        ext_str = "jpg";
        break;
        case BufferFormat::kRAW10:
        ext_str = "raw10";
        break;
        case BufferFormat::kRAW16:
        ext_str = "raw16";
        break;
        default:
        ext_str = "bin";
        break;
      }

      struct timeval tv;
      gettimeofday(&tv, NULL);
      uint64_t tv_ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
      file_path.appendFormat("/data/misc/qmmf/snapshot_%u_%llu.%s", image_sequence_count,
          tv_ms, ext_str);
      FILE *file = fopen(file_path.string(), "w+");
      if (!file) {
        ALOGE("%s:%s: Unable to open file(%s)", TAG, __func__,
            file_path.string());
        goto FAIL;
      }

      written_len = fwrite(buffer.data, sizeof(uint8_t), buffer.size, file);
      TEST_INFO("%s:%s: written_len =%d", TAG, __func__, written_len);
      if (buffer.size != written_len) {
        ALOGE("%s:%s: Bad Write error (%d):(%s)\n", TAG, __func__, errno,
              strerror(errno));
        goto FAIL;
      }
      TEST_INFO("%s:%s: Buffer(0x%p) Size(%u) Stored@(%s)\n", TAG, __func__,
                buffer.data, written_len, file_path.string());

    FAIL:
      if (file != NULL) {
        fclose(file);
      }
    }
  }
  // Return buffer back to recorder service.
  recorder_.ReturnImageCaptureBuffer(camera_id, buffer);
  TEST_INFO("%s:%s Exit", TAG, __func__);
}

void RecorderGtest::ParseFaceInfo(const android::CameraMetadata &res,
                                  struct FaceInfo &info) {
  camera_metadata_ro_entry rect_entry, crop_entry;
  Rect<uint32_t> rect;
  uint32_t active_w = 0, active_h = 0;

  if (res.exists(ANDROID_STATISTICS_FACE_RECTANGLES)) {
    // Check Face Rectangles exit or not.
    rect_entry = res.find(ANDROID_STATISTICS_FACE_RECTANGLES);
    if (rect_entry.count > 0) {
      crop_entry = res.find(ANDROID_SCALER_CROP_REGION);
      if (crop_entry.count < 4) {
        TEST_ERROR("Unable to read crop region (count = %d)", crop_entry.count);
        assert(0);
      } else {
        active_w = crop_entry.data.i32[2];
        active_h = crop_entry.data.i32[3];
      }

      if ((active_w == 0) || (active_h == 0)) {
        TEST_ERROR("Invaild crop region(%d, %d)", active_w, active_h);
        assert(0);
      }

      TEST_INFO("%d face detected", rect_entry.count / 4);
      for (uint32_t i = 0 ; i < rect_entry.count; i += 4) {
        rect.left = rect_entry.data.i32[i + 0] *
                       info.fd_stream_width / active_w;
        rect.top = rect_entry.data.i32[i + 1] *
                       info.fd_stream_height / active_h;
        rect.width = rect_entry.data.i32[i + 2] *
                       info.fd_stream_width / active_w - rect.left;
        rect.height = rect_entry.data.i32[i + 3] *
                       info.fd_stream_height / active_h - rect.top;
        info.face_rect.push_back(rect);
      }
    }else {
      TEST_INFO("No face detected");
    }
  }
}

void RecorderGtest::ApplyFaceOveralyOnStream(struct FaceInfo &info) {
  face_overlay_lock_.lock();
  if (face_bbox_active_) {
    uint32_t i;
    int ret;
    uint32_t last_num = face_bbox_id_.size();
    uint32_t cur_num = info.face_rect.size();
    OverlayParam object_params;

    for(i = 0; i < std::min(last_num, cur_num); i++) {
      ret = RecorderGtest::recorder_.GetOverlayObjectParams(face_track_id_,
          face_bbox_id_[i], object_params);
      assert(ret == 0);
      object_params.bounding_box.start_x = info.face_rect[i].left;
      object_params.bounding_box.width = info.face_rect[i].width;
      if (object_params.bounding_box.width <= 0) {
        TEST_INFO("invalid width(%d)", object_params.bounding_box.width);
        object_params.bounding_box.width = 1;
      }
      object_params.bounding_box.start_y = info.face_rect[i].top;
      object_params.bounding_box.height = info.face_rect[i].height;
      if (object_params.bounding_box.height <= 0) {
        TEST_INFO("invalid width(%d)", object_params.bounding_box.height);
        object_params.bounding_box.height = 1;
      }
      ret = RecorderGtest::recorder_.UpdateOverlayObjectParams(face_track_id_,
          face_bbox_id_[i], object_params);
      assert(ret == 0);
      ret = RecorderGtest::recorder_.SetOverlay(face_track_id_, face_bbox_id_[i]);
      assert(ret == 0);
    }

    if (last_num > cur_num) {
      for(i = cur_num; i < last_num; i++) {
        ret = RecorderGtest::recorder_.RemoveOverlay(face_track_id_,
                                                     face_bbox_id_[i]);
        assert(ret == 0);
      }
    } else if (last_num < cur_num) {
      for (i = last_num; i < cur_num; i++) {
        // Create BoundingBox type overlay.
        std::string bb_text("Face");
        uint32_t bbox_id;
        memset(&object_params, 0x0, sizeof object_params);
        object_params.type  = OverlayType::kBoundingBox;
        object_params.color = COLOR_LIGHT_GREEN;
        object_params.bounding_box.start_x = info.face_rect[i].left;
        object_params.bounding_box.start_y = info.face_rect[i].top;
        object_params.bounding_box.width   = info.face_rect[i].width;
        object_params.bounding_box.height  = info.face_rect[i].height;
        bb_text.copy(object_params.bounding_box.box_name, bb_text.length());
        ret = recorder_.CreateOverlayObject(face_track_id_,
                 object_params, &bbox_id);
        assert(ret == 0);
        face_bbox_id_.push_back(bbox_id);
        ret = RecorderGtest::recorder_.SetOverlay(face_track_id_, bbox_id);
        assert(ret == 0);
      }
    }
  }
  face_overlay_lock_.unlock();
  info.face_rect.clear();
}

status_t DumpBitStream::SetUp(const StreamDumpInfo& dumpinfo) {

  TEST_DBG("%s:%s: Enter", TAG, __func__);
  assert(dumpinfo.width > 0);
  assert(dumpinfo.height > 0);

  const char* type_string;
  switch (dumpinfo.format) {
    case VideoFormat::kAVC:
      type_string = "h264";
      break;
    case VideoFormat::kHEVC:
      type_string = "h265";
      break;
    default:
      type_string = "bin";
      break;
  }
  String8 extn(type_string);
  String8 bitstream_filepath;
  bitstream_filepath.appendFormat("/data/misc/qmmf/gtest_track_%d_%dx%d.%s",
                                  dumpinfo.track_id, dumpinfo.width,
                                  dumpinfo.height, extn.string());
  int32_t file_fd = open(bitstream_filepath.string(),
                          O_CREAT | O_WRONLY | O_TRUNC, 0655);
  if (file_fd <= 0) {
    TEST_ERROR("%s:%s File open failed!", TAG, __func__);
    return BAD_VALUE;
  }

  file_fds_.push_back(file_fd);

  TEST_DBG("%s:%s: Exit", TAG, __func__);
  return NO_ERROR;
}

status_t DumpBitStream::Dump(const std::vector<BufferDescriptor>& buffers,
                             const int32_t file_fd) {

  TEST_DBG("%s:%s: Enter", TAG, __func__);
  assert(file_fd > 0);

  for (auto& iter : buffers) {
    uint32_t exp_size = iter.size;
    TEST_DBG("%s:%s BitStream buffer data(0x%x):size(%d):ts(%lld):flag(0x%x)"
      ":buf_id(%d):capacity(%d)", TAG, __func__, iter.data, iter.size,
       iter.timestamp, iter.flag, iter.buf_id, iter.capacity);

    uint32_t written_length = write(file_fd, iter.data, iter.size);
    TEST_DBG("%s:%s: written_length(%d)", TAG, __func__, written_length);
    if (written_length != exp_size) {
      TEST_ERROR("%s:%s: Bad Write error (%d) %s", TAG, __func__, errno,
      strerror(errno));
      return BAD_VALUE;
    }

    if(iter.flag & static_cast<uint32_t>(BufferFlags::kFlagEOS)) {
      TEST_INFO("%s:%s EOS Last buffer!", TAG, __func__);
      break;
    }
  }

  TEST_DBG("%s:%s: Exit", TAG, __func__);
  return NO_ERROR;
}

void DumpBitStream::Close(int32_t file_fd) {
  TEST_DBG("%s:%s: Enter", TAG, __func__);
  if (file_fd > 0) {
    auto iter = std::find(file_fds_.begin(), file_fds_.end(), file_fd);
    if(iter != file_fds_.end()) {
      close(file_fd);
      file_fds_.erase(iter);
    } else {
      TEST_WARN("%s:%s: file_fd does not exist!", TAG, __func__);
    }
  }
  TEST_DBG("%s:%s: Exit", TAG, __func__);
}

void DumpBitStream::CloseAll() {
  TEST_DBG("%s:%s: Enter", TAG, __func__);
  for (auto& iter : file_fds_) {
    if (iter > 0) {
      close(iter);
    }
  }
  file_fds_.clear();
  TEST_DBG("%s:%s: Exit", TAG, __func__);
}
