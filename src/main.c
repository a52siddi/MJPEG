/*************************************************************************************
 * vim :set ts=8:
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <malloc.h>
#include <CL/cl.h>

#include "jpeg.h"
#include "utils.h"
#include "screen.h"
#include "errno.h"
#include "define_common.h"
#include "iqzz.h"
#include "unpack_block.h"
#include "idct.h"
#include "conv.h"
#include "upsampler.h"
#include "huffman.h"
#include "skip_segment.h"
#include "SDL/SDL.h"

#define MAX_SOURCE_SIZE (0x100000)

////variables for profiling the code/////////

//cl_ulong prof_iqzz = 0;
//cl_ulong prof_IDCT = 0;
//cl_ulong prof_upsample = 0;
//cl_ulong prof_Colorconv = 0;

void chk(cl_int status, const char* cmd) {

   if(status != CL_SUCCESS) {
      printf("%s failed (%d)\n", cmd, status);
      exit(-1);
   }
}

static void usage(char *progname) {
	printf("Usage : %s [options] <mjpeg_file>\n", progname);
	printf("Options list :\n");
	printf("\t-f <framerate> : set framerate for the movie\n");
	printf("\t-h : display this help and exit\n");
}

#define CL_CHECK(_expr)                                                         \
   do {                                                                         \
     cl_int _err = _expr;                                                       \
     if (_err == CL_SUCCESS)                                                    \
       break;                                                                   \
     fprintf(stderr, "OpenCL Error: '%s' returned %d!\n", #_expr, (int)_err);   \
     abort();                                                                   \
   } while (0)

#define CL_CHECK_ERR(_expr)                                                     \
   ({                                                                           \
     cl_int _err = CL_INVALID_VALUE;                                            \
     typeof(_expr) _ret = _expr;                                                \
     if (_err != CL_SUCCESS) {                                                  \
       fprintf(stderr, "OpenCL Error: '%s' returned %d!\n", #_expr, (int)_err); \
       abort();                                                                 \
     }                                                                          \
     _ret;                                                                      \
   })


void pfn_notify(const char *errinfo, const void *private_info, size_t cb, void *user_data)
{
	fprintf(stderr, "OpenCL Error (via pfn_notify): %s\n", errinfo);
}

cl_ulong checkTime(cl_event ev)
{
    cl_ulong start;
    cl_ulong end;

    clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_END,   sizeof(cl_ulong), &end,   NULL);

    return (end - start);
}



int main(int argc, char *argv[])
{

	uint8_t marker[2];
	uint8_t HT_type = 0;
	uint8_t HT_index = 0;
	uint8_t DQT_table[4][64];
	uint8_t QT_index = 0;
	uint16_t nb_MCU = 0, nb_MCU_X = 0, nb_MCU_Y = 0;
	cl_uint max_ss_h = 0 , max_ss_v = 0;
//	uint16_t max_ss_h = 0 , max_ss_v = 0;
cl_int index = 0, index_X, index_Y, index_SOF;

//    uint8_t component_order[4];
//	int32_t MCU[64];
//	int32_t unZZ_MCU[64];
//	uint8_t *YCbCr_MCU[3] = { NULL, NULL, NULL};
//	uint8_t *YCbCr_MCU_ds[3] = { NULL, NULL, NULL};
//	uint32_t *RGB_MCU = NULL;
//	uint32_t YH = 0, YV = 0;


	cl_uchar component_order[4];
	cl_int MCU[64];
	//cl_int newMCU[64];
	cl_int unZZ_MCU[64];
	cl_uchar* YCbCr_MCU[3] = { NULL, NULL, NULL};
//	cl_uchar* YCbCr_MCU[3];
	cl_uchar *YCbCr_MCU_ds[3] = { NULL, NULL, NULL};
	cl_uchar *Y_ForGPU= NULL;
	cl_uchar *Cb_ForGPU= NULL;
	cl_uchar *Cr_ForGPU= NULL;
	cl_uint* RGB_testing= NULL;
	cl_uint* RGB_MCU = NULL;
	uint32_t YH = 0, YV = 0;
	uint32_t CrH = 0, CrV = 0, CbH = 0, CbV = 0;
	uint32_t screen_init_needed;
	uint32_t end_of_file;
	int chroma_ss;
	int args;
	uint32_t framerate = 0;
	uint32_t color = 1;

	cl_mem for_Y= NULL;
	cl_mem for_Cb= NULL;
	cl_mem for_Cr= NULL;

//	 cl_event ev_iqzz;
//	 cl_event ev_IDCT;
//	 cl_event ev_upsample;
//	 cl_event ev_Colorconv;




	/////-----Allocate space for the result on the host side------//////




//	  cl_int n;
//	  cl_int m;
//	  size_t gws[2];
//	   size_t lws[2];

	FILE *movie = NULL;
	jfif_header_t jfif_header;
	DQT_section_t DQT_section;
	SOF_section_t SOF_section;
	SOF_component_t SOF_component[3];
	DHT_section_t DHT_section;
	SOS_section_t SOS_section;
	SOS_component_t SOS_component[3];

	scan_desc_t scan_desc = { 0, 0, {}, {} };
	huff_table_t *tables[2][4] = {
		{NULL , NULL, NULL, NULL} ,
		{NULL , NULL, NULL, NULL}
	};



	  //////////////OpenCL parameters/////////////////////////////

	        //int out;
	        cl_platform_id platform_id;
	        cl_uint ret_num_platforms;
	        cl_device_id device_id;
	        cl_uint ret_num_devices;
	        cl_context context;
	        cl_command_queue command_queue;
	        cl_program program;
	        //size_t kernel_code_size;
	        //int *result;
	        cl_int ret;
	        cl_kernel kernel= NULL;
	        cl_kernel cos_kernel= NULL;
	        cl_kernel sample_kernel= NULL;
	        cl_kernel color_kernel= NULL;
//	        cl_ulong prof_DCTstart, prof_DCTend;
//	        cl_event inv_dct;
	        //cl_uchar storeResult;

	        //storeResult= (cl_uchar *)malloc(3 * sizeof(cl_uchar));


	        	       	 FILE *fp;
//	        	       	 const char fileName[] = "/root/Downloads/tima_seq_version/src/invCosine.cl";
	        	       	 const char fileName[] = "./invCosine.cl";
	        	       	 size_t source_size;
	        	       	 char *source_str;

	        	     	/* Load kernel source file */
	        	     	fp = fopen(fileName, "rb");
	        	     	if (!fp) {
	        	     		fprintf(stderr, "Failed to load kernel.\n");
	        	     		exit(1);
	        	     	}


	        	     	source_str = (char *)malloc(MAX_SOURCE_SIZE);
	             	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	             	fclose(fp);

	    /////////Set platform, context, command-queue.........../////////////////////////

	        /* Get Platform */
	          ret= clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	           if (ret_num_platforms == 0)
	              {
	                  printf("Found 0 platforms!\n");
                  return EXIT_FAILURE;
	              }
	           /* Get Device */
	           ret= clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);
	           printf("Number of devices: %d\n", ret_num_devices);

	           /* Create Context */
	           context = clCreateContext(0, 1, &device_id, NULL, NULL, &ret);

	           if (!context)
	           {
	        	   printf("NO cCONTEXT\n");
	        	   return EXIT_FAILURE;

	           }

	           /* Create Command Queue */
	           command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &ret);

	           if (!command_queue)
	           {
	        	   printf("NO command queue\n");
	        	   return EXIT_FAILURE;

	           }



	        	/* Create kernel from source */
	       	       	program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);

	       	       	if (!program)
	       	       	{
	       	       		printf("NO PROGRAM!!!!\n");
	       	       		return EXIT_FAILURE;
	       	       	}

	       	       	ret= clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);



	       	        if (ret != CL_SUCCESS)
	       	        {
	       	               printf("building program failed\n");
	       	            size_t log_size;
	       	            char buffer[10240];


//	       	         char buffer[10240];
//	       	         clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
//	       	         fprintf(stderr, "CL Compilation failed:\n%s", buffer);
	       	               //if (ret == CL_)
	       	               //{

	       	                   clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
	       	                   clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &log_size);
	       	                   printf("%s\n", buffer);
	       	               //}
	       	           }

              //--------kernel for iqzz-----------//
	       	  kernel= clCreateKernel(program, "iqzz_block", &ret);

	       	  //-------kernel for idct-----------//
	       	  cos_kernel= clCreateKernel(program, "IDCT", &ret);

	       	  //------kernel for upsampling-------//
	       	  sample_kernel= clCreateKernel(program, "upsampler", &ret);

	       	  color_kernel= clCreateKernel(program, "YCbCr_to_ARGB", &ret);

	       	if(ret != CL_SUCCESS)
	       	{
	       		printf("-----COULD NOT CREATE KERNEL!!---\n");
	       		exit(1);
	       	}


	       	//------Allocate memory for color elements--------//

	       	Y_ForGPU= Cb_ForGPU= Cr_ForGPU= (cl_uchar *)malloc(MCU_sx * MCU_sy * max_ss_h * max_ss_v);
	         Cb_ForGPU= (cl_uchar *)malloc(MCU_sx * MCU_sy * max_ss_h * max_ss_v);
	       		Cr_ForGPU= (cl_uchar *)malloc(MCU_sx * MCU_sy * max_ss_h * max_ss_v);

	       		//Now will do it for RGB
	       		RGB_testing= (cl_uint *)malloc (MCU_sx * MCU_sy * max_ss_h * max_ss_v * sizeof(cl_int));


	           /* Create buffer object */
