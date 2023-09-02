#include <stdlib.h> 
#include <stdio.h> 
#include "mmio.h"
#define ACCEL_CONTROL 0x20000
#define AP_DONE_MASK 0b10

#define ACCEL_INT 0x20004
#define ACCEL_A 0x20010
#define ACCEL_B 0x20018

#define MAX_FILE_SIZE 663554

void writefile(char *name, char* buffer)
{
	FILE *file;
	//char *buffer;
	unsigned long fileLen =  MAX_FILE_SIZE;

	//Open file
	file = fopen(name, "wb");
	if (!file)
	{
		fprintf(stderr, "Unable to open file %s", name);
		return;
	}
	
	//Get file length

	//Allocate memory
	/*buffer=(char *)malloc(fileLen+1);
	if (!buffer)
	{
		fprintf(stderr, "Memory error!");
    fclose(file);
		return;
	}*/

	//Read file contents into buffer
  fwrite(buffer, fileLen, 1, file);
	fclose(file);
}
void readfile(char *name, char* buffer)
{
	FILE *file;
	//char *buffer;
	unsigned long fileLen;

	//Open file
	file = fopen(name, "rb");
	if (!file)
	{
		fprintf(stderr, "Unable to open file %s", name);
		return;
	}
	
	//Get file length
	fseek(file, 0, SEEK_END);
	fileLen=ftell(file);
	fseek(file, 0, SEEK_SET);
  printf("File Size %d Byte\n", fileLen);

	//Allocate memory
	/*buffer=(char *)malloc(fileLen+1);
	if (!buffer)
	{
		fprintf(stderr, "Memory error!");
    fclose(file);
		return;
	}*/

	//Read file contents into buffer
  fread(buffer, fileLen, 1, file);
	fclose(file);
}

int gemx_accel(char* a, char* b){
    // Disable interrupt for now
    reg_write32(ACCEL_INT, 0x0);

    // Set up pointer a and pointer b address
    reg_write32(ACCEL_A, (uint32_t)a);
    reg_write32(ACCEL_B, (uint32_t)b);

    // Write to ap_start to start the execution 
    reg_write32(ACCEL_CONTROL, 0x1);
    //printf("Accel Control: %x\n", reg_read32(ACCEL_CONTROL));

    // Done?
    int done = 0;
    while (!done){
        done = reg_read32(ACCEL_CONTROL) & AP_DONE_MASK;
    }
    return 0;
}




int main (){

	char in_buffer[MAX_FILE_SIZE];
	char out_buffer[MAX_FILE_SIZE];
	//char *buffer;
  char * in_path = "/scratch/qijing.huang/firesim/target-design/firechip/hls_gemx_tl_gemx/src/main/c/app/app.bin";
  char * out_path = "/scratch/qijing.huang/firesim/target-design/firechip/hls_gemx_tl_gemx/src/main/c/app/app_out.bin";
	readfile(in_path, in_buffer);
  
  printf("%x\n", in_buffer);

  gemx_accel(in_buffer, out_buffer);
  writefile(out_path, out_buffer);
  //printf("%s\n", in_buffer);
	//free(buffer);
	return 0;
}
