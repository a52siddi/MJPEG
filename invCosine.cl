#define  IDCT_INT_MIN   (- IDCT_INT_MAX - 1)
#define  IDCT_INT_MAX   2147483647

#define c0_1  16384
#define c0_s2 23170
#define c1_1  16069
#define c1_s2 22725
#define c2_1  15137
#define c2_s2 21407
#define c3_1  13623
#define c3_s2 19266
#define c4_1  11585
#define c4_s2 16384
#define c5_1  9102
#define c5_s2 12873
#define c6_1  6270
#define c6_s2 8867
#define c7_1  3196
#define c7_s2 4520
#define c8_1  0
#define c8_s2 0
#define sqrt2 c0_s2

#define HUFF_DC     0
#define HUFF_AC     1
#define HUFF_EOB    0x00
#define HUFF_ZRL    0xF0

#define Y(i,j)          Y[(i << 3) + j]
#define X(i,j)          (output[(i << 3) + j])


__global void* malloc(size_t size, global uchar *heap, global uint *next)
{
  uint index = atomic_add(next, size);
  return heap+index;
}


__constant uchar G_ZZ[64] = {
	0, 1, 8, 16, 9, 2, 3, 10,
	17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};


#define ARITH_BITS      16



#define ARITH_MIN       (-1 << (ARITH_BITS-1))



#define ARITH_MAX       (~ARITH_MIN)



#define S_BITS           3


#define C_BITS          14

#define SCALE(x,n)      ((x) << (n))



#define but(a,b,x,y)    { x = SUB(a,b); y = ADD(a,b); }




typedef struct _huff_table_t {
	ushort code;
	char  value;
	char is_elt;
	struct _huff_table_t *parent;
	struct _huff_table_t *left;
	struct _huff_table_t *right;
} huff_table_t;

typedef struct _scan_desc_t {
	uchar bit_count;
	uchar window;
	char pred[3];
	huff_table_t *table[2][3];
} scan_desc_t;



 
static  int DESCALE (int x, int n)
{
	return (x + (1 << (n - 1)) - (x < 0)) >> n;
}



static  int ADD(int x, int y)
{
	int r = x + y;

	return r;       
}

static  int SUB(int x, int y)
{
	int r = x - y;

	return r;        
}

static  int CMUL(int c, int x)
{
	int r = c * x;

	

	r = (r + (1 << (C_BITS - 1))) >> C_BITS;
	return r;
}


static  void rot(int f, int k, int x, int y, int *rx, int *ry) {
	int COS[2][8] = {
		{c0_1, c1_1, c2_1, c3_1, c4_1, c5_1, c6_1, c7_1},
		{c0_s2, c1_s2, c2_s2, c3_s2, c4_s2, c5_s2, c6_s2, c7_s2}
	};

	*rx = SUB(CMUL(COS[f][k], x), CMUL(COS[f][8 - k], y));
	*ry = ADD(CMUL(COS[f][8 - k], x), CMUL(COS[f][k], y));
}


static int reformat (uint S, int good)
{
	int St = 0;

	if (good == 0) return 0;
	St = 1 << (good - 1);   /* 2^(good-1) */

	if (S < St) return (S + 1 + ((-1) << good));
	else return S;
}




void idct_1D(int *Y);

__kernel void IDCT(__global int* input, __global uchar* output) 
{



    int Y[64]; 
	int k,l;
	int Yc[8];
	
		for (k = 0; k < 8; k++)
	{
		for (l = 0; l < 8; l++)
		{

		Y(k,l) = SCALE(input[(k << 3) + l], S_BITS);	
		
		}			
		idct_1D(&Y(k,0));
	   
}
	for (l = 0; l < 8; l++)
	{
	
	for (k = 0; k < 8; k++)
			{Yc[k] = Y(k, l);}
	

		idct_1D(Yc);

         for (k = 0; k < 8; k++)
		{
		
			int r = 128 + DESCALE(Yc[k], S_BITS + 3);
			r = r > 0 ? (r < 255 ? r : 255) : 0;
			X(k, l) = r;
		}
		
	}
}