//	           cl_mem DCT_Input = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, 64 * sizeof(int32_t), NULL, &ret);
//
//	           //Output buffer
//	           cl_mem  DCT_Output = clCreateBuffer(context, CL_MEM_READ_WRITE, 3 * sizeof(uint8_t), NULL, &ret);

	    	/* Copy input data to the memory buffer */
	    //	ret = clEnqueueWriteBuffer(command_queue, DCT_Input, CL_TRUE, 0, 64 * sizeof(int32_t), unZZ_MCU, 0, NULL, NULL);


	    /////////////-------------------Create Buffers--------------------------------///////////////////

	        cl_mem block_GPU = clCreateBuffer(context, CL_MEM_READ_WRITE, 64 * sizeof(cl_int), NULL, &ret);
	       	chk(ret, "clCreateBuffer");

//This will serve as the output buffer for iqzz
	     cl_mem DCT_Input = clCreateBuffer(context, CL_MEM_READ_WRITE| CL_MEM_COPY_HOST_PTR, 64 * sizeof(cl_int), unZZ_MCU, &ret);
	     chk(ret, "clCreateBuffer");

	     cl_mem qtable_GPU = clCreateBuffer(context, CL_MEM_READ_WRITE, 64 * sizeof(cl_uchar), NULL, &ret);
	     chk(ret, "clCreateBuffer");



//		for_Cb= clCreateBuffer(context, CL_MEM_READ_WRITE| CL_MEM_COPY_HOST_PTR, (MCU_sx * MCU_sy * max_ss_h * max_ss_v), Cb_ForGPU , &ret);
//       chk(ret, "clCreateBuffer");

	    ///////////UPSAMPLING//////////////////



//	     //h_factor for upsampling
//	     cl_mem Hfactor_GPU= clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uchar), NULL, &ret);
//	     chk(ret, "clCreateBuffer");
//
//	     //v_factor for upsampling
//	     cl_mem Vfactor_GPU= clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uchar), NULL, &ret);
//	     chk(ret, "clCreateBuffer");
//
//	     //mcu_h for upsampling
//	     cl_mem H_MCU_GPU= clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ushort), NULL, &ret);
//	     chk(ret, "clCreateBuffer");
//
//	     //mcu_v for upsampling
//	     cl_mem V_MCU_GPU= clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ushort), NULL, &ret);
//	     chk(ret, "clCreateBuffer");

//	     //YCbCr_MCU for YCbCrtoARGB
//	 	     cl_mem colorMCU_GPU= clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR, 3 * (MCU_sx * MCU_sy * max_ss_h * max_ss_v), YCbCr_MCU, &ret);
//	 	     chk(ret, "clCreateBuffer");
//
//	     //rgb_MCU for YCbCrtoARGB
//	 	     cl_mem RGB_GPU= clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,  MCU_sx * MCU_sy * max_ss_h * max_ss_v * sizeof(cl_int), RGB_MCU, &ret);
//	 	     chk(ret, "clCreateBuffer");



//Make a new input buffer for IDCT
//     cl_mem DCT_NextStep = clCreateBuffer(context, CL_MEM_READ_WRITE, 64 * sizeof(cl_int), NULL, &ret);
//	     chk(ret, "clCreateBuffer");


	     /////////--------------Transfer data to buffers------------------------///////////////////

