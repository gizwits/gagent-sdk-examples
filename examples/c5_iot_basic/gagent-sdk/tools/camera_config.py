"""Opt-in MCU/SoC camera profile. No ESP media dependencies are discovered when off."""
import json
from pathlib import Path
PROFILES = json.loads(Path(__file__).with_name("camera_profiles.json").read_text())
from urllib.parse import urlsplit

# event_duration_ms is accepted for old boards only; firmware no longer auto-stops by duration.
DEFAULTS = dict(enabled=False, sensor="ov2710", i2c_port=0, scl_pin=13, sda_pin=14,
                i2c_hz=100000, xclk_pin=11, xclk_hz=24000000, reset_pin=26,
                pwdn_pin=-1, power_pin=12, power_active_high=True, power_delay_ms=20,
                width=640, height=360, fps=20, bitrate=600000,
                capture_mode="", fps_den=1, adaptive_bitrate=True, bitrate_min=0, bitrate_max=0,
                gop_frames=0, qp_min=25, qp_max=45, buffer_count=2,
                reset_active_high=False, pwdn_active_high=True, reset_hold_ms=10, reset_release_ms=10,
                detection="none", score_threshold=50, detect_interval_ms=1000,
                confirm_frames=3, absent_ms=3000, auto_stream=False,
                event_duration_ms=0, cooldown_ms=60000, backend_url="", whip_url="")


def resolve_camera(raw, mode, target, application, voice, provider, hosted=None):
    if raw is None:
        return {"enabled": False}
    if not isinstance(raw, dict) or set(raw)-set(DEFAULTS):
        raise ValueError("Unknown camera setting")
    result = {**DEFAULTS, **raw}
    for key in ("enabled", "power_active_high", "auto_stream", "adaptive_bitrate", "reset_active_high", "pwdn_active_high"):
        if type(result[key]) is not bool:
            raise ValueError("Camera switch must be boolean: " + key)
    if not result["enabled"]:
        return {"enabled": False}
    if target != "esp32p4" or mode not in ("mcu", "soc") or (mode == "mcu" and application is None):
        raise ValueError("Camera requires an ESP32-P4 SoC board or configuration-only MCU board")
    if result["sensor"] not in PROFILES:
        raise ValueError("Camera sensor must be one of: " + ", ".join(PROFILES))
    if not isinstance(result['capture_mode'],str): raise ValueError('capture_mode must be a string')
    for key in ('bitrate','bitrate_min','bitrate_max','gop_frames','fps'):
        if type(result[key]) is not int: raise ValueError('Camera value must be integer: '+key)
    profile = PROFILES[result["sensor"]]
    result["capture_mode"] = result["capture_mode"] or profile["default"]
    if result["capture_mode"] not in profile["modes"]:
        raise ValueError("Unsupported camera capture_mode")
    native_w, native_h, native_fps, _, _ = profile["modes"][result["capture_mode"]]
    if not result["bitrate_min"]: result["bitrate_min"] = min(250000, result["bitrate"])
    if not result["bitrate_max"]: result["bitrate_max"] = result["bitrate"]
    if not result["gop_frames"]: result["gop_frames"] = result["fps"]
    if result["detection"] not in ("none", "cat", "human_face"):
        raise ValueError("Invalid camera detection model")
    ranges = dict(i2c_port=(0,1), scl_pin=(0,54), sda_pin=(0,54), i2c_hz=(100000,400000),
                  xclk_pin=(-1,54), xclk_hz=(24000000,24000000), reset_pin=(-1,54),
                  pwdn_pin=(-1,54), power_pin=(-1,54), power_delay_ms=(0,10000), width=(160,native_w),height=(120,native_h),
                  fps=(1,min(30,native_fps)),fps_den=(1,1),bitrate=(100000,10000000),
                  bitrate_min=(100000,10000000),bitrate_max=(100000,10000000),
                  gop_frames=(1,300),qp_min=(0,51),qp_max=(0,51),buffer_count=(2,4),
                  reset_hold_ms=(0,10000),reset_release_ms=(0,10000),score_threshold=(1,100),
                  detect_interval_ms=(200,10000),confirm_frames=(1,20),absent_ms=(200,60000),
                  event_duration_ms=(0,600000),cooldown_ms=(1000,3600000))
    for key,(low,high) in ranges.items():
        if type(result[key]) is not int or not low <= result[key] <= high:
            raise ValueError("Invalid camera parameter: " + key)
    if result['width'] % 16 or result['height'] % 2:
        raise ValueError("H264 output requires width multiple of 16 and even height")
    if result['width']*native_h != result['height']*native_w and not (
            native_w == 1024 and native_h == 600 and (result['width'],result['height']) == (640,360)):
        raise ValueError("Output must preserve capture aspect ratio (except legacy SC2336 640x360)")
    if not result['bitrate_min'] <= result['bitrate'] <= result['bitrate_max'] or result['qp_min'] > result['qp_max']:
        raise ValueError("Invalid camera bitrate or QP bounds")
    pins=[result[k] for k in ('scl_pin','sda_pin','xclk_pin','reset_pin','pwdn_pin','power_pin') if result[k]>=0]
    from p4_config import reserved as radio_reserved
    reserved=radio_reserved(hosted)
    reserved.update((application or {}).get(k, -1) for k in ('tx_pin','rx_pin') if (application or {}).get(k, -1)>=0)
    if voice.get('enabled'):
        reserved.update(voice[k] for k in ('tx_pin','rx_pin'))
    if len(set(pins)) != len(pins) or set(pins)&reserved:
        raise ValueError("Camera GPIO conflicts with another camera pin, MCU UART, USB or Hosted radio")
    for key in ('backend_url','whip_url'):
        value=result[key]
        if not isinstance(value,str) or len(value)>512 or any(ord(c)<=32 or ord(c)>126 for c in value):
            raise ValueError("Invalid camera URL: " + key)
        if value:
            url=urlsplit(value)
            if url.scheme not in ('http','https') or not url.hostname or url.username or url.password or url.fragment:
                raise ValueError("Camera URL must be HTTP(S) without embedded credentials or fragment")
            if key=='backend_url' and url.query:
                raise ValueError("Camera backend URL cannot contain a query")
    if result['backend_url'] and result['whip_url']:
        raise ValueError("Select camera backend_url or whip_url, not both")
    if provider != 'gizwits' and (result['backend_url'] or result['whip_url'] or result['auto_stream']):
        raise ValueError("Camera streaming currently supports Gizwits only")
    if result['auto_stream'] and (result['detection']=='none' or not (result['backend_url'] or result['whip_url'])):
        raise ValueError("Automatic camera upload requires detection and an upload endpoint")
    return result


