#include <Arduino.h>
#include "stdlib.h"

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

    Serial.begin(256000);

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

}

void loop() {
  // put your setup code here, to run once:
        int ret;
        ret = adxl355_get_xyz(adxl355, &x, &y, &z);
        if (ret < 0)
            error();
        printf("%lld \t",x.integer);
        printf("%lld \t",y.integer);
        printf("%lld \n",z.integer);

        x.fractional = (x.fractional < 0)?(-x.fractional):x.fractional;
        y.fractional = (y.fractional < 0)?(-y.fractional):y.fractional;
        z.fractional = (z.fractional < 0)?(-z.fractional):z.fractional;

        Serial.print(x.integer);
        Serial.print(".");
        Serial.print(x.fractional);
        Serial.print("\t");
        Serial.print(y.integer);
        Serial.print(".");
        Serial.print(y.fractional);
        Serial.print("\t");
        Serial.print(z.integer);
        Serial.print(".");
        Serial.println(z.fractional);

        delay(10);
}
