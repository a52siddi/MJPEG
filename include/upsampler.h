/*************************************************************************************
 * vim :set ts=8:
 ******************************************************************************/
#ifndef UPSAMPLER_H
#define UPSAMPLER_H

#include<stdint.h>




extern void upsampler(uint8_t *MCU_ds, uint8_t *MCU_us,
		uint8_t h_factor, uint8_t v_factor,
		uint16_t nb_MCU_H, uint16_t nb_MCU_V);

#endif
