"""Resolve explicit Board-owned ESP-Hosted wiring; never infer pins from a name."""


def resolve_hosted(raw, target, application, voice):
    if target != 'esp32p4':
        if raw is not None: raise ValueError('p4_hosted requires ESP32-P4')
        return None
    if not isinstance(raw, dict): raise ValueError('P4 board must declare p4_hosted wiring')
    defaults = dict(reset_active_high=True, power_pin=-1,
                    power_active_high=True, power_delay_ms=20, reserved_pins=[])
    common = {'chip','transport','reset_pin','sdio','spi'} | set(defaults)
    if set(raw)-common: raise ValueError('Unknown p4_hosted setting')
    cfg = {**defaults, **raw}
    if cfg.get('chip') not in ('esp32c5','esp32c6','esp32c61'): raise ValueError('Unsupported Hosted chip')
    if cfg.get('transport') not in ('sdio','spi'): raise ValueError('Hosted transport must be sdio or spi')
    def integer(value, low, high, label):
        if type(value) is not int or not low <= value <= high: raise ValueError('Invalid Hosted '+label)
    for key in ('reset_active_high','power_active_high'):
        if type(cfg[key]) is not bool: raise ValueError('Invalid Hosted '+key)
    integer(cfg.get('reset_pin'),0,54,'reset_pin')
    integer(cfg['power_pin'],-1,54,'power_pin')
    integer(cfg['power_delay_ms'],0,10000,'power_delay_ms')
    bus = cfg['transport']; other = 'spi' if bus=='sdio' else 'sdio'
    if other in cfg: raise ValueError('Declare only the selected Hosted transport')
    config=cfg.get(bus)
    keys = {'clk_pin','cmd_pin','data_pins','clock_khz','reset_delay_ms'} if bus=='sdio' else {'clk_pin','mosi_pin','miso_pin','cs_pin','handshake_pin','data_ready_pin','clock_mhz','mode'}
    if not isinstance(config,dict) or set(config)!=keys: raise ValueError('Declare all '+bus+' settings')
    pins=[cfg['reset_pin']]
    if cfg['power_pin']>=0: pins.append(cfg['power_pin'])
    for key,value in config.items():
        if key.endswith('_pin'):
            integer(value,0,54,key);pins.append(value)
    if bus=='sdio':
        if not isinstance(config['data_pins'],list) or len(config['data_pins'])!=4: raise ValueError('SDIO requires four data pins, D0 through D3')
        for pin in config['data_pins']:integer(pin,0,54,'data pin')
        pins.extend(config['data_pins'])
        integer(config['clock_khz'],400,50000,'SDIO clock')
        integer(config['reset_delay_ms'],100,10000,'reset_delay_ms')
    else:
        integer(config['clock_mhz'],1,40,'SPI clock');integer(config['mode'],0,3,'SPI mode')
    extra=cfg['reserved_pins']
    if not isinstance(extra,list):raise ValueError('Hosted reserved_pins must be a list')
    for pin in extra:integer(pin,0,54,'reserved pin')
    if len(set(extra))!=len(extra) or set(pins)&set(extra):raise ValueError('Overlapping Hosted reserved pins')
    occupied={24,25} # USB Serial/JTAG owns both pins on every P4 board.
    occupied.update((application or {}).get(k,-1) for k in ('tx_pin','rx_pin'))
    if voice.get('enabled'):occupied.update(voice[k] for k in ('tx_pin','rx_pin'))
    if len(set(pins))!=len(pins) or (set(pins)|set(extra))&occupied:raise ValueError('Hosted GPIO conflicts with UART, USB or another Hosted pin')
    return cfg


def reserved(cfg):
    if not cfg:return {24,25}
    bus=cfg[cfg['transport']]
    pins={cfg['reset_pin'],cfg['power_pin'],24,25,*cfg['reserved_pins']}
    pins.update(value for key,value in bus.items() if key.endswith('_pin'))
    pins.update(bus.get('data_pins',[]));pins.discard(-1)
    return pins


