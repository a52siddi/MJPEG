
__kernel void YCbCr_to_ARGB(uint8_t *YCbCr_MCU[3], uint32_t *RGB_MCU, uint32_t nb_MCU_H, uint32_t nb_MCU_V)
{

	uint8_t *MCU_Y, *MCU_Cb, *MCU_Cr;
	int R, G, B;
	uint32_t ARGB;
	uint8_t index, i, j;

	MCU_Y = YCbCr_MCU[0];
	MCU_Cb = YCbCr_MCU[1];
	MCU_Cr = YCbCr_MCU[2];
	for (i = 0; i < 8 * nb_MCU_V; i++) {
		for (j = 0; j < 8 * nb_MCU_H; j++) {
			index = i * (8 * nb_MCU_H)  + j;
			R = (MCU_Cr[index] - 128) * 1.402f + MCU_Y[index];
			B = (MCU_Cb[index] - 128) * 1.7772f + MCU_Y[index];
			G = MCU_Y[index] - (MCU_Cb[index] - 128) * 0.34414f -
				(MCU_Cr[index] - 128) * 0.71414f;
			/* Saturate */
			if (R > 255)
				R = 255;
			if (R < 0)
				R = 0;
			if (G > 255)
				G = 255;
			if (G < 0)
				G = 0;
			if (B > 255)
				B = 255;
			if (B < 0)
				B = 0;
			ARGB = ((R & 0xFF) << 16) | ((G & 0xFF) << 8) | (B & 0xFF);
			// ARGB = 0xFF << 8;
			RGB_MCU[(i * (8 * nb_MCU_H) + j)] = ARGB;
		}
	}
}