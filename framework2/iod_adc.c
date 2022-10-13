#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "iod_main.h"

#define IOD_ADC_FLT_NUM     (4)

#define ADC0_GPIO_GP26      GPIO_GP26_ADC
#define ADC1_GPIO_GP27      GPIO_GP27_ADC
#define ADC2_GPIO_GP28      GPIO_GP28_ADC
#define ADC0_CHANNEL        (0)
#define ADC1_CHANNEL        (1)
#define ADC2_CHANNEL        (2)

enum iod_adc_group {
    ADC_VREST = 0,
    ADC_GYRO1,
    ADC_GYRO2,
    ADC_GROUP_NUM
};

struct iod_adc_filter {
    uint16_t au16_value_mem[IOD_ADC_FLT_NUM];
    uint16_t u16_value;
    uint8_t u8_count;
    uint8_t u8_index;
};

const uint8_t acu8s_adc_channel[ADC_GROUP_NUM] = {
    ADC0_CHANNEL,
    ADC1_CHANNEL,
    ADC2_CHANNEL
};

static struct iod_adc_filter sts_adc_filter[ADC_GROUP_NUM];

static void iod_adc_filter(uint16_t, struct iod_adc_filter *);

// 外部公開関数
void iod_adc_init() {
    // ADC0の初期設定
    adc_init();
    adc_gpio_init(ADC0_GPIO_GP26);
    adc_gpio_init(ADC1_GPIO_GP27);
    adc_gpio_init(ADC2_GPIO_GP28);

    // ADC0のフィルタ初期設定
    memset(&sts_adc_filter, 0, sizeof(sts_adc_filter));
}

void iod_adc_deinit() {
}

void iod_adc_reinit() {
}

void iod_adc_main_1ms() {
}

void iod_adc_main_in() {
    uint8_t u8a_index;

    // ADC0のフィルタ処理
    for (u8a_index = 0;u8a_index < ADC_GROUP_NUM; u8a_index++) {
        adc_select_input(acu8s_adc_channel[u8a_index]);
        uint16_t u16a_adc_value_now = adc_read();
        iod_adc_filter(u16a_adc_value_now, &sts_adc_filter[u8a_index]);
    }
}

void iod_adc_main_out() {
}

void iod_read_vrest_value(uint16_t *pu16a_value) {
    *pu16a_value = sts_adc_filter[ADC_VREST].u16_value;
}

void iod_read_gyro1_value(uint16_t *pu16a_value) {
    *pu16a_value = sts_adc_filter[ADC_GYRO1].u16_value;
}

void iod_read_gyro2_value(uint16_t *pu16a_value) {
    *pu16a_value = sts_adc_filter[ADC_GYRO2].u16_value;
}

// 内部関数
static void iod_adc_filter(uint16_t u16a_value_now, struct iod_adc_filter *psta_filter) {
    uint32_t u32a_value_sum = 0;
    uint8_t u8a_index;

    // フィルタの個数分、ADCを記憶する
    psta_filter->au16_value_mem[psta_filter->u8_index] = u16a_value_now;
    psta_filter->u8_index++;
    psta_filter->u8_count++;
    // インデックスとカウンタを修正
    if (psta_filter->u8_index >= IOD_ADC_FLT_NUM) {
        psta_filter->u8_index = 0;
    }
    if (psta_filter->u8_count >= IOD_ADC_FLT_NUM) {
        psta_filter->u8_count = IOD_ADC_FLT_NUM;
    }
    // ADCの平均値を計算する
    for (u8a_index = 0; u8a_index < psta_filter->u8_count; u8a_index++) {
        u32a_value_sum += psta_filter->au16_value_mem[u8a_index];
    }
    psta_filter->u16_value = (uint16_t)(u32a_value_sum / psta_filter->u8_count);
}
