#ifndef _HEAD_H_
#define _HEAD_H_

#define ADC_DEVICE_TYPE    'W'
#define ADC_DEVICE_INIT    _IO(ADC_DEVICE_TYPE, 0x40)
#define ADC_DEVICE_START   _IO(ADC_DEVICE_TYPE, 0x41)
#define ADC_DEVICE_STOP    _IO(ADC_DEVICE_TYPE, 0x42)

#endif 