//	     ret = clEnqueueWriteBuffer(command_queue, DCT_Input, CL_FALSE, 0, 64 * sizeof(cl_int), unZZ_MCU, 0, NULL, NULL);
//	      chk(ret, "clEnqueueWriteBuffer");


	screen_init_needed = 1;
	if (argc < 2) {
		usage(argv[0]);
		exit(1);
	}
	args = 1;

	while (args < argc) {
		if (argv[args][0] ==  '-') {
			switch(argv[args][1]) {
				case 'f':
					if ((argc - args) < 1) {
						usage(argv[0]);
						exit(1);
					}
					framerate = atoi(argv[args+1]);
					args +=2;
					break;
				case 'h':
					usage(argv[0]);
					exit(0);
					break;
				case 'c':
					color=0;
					args++;
					break;
				default :
					usage(argv[0]);
					exit(1);
					break;
			}
		} else {
			break;
		}
	}

	if ((argc - args) != 1) {
		usage(argv[0]);
		exit(1);
	}

	if ((movie = fopen(argv[args], "r")) == NULL) {
		perror(strerror(errno));
		exit(-1);
	}

	/*---- Actual computation ----*/
	end_of_file = 0;
	while (!end_of_file ) {
		int elem_read = fread(&marker, 2, 1, movie);

		if (elem_read != 1) {
			if (feof(movie)) {
				end_of_file = 1;
				break;
			}
			else {
				fprintf(stderr, "Error reading marker from input file\n");
				exit (1);
			}
		}

		if (marker[0] == M_SMS) {
			switch (marker[1]) {
				case M_SOF0:
					{
						IPRINTF("SOF0 marker found\r\n");

						COPY_SECTION(&SOF_section, sizeof(SOF_section));
						CPU_DATA_IS_BIGENDIAN(16, SOF_section.length);
						CPU_DATA_IS_BIGENDIAN(16, SOF_section.height);
						CPU_DATA_IS_BIGENDIAN(16, SOF_section.width);

						VPRINTF("Data precision = %d\r\n",
								SOF_section.data_precision);
						VPRINTF("Image height = %d\r\n",
								SOF_section.height);
						VPRINTF("Image width = %d\r\n",
								SOF_section.width);
						VPRINTF("%d component%c\r\n",
								SOF_section.n,
								(SOF_section.n > 1) ? 's' : ' ');

						COPY_SECTION(&SOF_component,
								sizeof(SOF_component_t)* SOF_section.n);

						YV = SOF_component[0].HV & 0x0f;
						YH = (SOF_component[0].HV >> 4) & 0x0f;

						for (index = 0 ; index < SOF_section.n ; index++) {
							if (YV > max_ss_v) {
								max_ss_v = YV;
							}
							if (YH > max_ss_h) {
								max_ss_h = YH;
							}
						}

						if (SOF_section.n > 1) {
							CrV = SOF_component[1].HV & 0x0f;
							CrH = (SOF_component[1].HV >> 4) & 0x0f;
							CbV = SOF_component[2].HV & 0x0f;
							CbH = (SOF_component[2].HV >> 4) & 0x0f;
						}
						IPRINTF("Subsampling factor = %ux%u, %ux%u, %ux%u\r\n",
								YH, YV, CrH, CrV, CbH, CbV);

						if (screen_init_needed == 1) {
							screen_init_needed = 0;
							screen_init(SOF_section.width, SOF_section.height, framerate);

							nb_MCU_X = intceil(SOF_section.height, MCU_sx * max_ss_v);
							nb_MCU_Y = intceil(SOF_section.width, MCU_sy * max_ss_h);
							nb_MCU = nb_MCU_X * nb_MCU_Y;

							for (index = 0 ; index < SOF_section.n ; index++) {
								YCbCr_MCU[index] = malloc(MCU_sx * MCU_sy * max_ss_h * max_ss_v);
								YCbCr_MCU_ds[index] = malloc(MCU_sx * MCU_sy * max_ss_h * max_ss_v);

							}
//							RGB_MCU = malloc (MCU_sx * MCU_sy * max_ss_h * max_ss_v * sizeof(int32_t));

							RGB_MCU = malloc (MCU_sx * MCU_sy * max_ss_h * max_ss_v * sizeof(cl_int));
						}

						//---Setting color variables equal to array elements----//

						Y_ForGPU= YCbCr_MCU[0];
						Cb_ForGPU= YCbCr_MCU[1];
						Cr_ForGPU= YCbCr_MCU[2];

						//----RGB is being assigned value-----//

						RGB_testing= RGB_MCU;



						break;
					}

				case M_DHT:
					{
						// Depending on how the JPEG is encoded, DHT marker may not be
						// repeated for each DHT. We need to take care of the general
						// state where all the tables are stored sequentially
						// DHT size represent the currently read data... it starts as a
						// zero value
						volatile int DHT_size = 0;
						IPRINTF("DHT marker found\r\n");

						COPY_SECTION(&DHT_section.length, 2);
						CPU_DATA_IS_BIGENDIAN(16, DHT_section.length);
						// We've read the size : DHT_size is now 2
						DHT_size += 2;

						//We loop while we've not read all the data of DHT section
						while (DHT_size < DHT_section.length) {

							int loaded_size = 0;
							// read huffman info, DHT size ++
							NEXT_TOKEN(DHT_section.huff_info);
							DHT_size++;

							// load the current huffman table
							HT_index = DHT_section.huff_info & 0x0f;
							HT_type = (DHT_section.huff_info >> 4) & 0x01;
							IPRINTF("Huffman table index is %d\r\n", HT_index);
							IPRINTF("Huffman table type is %s\r\n",
									HT_type ? "AC" : "DC");

							VPRINTF("Loading Huffman table\r\n");
							tables[HT_type][HT_index] = (huff_table_t *) malloc(sizeof(huff_table_t));
							loaded_size = load_huffman_table_size(movie,
									DHT_section.length,
									DHT_section.huff_info,
									tables[HT_type][HT_index]);
							if (loaded_size < 0) {
								VPRINTF("Failed to load Huffman table\n");

								goto clean_end;
							}
							DHT_size += loaded_size;

							IPRINTF("Huffman table length is %d, read %d\r\n", DHT_section.length, DHT_size);
						}

						break;
					}

				case M_SOI:
					{
						IPRINTF("SOI marker found\r\n");
						break;
					}

				case M_EOI:
					{
						IPRINTF("EOI marker found\r\n");
						end_of_file = 1;
						break;
					}

				case M_SOS:
					{
						IPRINTF("SOS marker found\r\n");

						COPY_SECTION(&SOS_section, sizeof(SOS_section));
						CPU_DATA_IS_BIGENDIAN(16, SOS_section.length);

						COPY_SECTION(&SOS_component,
								sizeof(SOS_component_t) * SOS_section.n);
						IPRINTF("Scan with %d components\r\n", SOS_section.n);

						/* On ignore les champs Ss, Se, Ah, Al */
						SKIP(3);

						scan_desc.bit_count = 0;

						for (index = 0; index < SOS_section.n; index++) {
							/*index de SOS correspond à l'ordre d'apparition des MCU,
							 *  mais pas forcement égal à celui de déclarations des labels*/

//							/ * Index of SOS corresponds to the order of appearance of the MCUs,
//							* But not necessarily equal to that of declarations of labels * /
							for(index_SOF=0 ; index_SOF < SOF_section.n; index_SOF++) {
								if(SOF_component[index_SOF].index==SOS_component[index].index){
									component_order[index]=index_SOF;
									break;
								}
								if(index_SOF==SOF_section.n-1)
									VPRINTF("Invalid component label in SOS section");
							}


							scan_desc.pred[index] = 0;
							scan_desc.table[HUFF_AC][index] =
								tables[HUFF_AC][(SOS_component[index].acdc >> 4) & 0x0f];
							scan_desc.table[HUFF_DC][index] =
								tables[HUFF_DC][SOS_component[index].acdc & 0x0f];
						}


						for (index_X = 0; index_X < nb_MCU_X; index_X++) {

							for (index_Y = 0; index_Y < nb_MCU_Y; index_Y++) {
								for (index = 0; index < SOS_section.n; index++)
								{

									int component_index = component_order[index];
									//avoiding unneeded computation
									int nb_MCU = ((SOF_component[component_index].HV>> 4) & 0xf) * (SOF_component[component_index].HV & 0x0f);


									for (chroma_ss = 0; chroma_ss < nb_MCU; chroma_ss++)
									{
										unpack_block(movie, &scan_desc,index, MCU);

								/////--------------Transfer data to buffers----------------////////////

										ret = clEnqueueWriteBuffer(command_queue, block_GPU, CL_TRUE, 0, 64 * sizeof(cl_int), MCU, 0, NULL, NULL);
										chk(ret, "clEnqueueWriteBuffer");

										ret = clEnqueueWriteBuffer(command_queue, qtable_GPU, CL_TRUE, 0, 64 * sizeof(cl_uchar), DQT_table[SOF_component[component_index].q_table], 0, NULL, NULL);
										chk(ret, "clEnqueueWriteBuffer");


										/* Set OpenCL kernel arguments */
									    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&block_GPU);
										ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&DCT_Input);
										ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&qtable_GPU);
									    chk(ret, "clSetKernelArg");

									    size_t global=64;
									    size_t local= 32;

		   										 ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
		 										 chk(ret, "clEnqueueNDRange");
//		 									    ret = clWaitForEvents(1, &ev_iqzz);
//		 										prof_iqzz += checkTime(ev_iqzz);
//
////										//Execute kernel on the device
////										ret = clEnqueueTask(command_queue, kernel, 0, NULL, NULL);
////
										//Copy result from device to host
										ret = clEnqueueReadBuffer(command_queue, DCT_Input, CL_TRUE, 0, 64 * sizeof(cl_int), &unZZ_MCU, 0, NULL, NULL);
									    chk(ret, "clEnqueueRead");

									    clFinish(command_queue);


//										iqzz_block(MCU, unZZ_MCU,DQT_table[SOF_component[component_index].q_table]);


										////////////------------------Set kernel arguments-------------------////////////////////

								     //Output buffer
									     cl_mem  DCT_Output = clCreateBuffer(context, CL_MEM_READ_WRITE| CL_MEM_COPY_HOST_PTR, (MCU_sx * MCU_sy * max_ss_h * max_ss_v) + 4, YCbCr_MCU_ds[component_index] + (64 * chroma_ss), &ret);
									     chk(ret, "clCreateBuffer");
////
//////										 ret = clEnqueueWriteBuffer(command_queue, DCT_Input, CL_TRUE, 0, 64 * sizeof(cl_int), unZZ_MCU, 0, NULL, NULL);
//////										chk(ret, "clEnqueueWriteBuffer");
//////
//////
//////
////////										 ret = clEnqueueWriteBuffer(command_queue, DCT_Output, CL_TRUE, 0, (3 * sizeof(cl_uchar)) + sizeof(64 * chroma_ss), YCbCr_MCU_ds[component_index] + (64 * chroma_ss), 0, NULL, NULL);
////////										 chk(ret, "clEnqueueWriteBuffer");


										/* Set OpenCL kernel arguments */
										ret = clSetKernelArg(cos_kernel, 0, sizeof(cl_mem), (void *)&DCT_Input);
										ret |= clSetKernelArg(cos_kernel, 1, sizeof(cl_mem), (void *)&DCT_Output);
										chk(ret, "clSetKernelArg");

										size_t globalForInverseDCT=64;

		   										 ret = clEnqueueNDRangeKernel(command_queue, cos_kernel, 1, NULL, &globalForInverseDCT, NULL, 0, NULL, NULL);
		 										 chk(ret, "clEnqueueNDRange");
//		 									    ret = clWaitForEvents(1, &ev_IDCT);
//		 									   prof_IDCT += checkTime(ev_IDCT);

//										 //Execute kernel on the device
////										 ret = clEnqueueTask(command_queue, cos_kernel, 0, NULL, NULL);
//
////										 size_t global_size[2]= {1,1};
////
////										 ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global_size, NULL, 0, NULL, NULL);
////										 chk(ret, "clEnqueueNDRange");
//
										//Copy result to the host
										ret = clEnqueueReadBuffer(command_queue, DCT_Output, CL_TRUE, 0, (MCU_sx * MCU_sy * max_ss_h * max_ss_v) + 4, YCbCr_MCU_ds[component_index] + (64 * chroma_ss), 0, NULL, NULL);
										chk(ret, "clEnqueueRead");

									clFinish(command_queue);

//										IDCT(unZZ_MCU, YCbCr_MCU_ds[component_index] + (64 * chroma_ss));
									}
//////////////////-------OpenCL code for upsampling----------------/////////////////

								     //DCT input buffer for upsampling
								     cl_mem upsample_in=  clCreateBuffer(context, CL_MEM_READ_WRITE| CL_MEM_COPY_HOST_PTR, (MCU_sx * MCU_sy * max_ss_h * max_ss_v) ,YCbCr_MCU_ds[component_index], &ret);
								     chk(ret, "clCreateBuffer");

								     //DCT output buffer for upsampling
								     cl_mem upsample_out= clCreateBuffer(context, CL_MEM_READ_WRITE| CL_MEM_COPY_HOST_PTR, (MCU_sx * MCU_sy * max_ss_h * max_ss_v), YCbCr_MCU[component_index], &ret);
								     chk(ret, "clCreateBuffer");
//
////									 ret = clEnqueueWriteBuffer(command_queue, upsample_in, CL_TRUE, 0, 3*sizeof(cl_uchar), YCbCr_MCU_ds[component_index], 0, NULL, NULL);
////									 chk(ret, "clEnqueueWriteBuffer");
//
////									 ret = clEnqueueWriteBuffer(command_queue, Hfactor_GPU, CL_TRUE, 0,  sizeof(cl_uchar), (max_ss_h / ((SOF_component[component_index].HV >> 4) & 0xf)), 0, NULL, NULL);
////									chk(ret, "clEnqueueWriteBuffer");
////
////								   ret = clEnqueueWriteBuffer(command_queue, Vfactor_GPU, CL_TRUE, 0, sizeof(cl_uchar), &(max_ss_v / ((SOF_component[component_index].HV) & 0xf)), 0, NULL, NULL);
////								  chk(ret, "clEnqueueWriteBuffer");
////
////								ret = clEnqueueWriteBuffer(command_queue, H_MCU_GPU, CL_TRUE, 0, sizeof(cl_ushort), &max_ss_h, 0, NULL, NULL);
////								chk(ret, "clEnqueueWriteBuffer");
////
////							  ret = clEnqueueWriteBuffer(command_queue, V_MCU_GPU, CL_TRUE, 0, sizeof(cl_ushort), &max_ss_v, 0, NULL, NULL);
////							chk(ret, "clEnqueueWriteBuffer");

						cl_uchar Horizontal= (max_ss_h / (SOF_component[component_index].HV >> 4) & 0xf);
							cl_uchar Vertical= (max_ss_v / (SOF_component[component_index].HV) & 0xf);
////
							/* Set OpenCL kernel arguments */
							ret = clSetKernelArg(sample_kernel, 0, sizeof(cl_mem), (void *)&upsample_in);
							ret |= clSetKernelArg(sample_kernel, 1, sizeof(cl_mem), (void *)&upsample_out);
							ret |= clSetKernelArg(sample_kernel, 2, sizeof(cl_uchar), (void *)&Horizontal);
							ret |= clSetKernelArg(sample_kernel, 3, sizeof(cl_uchar), (void *)&Vertical);
							ret |= clSetKernelArg(sample_kernel, 4, sizeof(cl_ushort), (void *)&max_ss_h);
							ret |= clSetKernelArg(sample_kernel, 5, sizeof(cl_ushort), (void *)&max_ss_v);
							chk(ret, "clSetKernelArg");

							size_t globalForUpsample=1;

   							ret = clEnqueueNDRangeKernel(command_queue, sample_kernel, 1, NULL, &globalForUpsample, NULL, 0, NULL, NULL);
 						    chk(ret, "clEnqueueNDRange");

							 //Execute kernel on the device
////							 ret = clEnqueueTask(command_queue, sample_kernel, 0, NULL, NULL);
//							 ret = clWaitForEvents(1, &ev_upsample);
//						     prof_upsample += checkTime(ev_upsample);
//
//
//								//Copy result to the host
						ret = clEnqueueReadBuffer(command_queue, upsample_out, CL_TRUE, 0, (MCU_sx * MCU_sy * max_ss_h * max_ss_v), YCbCr_MCU[component_index], 0, NULL, NULL);
						chk(ret, "clEnqueueRead");


//									upsampler(YCbCr_MCU_ds[component_index], YCbCr_MCU[component_index],
//											max_ss_h / ((SOF_component[component_index].HV >> 4) & 0xf),
//											max_ss_v / ((SOF_component[component_index].HV) & 0xf),
//											max_ss_h,
//											max_ss_v);
//									upsampler(YCbCr_MCU_ds[component_index], YCbCr_MCU[component_index],
//											Horizontal,
//											 Vertical,
//											 max_ss_h,
//											 max_ss_v);
								}


								if(color&&(SOF_section.n>1))
								{


								     //YCbCr_MCU for YCbCrtoARGB

								 	     for_Y= clCreateBuffer(context, CL_MEM_READ_WRITE| CL_MEM_COPY_HOST_PTR, (MCU_sx * MCU_sy * max_ss_h * max_ss_v), Y_ForGPU, &ret);
								 	     chk(ret, "clCreateBuffer");

								 	    for_Cb= clCreateBuffer(context, CL_MEM_READ_WRITE| CL_MEM_COPY_HOST_PTR, (MCU_sx * MCU_sy * max_ss_h * max_ss_v), Cb_ForGPU , &ret);
								 	  	chk(ret, "clCreateBuffer");

								 	   for_Cr= clCreateBuffer(context, CL_MEM_READ_WRITE| CL_MEM_COPY_HOST_PTR, (MCU_sx * MCU_sy * max_ss_h * max_ss_v), Cr_ForGPU, &ret);
						                chk(ret, "clCreateBuffer");

								     //rgb_MCU for YCbCrtoARGB
								 	     cl_mem RGB_GPU= clCreateBuffer(context, CL_MEM_READ_WRITE, (MCU_sx * MCU_sy * max_ss_h * max_ss_v * sizeof(cl_int)), NULL, &ret);
								 	     chk(ret, "clCreateBuffer");

////								 	   ret = clEnqueueWriteBuffer(command_queue, for_Y, CL_FALSE, 0, sizeof(cl_uchar), Y_ForGPU, 0, NULL, NULL);
////								 	    chk(ret, "clEnqueueWriteBuffer");
////
////									 	 ret = clEnqueueWriteBuffer(command_queue, for_Cb, CL_FALSE, 0, sizeof(cl_uchar), Cb_ForGPU, 0, NULL, NULL);
////									 	   chk(ret, "clEnqueueWriteBuffer");
////
////										 	ret = clEnqueueWriteBuffer(command_queue, for_Cr, CL_FALSE, 0, sizeof(cl_uchar), Cr_ForGPU, 0, NULL, NULL);
////										 	 chk(ret, "clEnqueueWriteBuffer");
//
////								 	    ret = clEnqueueWriteBuffer(command_queue, RGB_GPU, CL_TRUE, 0,  sizeof(cl_uint), RGB_MCU, 0, NULL, NULL);
////								 	    chk(ret, "clEnqueueWriteBuffer");
//
//
//

							ret = clSetKernelArg(color_kernel, 0, sizeof(cl_mem), &for_Y);
								ret |= clSetKernelArg(color_kernel, 1, sizeof(cl_mem), &for_Cb);
							ret |= clSetKernelArg(color_kernel, 2, sizeof(cl_mem), &for_Cr);
							ret |= clSetKernelArg(color_kernel, 3, sizeof(cl_mem), &RGB_GPU);
									ret |= clSetKernelArg(color_kernel, 4, sizeof(cl_uint), &max_ss_h);
								ret |= clSetKernelArg(color_kernel, 5, sizeof(cl_uint), &max_ss_v);
									chk(ret, "clSetKernelArg");

									const size_t itemColor[2] = {10, 10};
//
									 ret = clEnqueueNDRangeKernel(command_queue, color_kernel, 2, NULL, itemColor, NULL, 0, NULL, NULL);
									 chk(ret, "clEnqueueNDRange");
//									 ret = clWaitForEvents(1, &ev_Colorconv);
//									 prof_Colorconv += checkTime(ev_Colorconv);

//
////				                     //Execute kernel on the device
////								 ret = clEnqueueTask(command_queue, color_kernel, 0, NULL, NULL);

								//Copy result to the host
									ret = clEnqueueReadBuffer(command_queue, RGB_GPU, CL_TRUE, 0, (MCU_sx * MCU_sy * max_ss_h * max_ss_v * sizeof(cl_int)), RGB_testing, 0, NULL, NULL);
									chk(ret, "clEnqueueRead");



//									YCbCr_to_ARGB(YCbCr_MCU, RGB_MCU, max_ss_h, max_ss_v);
								} else
								{
									to_NB(YCbCr_MCU, RGB_MCU, max_ss_h, max_ss_v);
								}


								screen_cpyrect
									(index_Y * MCU_sy * max_ss_h,
									 index_X * MCU_sx * max_ss_v,
									 MCU_sy * max_ss_h,
									 MCU_sx * max_ss_v,
									 RGB_MCU);
							}

						}

						if (screen_refresh() == 1)
						{
							end_of_file = 1;
						}

						for (HT_index = 0; HT_index < 4; HT_index++)
						{
							free_huffman_tables(tables[HUFF_DC][HT_index]);
							free_huffman_tables(tables[HUFF_AC][HT_index]);
							tables[HUFF_DC][HT_index] = NULL;
							tables[HUFF_AC][HT_index] = NULL;
						}


						COPY_SECTION(&marker, 2);

						break;
					}

				case M_DQT:
					{
						int DQT_size = 0;
						IPRINTF("DQT marker found\r\n");

						COPY_SECTION(&DQT_section.length, 2);
						CPU_DATA_IS_BIGENDIAN(16, DQT_section.length);
						DQT_size += 2;

						while (DQT_size < DQT_section.length) {

					NEXT_TOKEN(DQT_section.pre_quant);
							DQT_size += 1;
							IPRINTF
								("Quantization precision is %s\r\n",
								 ((DQT_section.
								   pre_quant >> 4) & 0x0f) ? "16 bits" : "8 bits");

							QT_index = DQT_section.pre_quant & 0x0f;
							IPRINTF("Quantization table index is %d\r\n", QT_index);

							IPRINTF("Reading quantization table\r\n");
							COPY_SECTION(DQT_table[QT_index], 64);
							DQT_size += 64;

						}

						break;
					}

				case M_APP0:
					{
						IPRINTF("APP0 marker found\r\n");

						COPY_SECTION(&jfif_header, sizeof(jfif_header));
						CPU_DATA_IS_BIGENDIAN(16, jfif_header.length);
						CPU_DATA_IS_BIGENDIAN(16, jfif_header.xdensity);
						CPU_DATA_IS_BIGENDIAN(16, jfif_header.ydensity);

						if (jfif_header.identifier[0] != 'J'
								|| jfif_header.identifier[1] != 'F'
								|| jfif_header.identifier[2] != 'I'
								|| jfif_header.identifier[3] != 'F') {
							VPRINTF("Not a JFIF file\r\n");
						}

						break;
					}

				case M_COM:
					{
						IPRINTF("COM marker found\r\n");
						uint16_t length;
						char * comment;
						//COPY_SECTION(&length, 2);
						CPU_DATA_IS_BIGENDIAN(16, length);
						comment = (char *) malloc (length - 2);
						COPY_SECTION(comment, length - 2);
						IPRINTF("Comment found : %s\r\n", comment);
						free(comment);
						break;
					}

				default:
					{
						IPRINTF("Unsupported token: 0x%x\r\n", marker[1]);
						skip_segment(movie);
						break;
					}
			}
		}
		else
		{
			VPRINTF("Invalid marker, expected SMS (0xff), got 0x%lx (second byte is 0x%lx) \n", marker[0], marker[1]);
		}
	}