def generate_camera(result, output):
    config=result['camera']
    # Remove generated feature headers when switching a build directory to off.
    if not config['enabled']:
        for name in ('camera_board.h','camera.cmake'):
            (output/name).unlink(missing_ok=True)
        return
    from p4_config import reserved
    blocked=reserved(result.get('p4_hosted'))
    blocked.update(result.get('application',{}).get(k,-1) for k in ('tx_pin','rx_pin'))
    generate_runtime(config, output, blocked)
    text='#pragma once\n'
    for key,value in config.items():
        if type(value) is bool: value=int(value)
        text+=f'#define GAGENT_CAMERA_{key.upper()} {json.dumps(value)}\n'
    text+=f'#define GAGENT_CAMERA_CAT {int(config["detection"]=="cat")}\n'
    text+=f'#define GAGENT_CAMERA_FACE {int(config["detection"]=="human_face")}\n'
    (output/'camera_board.h').write_text(text)
    (output/'camera.cmake').write_text(f'set(GAGENT_CAMERA_DETECT "{config["detection"]}")\n')


def sdk_values(selected, root):
    if not selected.get('camera',{}).get('enabled'):
        return {}
    sensor=selected['camera']['sensor']
    config=selected['camera']
    mode=config.get('capture_mode') or PROFILES[sensor]['default']
    sensor_values={f'CONFIG_CAMERA_{name.upper()}': 'y' if name==sensor else 'n' for name in PROFILES}
    # Reset every curated mode choice so a stale sdkconfig is rejected explicitly.
    for name,profile in PROFILES.items():
        prefix='CONFIG_CAMERA_'+name.upper()+'_MIPI_'
        for item in profile['modes']:
            sensor_values[prefix+item]='y' if name==sensor and item==mode else 'n'
            sensor_values[prefix+'DEFAULT_FMT_'+item]='y' if name==sensor and item==mode else 'n'
    # Clear upstream defaults which are outside the curated SDK modes.
    sensor_values['CONFIG_CAMERA_OV5647_MIPI_RAW8_800X800_50FPS']='n'
    sensor_values['CONFIG_CAMERA_OV5647_MIPI_DEFAULT_FMT_RAW8_800X800_50FPS']='n'
    if sensor=='ov2710':
        sensor_values.update({
            'CONFIG_CAMERA_OV2710_CUSTOMIZED_IPA_JSON_CONFIGURATION_FILE':'y',
            'CONFIG_CAMERA_OV2710_DEFAULT_IPA_JSON_CONFIGURATION_FILE':'n',
            'CONFIG_CAMERA_OV2710_CUSTOMIZED_IPA_JSON_CONFIGURATION_FILE_PATH':json.dumps(str(root/'optional/camera/components/gagent_camera/ov2710_custom.json')),
        })
    return {
        **sensor_values,
        'CONFIG_ESP_VIDEO_ENABLE_ISP_PIPELINE_CONTROLLER':'y',
        'CONFIG_ISP_PIPELINE_CONTROLLER_TASK_STACK_USE_PSRAM':'y',
        'CONFIG_MBEDTLS_SSL_PROTO_DTLS':'y', 'CONFIG_MBEDTLS_SSL_DTLS_SRTP':'y',
        'CONFIG_MBEDTLS_X509_CREATE_C':'y', 'CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC':'y',
        # WebRTC's DTLS peer must support client and server roles.
        'CONFIG_MBEDTLS_TLS_CLIENT_ONLY':'n','CONFIG_MBEDTLS_TLS_SERVER_AND_CLIENT':'y',
        'CONFIG_MBEDTLS_TLS_SERVER_ONLY':'n','CONFIG_MBEDTLS_TLS_DISABLED':'n',
        'CONFIG_LWIP_IPV6':'y', 'CONFIG_LWIP_SNTP_MAX_SERVERS':'2', 'CONFIG_LWIP_TCPIP_RECVMBOX_SIZE':'64', 'CONFIG_LWIP_MAX_UDP_PCBS':'24',
        'CONFIG_CODEC_I2C_BACKWARD_COMPATIBLE':'n',
    }


