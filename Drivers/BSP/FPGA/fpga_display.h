#ifndef __FPGA_DISPLAY_H
#define __FPGA_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define FPGA_DISPLAY_LINE_MAX      160U
#define FPGA_DISPLAY_TEXT_MAX      96U

typedef enum
{
    FPGA_DISPLAY_RESULT_NONE = 0,
    FPGA_DISPLAY_RESULT_SMS,
    FPGA_DISPLAY_RESULT_RX,
    FPGA_DISPLAY_RESULT_BATTERY,
    FPGA_DISPLAY_RESULT_STATUS,
    FPGA_DISPLAY_RESULT_TEST,
    FPGA_DISPLAY_RESULT_LOG,
    FPGA_DISPLAY_RESULT_BAD_LINE
} fpga_display_result_t;

void FPGA_DisplayInit(void);
void FPGA_DisplayPollUsart1(void);
fpga_display_result_t FPGA_DisplayProcessLine(const char *line);

#endif
