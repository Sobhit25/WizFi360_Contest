#include <Arduino.h>
#include "WizFi360_Sobhit_inferencing.h"
#include "stdlib.h"

#define debug_nn true

extern "C" {
#include "adxl355.h"
#include "no_os_spi.h"
#include "pico_spi.h"


#define SPI_DEVICE_ID   1
#define SPI_BAUDRATE    1000000
//#define SPI_CS          SPI1_CS_GP13
//#define SPI_OPS         &pico_spi_ops
//#define SPI_EXTRA       &adxl355_spi_extra_ip

  // Read single accel data 
struct adxl355_frac_repr x;
struct adxl355_frac_repr y;
struct adxl355_frac_repr z;

struct pico_spi_init_param adxl355_spi_extra_ip = {
	.spi_tx_pin = SPI1_TX_GP11,
	.spi_rx_pin = SPI1_RX_GP12,
	.spi_sck_pin = SPI1_SCK_GP10,
	.spi_cs_pin = SPI1_CS_GP13
};

static struct adxl355_dev *adxl355;

// Particular SPI configuration
// static struct no_os_spi_init_param adxl355_spi_init = {
// 	.max_speed_hz = 1000000,
//     .mode = 0,
// 	.bit_order = NO_OS_SPI_BIT_ORDER_MSB_FIRST,
//     .extra = &adxl355_spi_extra_ip
// };

no_os_spi_init_param adxl_spi_init= {
            .device_id = SPI_DEVICE_ID,
            .max_speed_hz = SPI_BAUDRATE,
            .chip_select = SPI1_CS_GP13,
            .mode = NO_OS_SPI_MODE_0,
            .bit_order = NO_OS_SPI_BIT_ORDER_MSB_FIRST,
            .platform_ops = &pico_spi_ops,
            .extra = &adxl355_spi_extra_ip
};

struct adxl355_init_param init_data_adxl355 = {
	.comm_init = { 
        .spi_init = adxl_spi_init },
	.comm_type = ADXL355_SPI_COMM
};

}

void error(void){
    printf("Initialization Error \r\n");
    while(1);
}

void setup() {
    int ret;
    
    ret = adxl355_init(&adxl355, init_data_adxl355);
    if (ret < 0) 
        error();

    ret = adxl355_set_range(adxl355, ADXL355_RANGE_8G);
    if (ret < 0) 
        error();

    //ret = adxl355_soft_reset(adxl355);
    //sleep_ms(250);
    if (ret < 0)
        error();
    // ret = adxl355_set_odr_lpf(adxl355, ADXL355_ODR_4000HZ);
    // if (ret < 0)
    //     error();
    ret = adxl355_set_op_mode(adxl355, ADXL355_MEAS_TEMP_OFF_DRDY_OFF);
    if (ret < 0)
        error();

    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3) {
    ei_printf("ERR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be equal to 3 (the 3 sensor axes)\n");
    return;
    }
}

void loop() {
  // put your setup code here, to run once:
        int ret;
ei_printf("\nStarting inferencing in 2 seconds...\n");

    delay(2000);

    ei_printf("Sampling...\n");

    // Allocate a buffer here for the values we'll read from the IMU
    float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };

    for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3) {
        // Determine the next tick (and then sleep later)
        uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);
        ret = adxl355_get_xyz(adxl355, &x, &y, &z);
        if (ret < 0)
            error();

        x.fractional = (x.fractional < 0)?(-x.fractional):x.fractional;
        y.fractional = (y.fractional < 0)?(-y.fractional):y.fractional;
        z.fractional = (z.fractional < 0)?(-z.fractional):z.fractional;

        buffer[ix] = (x.integer>0)?(x.integer + (x.fractional >> 28)/10000):(x.integer - (x.fractional >> 28)/10000);
        buffer[ix + 1] = (y.integer>0)?(y.integer + (y.fractional >> 28)/10000):(y.integer - (y.fractional >> 28)/10000);
        buffer[ix + 2] = (z.integer>0)?(z.integer + (z.fractional >> 28)/10000):(z.integer - (z.fractional >> 28)/10000);

        delayMicroseconds(next_tick - micros());
    }

    // Turn the raw buffer in a signal which we can the classify
    signal_t signal;
    int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    if (err != 0) {
        ei_printf("Failed to create signal from buffer (%d)\n", err);
        return;
    }

    // Run the classifier
    ei_impulse_result_t result = { 0 };

    err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", err);
        return;
    }

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif
}
