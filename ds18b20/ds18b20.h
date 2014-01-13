#ifndef _ds18b20_h
#define _ds18b20_h
/*
typedef enum _at91_ds18b20_cmds
{
	at91_ds18b20_ctl,
	at91_ds18b20_rst
} at91_ds18b20_cmds;
*/
typedef enum _at91_ds18b20_port
{
	at91_gpio_port_PA,	
	at91_gpio_port_PB,
	at91_gpio_port_PC,
	at91_gpio_port_PD,
} at91_ds18b20_port;

typedef enum _at91_ds18b20_pin
{
	at91_gpio_pin_0 = 0,	
	at91_gpio_pin_1,
	at91_gpio_pin_2,
	at91_gpio_pin_3,
	at91_gpio_pin_4,
	at91_gpio_pin_5,
	at91_gpio_pin_6,
	at91_gpio_pin_7,
	at91_gpio_pin_8,
	at91_gpio_pin_9,
	at91_gpio_pin_10,
	at91_gpio_pin_11,
	at91_gpio_pin_12,
	at91_gpio_pin_13,
	at91_gpio_pin_14,
	at91_gpio_pin_15,
	at91_gpio_pin_16,
	at91_gpio_pin_17,
	at91_gpio_pin_18,
	at91_gpio_pin_19,
	at91_gpio_pin_20,
	at91_gpio_pin_21,
	at91_gpio_pin_22,
	at91_gpio_pin_23,
	at91_gpio_pin_24,
	at91_gpio_pin_25,
	at91_gpio_pin_26,
	at91_gpio_pin_27,
	at91_gpio_pin_28,
	at91_gpio_pin_29,
	at91_gpio_pin_30,
	at91_gpio_pin_31
} at91_ds18b20_pin;

typedef struct _at91_ds18b20_ctl_s
{
	int which_port;
	int which_pin;
} at91_ds18b20_ctl_s;

#define DS18B20            'D'
#define AT91_DS18B20_CTL		    _IOWR(DS18B20, 1, at91_ds18b20_ctl_s) 
#endif