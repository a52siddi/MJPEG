/*************************************************************************************
 * vim :set ts=8:
 ******************************************************************************/
#ifndef __CONV_H__
#define __CONV_H__

#include <stdint.h>

extern void YCbCr_to_ARGB(uint8_t *YCbCr_MCU[3], uint32_t* RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V);

//void YCbCr_to_ARGB(uint8_t YCbCr_MCU[0], uint8_t YCbCr_MCU[1], uint8_t YCbCr_MCU[2], uint32_t* RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V)

extern void to_NB(uint8_t  *YCbCr_MCU[3], uint32_t *RGB_MCU,
		uint32_t nb_MCU_H, uint32_t nb_MCU_V);

#endif