clean_end:
	for (int i = 0 ; i < 3 ; i++) {
		if (YCbCr_MCU[i] != NULL) {
			free(YCbCr_MCU[i]);
			YCbCr_MCU[i] = NULL;
		}
		if (YCbCr_MCU_ds[i] != NULL) {
			free(YCbCr_MCU_ds[i]);
			YCbCr_MCU_ds[i] = NULL;
		}
	}
	if (RGB_MCU != NULL) {
		free(RGB_MCU);
		RGB_MCU = NULL;
	}
	for (HT_index = 0; HT_index < 4; HT_index++) {
		free_huffman_tables(tables[HUFF_DC][HT_index]);
		free_huffman_tables(tables[HUFF_AC][HT_index]);
	}

	screen_exit();

//	printf("----------VIDEO HAS ENDED----------\n");
//
//
//
//	printf(" IQZZ: %10.5f [ms]\n", (prof_iqzz)/1000000.0);
//	printf("   IDCT: %10.5f [ms]\n", (prof_IDCT)/1000000.0);
//	printf("   Upsampling: %10.5f [ms]\n", (prof_upsample)/1000000.0);
//	printf("    YCbCr-to-RGB: %10.5f [ms]\n", (prof_Colorconv)/1000000.0);

	//clGetEventProfilingInfo(inv_dct, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &prof_start, NULL);
	//clGetEventProfilingInfo(inv_dct, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &prof_DCTend, NULL);

	/* Finalization */
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
//	ret = clReleaseMemObject(DCT_Input);
//	ret = clReleaseMemObject(DCT_Output);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	free(source_str);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <stdbool.h>
//#include <malloc.h>
//
//#include "jpeg.h"
//#include "utils.h"
//#include "screen.h"
//#include "errno.h"
//#include "define_common.h"
//#include "iqzz.h"
//#include "unpack_block.h"
//#include "idct.h"
//#include "conv.h"
//#include "upsampler.h"
//#include "huffman.h"
//#include "skip_segment.h"
//#include "SDL/SDL.h"
//
//static void usage(char *progname) {
//	printf("Usage : %s [options] <mjpeg_file>\n", progname);
//	printf("Options list :\n");
//	printf("\t-f <framerate> : set framerate for the movie\n");
//	printf("\t-h : display this help and exit\n");
//}
//
//int main(int argc, char *argv[])
//{
//	uint8_t marker[2];
//	uint8_t HT_type = 0;
//	uint8_t HT_index = 0;
//	uint8_t DQT_table[4][64];
//	uint8_t QT_index = 0;
//	uint16_t nb_MCU = 0, nb_MCU_X = 0, nb_MCU_Y = 0;
//	uint16_t max_ss_h = 0 , max_ss_v = 0;
//	uint8_t index = 0, index_X, index_Y, index_SOF;
//	uint8_t component_order[4];
//	int32_t MCU[64];
//	int32_t unZZ_MCU[64];
//	uint8_t *YCbCr_MCU[3] = { NULL, NULL, NULL};
//	uint8_t *YCbCr_MCU_ds[3] = { NULL, NULL, NULL};
//	uint32_t *RGB_MCU = NULL;
//	uint32_t YH = 0, YV = 0;
//	uint32_t CrH = 0, CrV = 0, CbH = 0, CbV = 0;
//	uint32_t screen_init_needed;
//	uint32_t end_of_file;
//	int chroma_ss;
//	int args;
//	uint32_t framerate = 0;
//	uint32_t color = 1;
//
//	FILE *movie = NULL;
//	jfif_header_t jfif_header;
//	DQT_section_t DQT_section;
//	SOF_section_t SOF_section;
//	SOF_component_t SOF_component[3];
//	DHT_section_t DHT_section;
//	SOS_section_t SOS_section;
//	SOS_component_t SOS_component[3];
//
//	scan_desc_t scan_desc = { 0, 0, {}, {} };
//	huff_table_t *tables[2][4] = {
//		{NULL , NULL, NULL, NULL} ,
//		{NULL , NULL, NULL, NULL}
//	};
//
//	screen_init_needed = 1;
//	if (argc < 2) {
//		usage(argv[0]);
//		exit(1);
//	}
//	args = 1;
//
//	while (args < argc) {
//		if (argv[args][0] ==  '-') {
//			switch(argv[args][1]) {
//				case 'f':
//					if ((argc - args) < 1) {
//						usage(argv[0]);
//						exit(1);
//					}
//					framerate = atoi(argv[args+1]);
//					args +=2;
//					break;
//				case 'h':
//					usage(argv[0]);
//					exit(0);
//					break;
//				case 'c':
//					color=0;
//					args++;
//					break;
//				default :
//					usage(argv[0]);
//					exit(1);
//					break;
//			}
//		} else {
//			break;
//		}
//	}
//
//	if ((argc - args) != 1) {
//		usage(argv[0]);
//		exit(1);
//	}
//
//	if ((movie = fopen(argv[args], "r")) == NULL) {
//		perror(strerror(errno));
//		exit(-1);
//	}
//
//	/*---- Actual computation ----*/
//	end_of_file = 0;
//	while (!end_of_file ) {
//		int elem_read = fread(&marker, 2, 1, movie);
//		if (elem_read != 1) {
//			if (feof(movie)) {
//				end_of_file = 1;
//				break;
//			} else {
//				fprintf(stderr, "Error reading marker from input file\n");
//				exit (1);
//			}
//		}
//
//		if (marker[0] == M_SMS) {
//			switch (marker[1]) {
//				case M_SOF0:
//					{
//						IPRINTF("SOF0 marker found\r\n");
//
//						COPY_SECTION(&SOF_section, sizeof(SOF_section));
//						CPU_DATA_IS_BIGENDIAN(16, SOF_section.length);
//						CPU_DATA_IS_BIGENDIAN(16, SOF_section.height);
//						CPU_DATA_IS_BIGENDIAN(16, SOF_section.width);
//
//						VPRINTF("Data precision = %d\r\n",
//								SOF_section.data_precision);
//						VPRINTF("Image height = %d\r\n",
//								SOF_section.height);
//						VPRINTF("Image width = %d\r\n",
//								SOF_section.width);
//						VPRINTF("%d component%c\r\n",
//								SOF_section.n,
//								(SOF_section.n > 1) ? 's' : ' ');
//
//						COPY_SECTION(&SOF_component,
//								sizeof(SOF_component_t)* SOF_section.n);
//
//						YV = SOF_component[0].HV & 0x0f;
//						YH = (SOF_component[0].HV >> 4) & 0x0f;
//
//						for (index = 0 ; index < SOF_section.n ; index++) {
//							if (YV > max_ss_v) {
//								max_ss_v = YV;
//							}
//							if (YH > max_ss_h) {
//								max_ss_h = YH;
//							}
//						}
//
//						if (SOF_section.n > 1) {
//							CrV = SOF_component[1].HV & 0x0f;
//							CrH = (SOF_component[1].HV >> 4) & 0x0f;
//							CbV = SOF_component[2].HV & 0x0f;
//							CbH = (SOF_component[2].HV >> 4) & 0x0f;
//						}
//						IPRINTF("Subsampling factor = %ux%u, %ux%u, %ux%u\r\n",
//								YH, YV, CrH, CrV, CbH, CbV);
//
//						if (screen_init_needed == 1) {
//							screen_init_needed = 0;
//							screen_init(SOF_section.width, SOF_section.height, framerate);
//
//							nb_MCU_X = intceil(SOF_section.height, MCU_sx * max_ss_v);
//							nb_MCU_Y = intceil(SOF_section.width, MCU_sy * max_ss_h);
//							nb_MCU = nb_MCU_X * nb_MCU_Y;
//							for (index = 0 ; index < SOF_section.n ; index++) {
//								YCbCr_MCU[index] = malloc(MCU_sx * MCU_sy * max_ss_h * max_ss_v);
//								YCbCr_MCU_ds[index] = malloc(MCU_sx * MCU_sy * max_ss_h * max_ss_v);
//							}
//							RGB_MCU = malloc (MCU_sx * MCU_sy * max_ss_h * max_ss_v * sizeof(int32_t));
//						}
//
//						break;
//					}
//
//				case M_DHT:
//					{
//						// Depending on how the JPEG is encoded, DHT marker may not be
//						// repeated for each DHT. We need to take care of the general
//						// state where all the tables are stored sequentially
//						// DHT size represent the currently read data... it starts as a
//						// zero value
//						volatile int DHT_size = 0;
//						IPRINTF("DHT marker found\r\n");
//
//						COPY_SECTION(&DHT_section.length, 2);
//						CPU_DATA_IS_BIGENDIAN(16, DHT_section.length);
//						// We've read the size : DHT_size is now 2
//						DHT_size += 2;
//
//						//We loop while we've not read all the data of DHT section
//						while (DHT_size < DHT_section.length) {
//
//							int loaded_size = 0;
//							// read huffman info, DHT size ++
//							NEXT_TOKEN(DHT_section.huff_info);
//							DHT_size++;
//
//							// load the current huffman table
//							HT_index = DHT_section.huff_info & 0x0f;
//							HT_type = (DHT_section.huff_info >> 4) & 0x01;
//
//							IPRINTF("Huffman table index is %d\r\n", HT_index);
//							IPRINTF("Huffman table type is %s\r\n",
//									HT_type ? "AC" : "DC");
//
//							VPRINTF("Loading Huffman table\r\n");
//							tables[HT_type][HT_index] = (huff_table_t *) malloc(sizeof(huff_table_t));
//							loaded_size = load_huffman_table_size(movie,
//									DHT_section.length,
//									DHT_section.huff_info,
//									tables[HT_type][HT_index]);
//							if (loaded_size < 0) {
//								VPRINTF("Failed to load Huffman table\n");
//
//								goto clean_end;
//							}
//							DHT_size += loaded_size;
//
//							IPRINTF("Huffman table length is %d, read %d\r\n", DHT_section.length, DHT_size);
//						}
//
//						break;
//					}
//
//				case M_SOI:
//					{
//						IPRINTF("SOI marker found\r\n");
//						break;
//					}
//
//				case M_EOI:
//					{
//						IPRINTF("EOI marker found\r\n");
//						end_of_file = 1;
//						break;
//					}
//
//				case M_SOS:
//					{
//						IPRINTF("SOS marker found\r\n");
//
//						COPY_SECTION(&SOS_section, sizeof(SOS_section));
//						CPU_DATA_IS_BIGENDIAN(16, SOS_section.length);
//
//						COPY_SECTION(&SOS_component,
//								sizeof(SOS_component_t) * SOS_section.n);
//						IPRINTF("Scan with %d components\r\n", SOS_section.n);
//
//						/* On ignore les champs Ss, Se, Ah, Al */
//						SKIP(3);
//
//						scan_desc.bit_count = 0;
//						for (index = 0; index < SOS_section.n; index++) {
//							/*index de SOS correspond à l'ordre d'apparition des MCU,
//							 *  mais pas forcement égal à celui de déclarations des labels*/
//
//							for(index_SOF=0 ; index_SOF < SOF_section.n; index_SOF++) {
//								if(SOF_component[index_SOF].index==SOS_component[index].index){
//									component_order[index]=index_SOF;
//									break;
//								}
//								if(index_SOF==SOF_section.n-1)
//									VPRINTF("Invalid component label in SOS section");
//							}
//
//
//							scan_desc.pred[index] = 0;
//							scan_desc.table[HUFF_AC][index] =
//								tables[HUFF_AC][(SOS_component[index].acdc >> 4) & 0x0f];
//							scan_desc.table[HUFF_DC][index] =
//								tables[HUFF_DC][SOS_component[index].acdc & 0x0f];
//						}
//
//						uint32_t component_index;
//						//avoiding unneeded computation
//						int nb_MCU;
//
//
//						for (index_X = 0; index_X < nb_MCU_X; index_X++) {
//							for (index_Y = 0; index_Y < nb_MCU_Y; index_Y++) {
//								for (index = 0; index < SOS_section.n; index++) {
//									component_index = component_order[index];
//									//avoiding unneeded computation
//									nb_MCU = ((SOF_component[component_index].HV>> 4) & 0xf) * (SOF_component[component_index].HV & 0x0f);
//
//									for (chroma_ss = 0; chroma_ss < nb_MCU; chroma_ss++) {
//										unpack_block(movie, &scan_desc,index, MCU);
//										iqzz_block(MCU, unZZ_MCU,DQT_table[SOF_component[component_index].q_table]);
//									    IDCT(unZZ_MCU, YCbCr_MCU_ds[component_index] + (64 * chroma_ss));
//									}
//									upsampler(YCbCr_MCU_ds[component_index], YCbCr_MCU[component_index],max_ss_h / ((SOF_component[component_index].HV >> 4) & 0xf),max_ss_v / ((SOF_component[component_index].HV) & 0xf), max_ss_h, max_ss_v);
//									}
//								if(color&&(SOF_section.n>1))
//														{
//															YCbCr_to_ARGB(YCbCr_MCU, RGB_MCU, max_ss_h, max_ss_v);
//														}
//														else
//														{
//															to_NB(YCbCr_MCU, RGB_MCU, max_ss_h, max_ss_v);
//														}
//
//														screen_cpyrect(index_Y * MCU_sy * max_ss_h, index_X * MCU_sx * max_ss_v, MCU_sy * max_ss_h, MCU_sx * max_ss_v, RGB_MCU);
//
//								}
//							}
//
//
//
//
////						for (index_X = 0; index_X < nb_MCU_X; index_X++) {
////							for (index_Y = 0; index_Y < nb_MCU_Y; index_Y++) {
////							for (index = 0; index < SOS_section.n; index++) {
////
////							component_index = component_order[index];
////															//avoiding unneeded computation
////						nb_MCU = ((SOF_component[component_index].HV>> 4) & 0xf) * (SOF_component[component_index].HV & 0x0f);
//
////									upsampler(YCbCr_MCU_ds[component_index], YCbCr_MCU[component_index],max_ss_h / ((SOF_component[component_index].HV >> 4) & 0xf),max_ss_v / ((SOF_component[component_index].HV) & 0xf), max_ss_h, max_ss_v);
////							}
////							}
////								}
//
////						for (index_X = 0; index_X < nb_MCU_X; index_X++) {
////							for (index_Y = 0; index_Y < nb_MCU_Y; index_Y++) {
//
////								if(color&&(SOF_section.n>1))
////								{
////									YCbCr_to_ARGB(YCbCr_MCU, RGB_MCU, max_ss_h, max_ss_v);
////								}
////								else
////								{
////									to_NB(YCbCr_MCU, RGB_MCU, max_ss_h, max_ss_v);
////								}
////
////								screen_cpyrect(index_Y * MCU_sy * max_ss_h, index_X * MCU_sx * max_ss_v, MCU_sy * max_ss_h, MCU_sx * max_ss_v, RGB_MCU);
////							}
////						}
//
//
//
//						if (screen_refresh() == 1)
//						{
//							end_of_file = 1;
//						}
//
//						for (HT_index = 0; HT_index < 4; HT_index++)
//						{
//							free_huffman_tables(tables[HUFF_DC][HT_index]);
//							free_huffman_tables(tables[HUFF_AC][HT_index]);
//							tables[HUFF_DC][HT_index] = NULL;
//							tables[HUFF_AC][HT_index] = NULL;
//						}
//
//						COPY_SECTION(&marker, 2);
//
//						break;
//					}
//
//				case M_DQT:
//					{
//						int DQT_size = 0;
//						IPRINTF("DQT marker found\r\n");
//
//						COPY_SECTION(&DQT_section.length, 2);
//						CPU_DATA_IS_BIGENDIAN(16, DQT_section.length);
//						DQT_size += 2;
//
//						while (DQT_size < DQT_section.length) {
//
//							NEXT_TOKEN(DQT_section.pre_quant);
//							DQT_size += 1;
//							IPRINTF
//								("Quantization precision is %s\r\n",
//								 ((DQT_section.
//								   pre_quant >> 4) & 0x0f) ? "16 bits" : "8 bits");
//
//							QT_index = DQT_section.pre_quant & 0x0f;
//							IPRINTF("Quantization table index is %d\r\n", QT_index);
//
//							IPRINTF("Reading quantization table\r\n");
//							COPY_SECTION(DQT_table[QT_index], 64);
//							DQT_size += 64;
//
//						}
//
//						break;
//					}
//
//				case M_APP0:
//					{
//						IPRINTF("APP0 marker found\r\n");
//
//						COPY_SECTION(&jfif_header, sizeof(jfif_header));
//						CPU_DATA_IS_BIGENDIAN(16, jfif_header.length);
//						CPU_DATA_IS_BIGENDIAN(16, jfif_header.xdensity);
//						CPU_DATA_IS_BIGENDIAN(16, jfif_header.ydensity);
//
//						if (jfif_header.identifier[0] != 'J'
//								|| jfif_header.identifier[1] != 'F'
//								|| jfif_header.identifier[2] != 'I'
//								|| jfif_header.identifier[3] != 'F') {
//							VPRINTF("Not a JFIF file\r\n");
//						}
//
//						break;
//					}
//
//				case M_COM:
//					{
//						IPRINTF("COM marker found\r\n");
//						uint16_t length;
//						char * comment;
//						COPY_SECTION(&length, 2);
//						CPU_DATA_IS_BIGENDIAN(16, length);
//						comment = (char *) malloc (length - 2);
//						COPY_SECTION(comment, length - 2);
//						IPRINTF("Comment found : %s\r\n", comment);
//						free(comment);
//						break;
//					}
//
//
//
//
//
//				default:
//					{
//						IPRINTF("Unsupported token: 0x%x\r\n", marker[1]);
//						skip_segment(movie);
//						break;
//					}
//			}
//		} else {
//			VPRINTF("Invalid marker, expected SMS (0xff), got 0x%lx (second byte is 0x%lx) \n", marker[0], marker[1]);
//		}
//	}
//
//clean_end:
//	for (int i = 0 ; i < 3 ; i++) {
//		if (YCbCr_MCU[i] != NULL) {
//			free(YCbCr_MCU[i]);
//			YCbCr_MCU[i] = NULL;
//		}
//		if (YCbCr_MCU_ds[i] != NULL) {
//			free(YCbCr_MCU_ds[i]);
//			YCbCr_MCU_ds[i] = NULL;
//		}
//	}
//	if (RGB_MCU != NULL) {
//		free(RGB_MCU);
//		RGB_MCU = NULL;
//	}
//	for (HT_index = 0; HT_index < 4; HT_index++) {
//		free_huffman_tables(tables[HUFF_DC][HT_index]);
//		free_huffman_tables(tables[HUFF_AC][HT_index]);
//	}
//
//	screen_exit();
//	return 0;
//}