def generate_runtime(config, output, blocked=()):
    """Customer-owned binding: the prebuilt library contains no board pins/video defaults."""
    hardware_keys=('i2c_port','sda_pin','scl_pin','i2c_hz','xclk_pin','xclk_hz','reset_pin','pwdn_pin','power_pin',
                   'reset_active_high','pwdn_active_high','power_active_high','power_delay_ms','reset_hold_ms','reset_release_ms','buffer_count')
    def fields(keys):
        return ','.join('.'+k+'='+str(int(config[k])) for k in keys)
    text='#include "gagent/camera/config.h"\nvoid gagent_camera_board_defaults(gagent_camera_config_t *out) {\n'
    text+='*out=(gagent_camera_config_t){.struct_size=sizeof(*out),.abi_version=GAGENT_CAMERA_CONFIG_ABI,\n'
    text+='.sensor='+json.dumps(config['sensor'])+',.capture_mode='+json.dumps(config['capture_mode'])+',\n'
    text+='.hardware={'+fields(hardware_keys)+'},\n.video={'+fields(('width','height','fps_den','gop_frames','qp_min','qp_max'))+',.fps_num='+str(config['fps'])+','
    text+='.bitrate={.target_bps=%d,.min_bps=%d,.max_bps=%d,.adaptive=%d}},\n'%(config['bitrate'],config['bitrate_min'],config['bitrate_max'],config['adaptive_bitrate'])
    text+='.backend_url='+json.dumps(config['backend_url'])+',.whip_url='+json.dumps(config['whip_url'])+'};\n}\n'
    text+='int gagent_camera_board_pin_allowed(int pin) {\n'
    for pin in sorted(set(blocked)-{-1}): text+='if(pin==%d)return 0;\n'%pin
    text+='return 1;\n}\n'
    (output/'camera_defaults.c').write_text(text)
    modes='static const gagent_camera_mode_t modes[]={\n'
    for sensor,profile in PROFILES.items():
        for mode,(w,h,fps,bits,lanes) in profile['modes'].items():
            modes+='{"%s","%s",%d,%d,%d,1,%d,%d,24000000},\n'%(sensor,mode,w,h,fps,bits,lanes)
    (output/'camera_modes.inc').write_text(modes+'};\n')
