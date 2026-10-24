kb_dir = meson.current_source_dir()

keyboards += {
    'leobog-hi75c' : {
        'dir' : kb_dir,
        'platform' : 'sh68f90',
        'src' : files(
            'user_matrix.c',
            'user_init.c',
            'user_sleep.c',
            'kb.c',
            'vial.c',
        ),
        'layouts' : {
            'default' : {
                'dir' : kb_dir / 'layouts/default',
                'src' : files('layouts/default/layout.c'),
            },
            # VIA: the keymap is edited from VIA (raw HID on interface 0) and kept
            # in flash; via.json is the definition to load into VIA.
            'default-via' : {
                'dir' : kb_dir / 'layouts/default',
                'src' : files('layouts/default/layout.c'),
                'via' : true,
            },
            # Vial: the same raw HID and dynamic keymap, plus the 0xFE commands
            # (UID, XZ definition, unlock) served from vial_definition.h.
            'default-vial' : {
                'dir' : kb_dir / 'layouts/default',
                'src' : files('layouts/default/layout.c'),
                'via' : true,
                'vial' : true,
            },
        },
        'options' : {
            'vendor_id' : '0x258a',
            'product_id' : '0x010c',
            'no_serial' : '1',
            'usb_sw2con' : '1',
            'led_com_isr' : '1',
        },
    },
}
