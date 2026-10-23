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
        ),
        'layouts' : {
            'default' : {
                'dir' : kb_dir / 'layouts/default',
                'src' : files('layouts/default/layout.c'),
            },
        },
        'options' : {
            'vendor_id' : '0x258a',
            'product_id' : '0x010c',
            'no_serial' : '1',
            'usb_sw2con' : '1',
            'led_com_isr' : '1',
            'led_gpio_test' : '1',
        },
    },
}
