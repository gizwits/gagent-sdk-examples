"""Portable camera settings and generated board glue for binary SDK consumers."""
import json
from pathlib import Path
import re
import sys


def camera_config(sdk, manifest, board):
    sys.path.insert(0,str(sdk/'tools'))
    from camera_config import resolve_camera
    source=board if board else manifest['board']
    raw=source.get('camera',{'enabled':False})
    config=resolve_camera(raw,source.get('mode',source.get('profile',{}).get('mode','soc')),
        manifest['target'],source.get('application'),source.get('aispeech',{}),manifest['cloud_provider'],source.get('p4_hosted'))
    if bool(config['enabled'])!=bool(manifest['board'].get('camera',{}).get('enabled')):
        raise ValueError('Camera capability differs from SDK')
    if config['enabled'] and config['detection']!='none':
        raise ValueError('Binary camera SDK does not contain detection models')
    return config


def sensor_key(key):
    return bool(re.match(r'CONFIG_CAMERA_(SC2336|OV2710|OV5647|SC202CS)(?:_|$)',key))


def sensor_values(sdk, config):
    sys.path.insert(0,str(sdk/'tools'))
    from camera_config import sdk_values
    values=sdk_values({'camera':config},sdk)
    if config['enabled'] and config['sensor']=='ov2710':
        values['CONFIG_CAMERA_OV2710_CUSTOMIZED_IPA_JSON_CONFIGURATION_FILE_PATH']=json.dumps(str(sdk/'camera/ov2710_custom.json'))
    return {key:value for key,value in values.items() if sensor_key(key)}


def generate_binding(sdk, output, manifest, board):
    sys.path.insert(0,str(sdk/'tools'))
    from p4_config import generate_power_binding
    source=board if board else manifest['board']
    binding=output/'gagent_board_binding'
    binding.mkdir(parents=True,exist_ok=True)
    generate_power_binding(source['p4_hosted'],binding)
    files=['p4_defaults.c']
    includes=[sdk/'components/gagent_p4/include']
    if manifest.get('camera_config_abi'):
        from camera_config import generate_runtime
        config=camera_config(sdk,manifest,board)
        if config['enabled']:
            from p4_config import reserved
            blocked=reserved(source.get('p4_hosted'))
            blocked.update(source.get('application',{}).get(k,-1) for k in ('tx_pin','rx_pin'))
            generate_runtime(config,binding,blocked)
            files.append('camera_defaults.c')
            includes.append(sdk/'components/gagent_camera/include')
    (binding/'CMakeLists.txt').write_text('idf_component_register(WHOLE_ARCHIVE SRCS '+ ' '.join('"'+f+'"' for f in files)+
        ' INCLUDE_DIRS '+ ' '.join('"'+str(p)+'"' for p in includes)+')\n')
    return binding
