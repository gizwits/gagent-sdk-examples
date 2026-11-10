#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define GAGENT_CAMERA_CONFIG_ABI 1u
/* Portable ABI: no ESP driver structures or caller-owned strings. All rates in bps/Hz. */
enum { GAGENT_CAMERA_OK=0, GAGENT_CAMERA_INVALID=1, GAGENT_CAMERA_UNSUPPORTED=2,
       GAGENT_CAMERA_STATE=3, GAGENT_CAMERA_BUSY=4 };
typedef struct {
    int32_t i2c_port, sda_pin, scl_pin;
    uint32_t i2c_hz;
    int32_t xclk_pin;
    uint32_t xclk_hz;
    int32_t reset_pin, pwdn_pin, power_pin;
    uint32_t reset_active_high, pwdn_active_high, power_active_high;
    uint32_t power_delay_ms, reset_hold_ms, reset_release_ms;
    uint32_t buffer_count;
} gagent_camera_hardware_t;
typedef struct {
    uint32_t target_bps, min_bps, max_bps, adaptive;
} gagent_camera_bitrate_t;
typedef struct {
    uint32_t width, height, fps_num, fps_den;
    /* First release supports H.264 with the ESP encoder's default rate control.
       There is no public CBR/VBR selector until the backend implements both. */
    uint32_t gop_frames, qp_min, qp_max;
    gagent_camera_bitrate_t bitrate;
} gagent_camera_video_t;
typedef struct {
    uint32_t struct_size, abi_version;
    char sensor[16], capture_mode[48];
    gagent_camera_hardware_t hardware;
    gagent_camera_video_t video;
    char backend_url[513], whip_url[513];
} gagent_camera_config_t;
typedef struct {
    char sensor[16], id[48];
    uint32_t width, height, fps_num, fps_den, raw_bits, csi_lanes, xclk_hz;
} gagent_camera_mode_t;
/* SDK-supported profiles, NOT a claim that a sensor is attached. The consumer
   selects its driver/native mode in board.json and rebuilds its own firmware. */
size_t gagent_camera_mode_count(void);
int gagent_camera_mode_at(size_t index, gagent_camera_mode_t *out);
int gagent_camera_validate(const gagent_camera_config_t *config);
/* Board defaults are linked from customer-generated source, never baked into .a. */
void gagent_camera_get_defaults(gagent_camera_config_t *out);
/* Call before application startup / first service.sync(). Config is copied. */
int gagent_camera_configure(const gagent_camera_config_t *config);
void gagent_camera_get_config(gagent_camera_config_t *out);
typedef struct {
    uint32_t struct_size, abi_version;
    gagent_camera_mode_t capture;
    gagent_camera_video_t output;
} gagent_camera_probe_result_t;
/* Synchronous hardware detection and native-mode verification. Call after configure,
   before application startup, on the same initialization task. Seals configuration.
   Does not start a network stream; driver errors are returned unchanged. */
int gagent_camera_probe(gagent_camera_probe_result_t *out);
/* Runtime policy updates are serialized by the media worker. OK means queued.
   Poll request_id/completed_id/error to observe application, including failure. */
typedef struct {
    uint32_t request_id, completed_id;
    int32_t error;
    uint32_t active, current_bps;
    gagent_camera_bitrate_t policy;
} gagent_camera_bitrate_status_t;
int gagent_camera_set_bitrate(const gagent_camera_bitrate_t *policy, uint32_t *request_id);
void gagent_camera_get_bitrate_status(gagent_camera_bitrate_status_t *out);
#ifdef __cplusplus
}
#endif
