#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <General.h>
#include "sd_card.h"
#include "ff.h"
#include "diskio.h"
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"

// Definições e constantes
#define ADC_PIN 26

// Protótipos de funções
void save_data_to_SD(int16_t acc[][3], int16_t gyro[][3], int size, int sample_offset);
void read_file(const char *filename);
void run_setrtc(void);
void run_format(void);
void run_mount(void);
void run_unmount(void);
void run_getfree(void);
void run_ls(void);
void run_cat(void);

#endif // SD_LOGGER_H