void idct_1D(int *Y) 
{

int z1[8], z2[8], z3[8];


	but(Y[0], Y[4], z1[1], z1[0]);
	rot(1, 6, Y[2], Y[6], &z1[2], &z1[3]);
	but(Y[1], Y[7], z1[4], z1[7]);
	z1[5] = CMUL(sqrt2, Y[3]);
	z1[6] = CMUL(sqrt2, Y[5]);

	but(z1[0], z1[3], z2[3], z2[0]);
	but(z1[1], z1[2], z2[2], z2[1]);
	but(z1[4], z1[6], z2[6], z2[4]);
	but(z1[7], z1[5], z2[5], z2[7]);

	z3[0] = z2[0];
	z3[1] = z2[1];
	z3[2] = z2[2];
	z3[3] = z2[3];
	rot(0, 3, z2[4], z2[7], &z3[4], &z3[7]);
	rot(0, 1, z2[5], z2[6], &z3[5], &z3[6]);

	but(z3[0], z3[7], Y[7], Y[0]);
	but(z3[1], z3[6], Y[6], Y[1]);
	but(z3[2], z3[5], Y[5], Y[2]);
	but(z3[3], z3[4], Y[4], Y[3]);
} 
 
 
/*-----------------Upsampling---------------------------*/

__kernel void upsampler(__global uchar *MCU_ds, __global uchar *MCU_us, uchar h_factor, uchar v_factor, ushort nb_MCU_H, ushort nb_MCU_V) 
{
    
    
	int us_index = 0, ds_index = 0 , index = 0, c_index;
	int base_index = 0;
	int stop_cond = 64 * nb_MCU_V * nb_MCU_H;
	
	__global uchar* base= MCU_ds+base_index;
	__global uchar* usIndex= MCU_us+us_index;
	
	
	if ((v_factor == 1) && (h_factor == 1)) {
	
		while (us_index < stop_cond) {
			//
			int increment = ((64 * nb_MCU_H) - 8);
			for (index = 0 ; index < nb_MCU_H ; index ++) {
			
			usIndex= (base + 8);
		
				us_index += 8;
				base_index += 64;
			}
			base_index -= increment	;
			if (base_index == 64) {
				base_index = 64 * nb_MCU_H;
			}
		}
	} else {
		

		while (us_index < stop_cond) {
			base_index = us_index;
		
			for(c_index = 0;c_index < 8;c_index++)
			{
				

				for (index = 0 ; index < h_factor ; index++) {
					MCU_us[us_index] = MCU_ds[ds_index];
					us_index++;
				}
				ds_index++;
			}
		
			for (index = 1 ; index < v_factor ; index++) {
				MCU_us[us_index]= MCU_ds[base_index];
				us_index += 8 * nb_MCU_H;
			}
		}
	}
	
	
}

/*---------------YCbCr2RGB----------------------------*/

__kernel void YCbCr_to_ARGB(__global uchar* Y_GPU, __global uchar* Cb_GPU, __global uchar* Cr_GPU, __global uint* RGB_MCU, uint nb_MCU_H, uint nb_MCU_V)
{
   
     
	__global uchar *MCU_Y, *MCU_Cb, *MCU_Cr;
	int R, G, B;
	uint ARGB;
	uchar index, i, j;
	
unsigned char	iid= get_global_id(0);
unsigned char	jid= get_global_id(1);

	// MCU_Y = &YCbCr_MCU[0];
	  // MCU_Cb = &YCbCr_MCU[1];
	  // MCU_Cr = &YCbCr_MCU[2];
	  
	  MCU_Y= Y_GPU;
	  MCU_Cb= Cb_GPU;
	  MCU_Cr= Cr_GPU;

	if (iid <= (8 * nb_MCU_V) && jid <= (8 * nb_MCU_H))
	{
	
	  
	
			index = iid * (8 * nb_MCU_H)  + jid;
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
		RGB_MCU[(iid * (8 * nb_MCU_H) + jid)] = ARGB;
		
		}
}


/*---------------IQZZ----------------------------*/

__kernel void iqzz_block(__global int in[64], __global int out[64],
		__global uchar table[64])
{
	uint index= get_global_id(0);

	if (index < 64)
	{
	
		out[G_ZZ[index]] = in[index] * table[index];
	}
}