def generate_hosted(selected, output):
    path=output/'p4_hosted_board.h';cfg=selected.get('p4_hosted')
    if not cfg:path.unlink(missing_ok=True);return
    text='#pragma once\n'
    for key in ('power_pin','power_active_high','power_delay_ms'):
        text+=f'#define GAGENT_HOSTED_{key.upper()} {int(cfg[key])}\n'
    path.write_text(text)
    generate_power_binding(cfg, output)


def generate_power_binding(cfg, output):
    (output/'p4_defaults.c').write_text('#include "gagent_p4.h"\n'
        'gagent_p4_power_config_t gagent_p4_board_power(void) {\n'
        'return (gagent_p4_power_config_t){%d,%s,%d};\n}\n' %
        (cfg['power_pin'],str(cfg['power_active_high']).lower(),cfg['power_delay_ms']))


def sdk_values(selected):
    cfg=selected.get('p4_hosted')
    if not cfg:return {}
    chip=cfg['chip'].upper();bus=cfg['transport'];p=cfg[bus]
    values={'CONFIG_SLAVE_IDF_TARGET_ESP32C5':'n','CONFIG_SLAVE_IDF_TARGET_ESP32C6':'n','CONFIG_SLAVE_IDF_TARGET_ESP32C61':'n',
            'CONFIG_ESP_HOSTED_CP_TARGET_ESP32C5':'n','CONFIG_ESP_HOSTED_CP_TARGET_ESP32C6':'n','CONFIG_ESP_HOSTED_CP_TARGET_ESP32C61':'n',
            'CONFIG_ESP32P4_EYE_C6_BOARD':'n','CONFIG_ESP_HOSTED_P4_DEV_BOARD_NONE':'y',
            'CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE':'n','CONFIG_ESP_HOSTED_SPI_HOST_INTERFACE':'n'}
    values['CONFIG_SLAVE_IDF_TARGET_'+chip]='y';values['CONFIG_ESP_HOSTED_CP_TARGET_'+chip]='y'
    prefix='CONFIG_ESP_HOSTED_'+bus.upper()
    values[prefix+'_HOST_INTERFACE']='y'
    values[prefix+'_GPIO_RESET_SLAVE']=str(cfg['reset_pin'])
    values[prefix+'_RESET_ACTIVE_HIGH']='y' if cfg['reset_active_high'] else 'n'
    values[prefix+'_RESET_ACTIVE_LOW']='n' if cfg['reset_active_high'] else 'y'
    if bus=='sdio':
        values.update(CONFIG_ESP_HOSTED_SDIO_SLOT_1='y',CONFIG_ESP_HOSTED_SDIO_SLOT_0='n',CONFIG_ESP_HOSTED_SDIO_4_BIT_BUS='y')
        pins=[p['cmd_pin'],p['clk_pin'],*p['data_pins']]
        for key,pin in zip(('CMD_SLOT_1','CLK_SLOT_1','D0_SLOT_1','D1_4BIT_BUS_SLOT_1','D2_4BIT_BUS_SLOT_1','D3_4BIT_BUS_SLOT_1'),pins):
            values['CONFIG_ESP_HOSTED_PRIV_SDIO_PIN_'+key]=str(pin)
        values[prefix+'_CLOCK_FREQ_KHZ']=str(p['clock_khz'])
        values[prefix+'_RESET_DELAY_MS']=str(p['reset_delay_ms'])
    else:
        values['CONFIG_ESP_HOSTED_SPI_HSPI']='y'
        for key in ('clk','mosi','miso','cs'):values[prefix+'_HSPI_GPIO_'+key.upper()]=str(p[key+'_pin'])
        values[prefix+'_GPIO_HANDSHAKE']=str(p['handshake_pin'])
        values[prefix+'_GPIO_DATA_READY']=str(p['data_ready_pin'])
        for mode in range(4):values[prefix+f'_PRIV_MODE_{mode}_ESP32XX']='y' if mode==p['mode'] else 'n'
        values[prefix+('_FREQ_ESP32C6' if chip=='ESP32C6' else '_FREQ_ESP32XX')]=str(p['clock_mhz'])
    return values
