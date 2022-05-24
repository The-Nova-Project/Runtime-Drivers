/*
 * Amazon FPGA Hardware Development Kit
 *
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Amazon Software License (the "License"). You may not use
 * this file except in compliance with the License. A copy of the License is
 * located at
 *
 *    http://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
 * implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include "fpga_pci.h"
#include "fpga_mgmt.h"
#include "fpga_dma.h"
#include "utils/lcd.h"


#define	MEM_16G              (1ULL << 34)
#define USER_INTERRUPTS_MAX  (16)
#define SU_RESET_OFF            0x02
#define SU_RESET_ON             0x03
#define TOTAL_INSTR             75
#define BUFFER_SIZE             32
#define DDR_START_ADDR          0x0

/* use the standard out logger */
static const struct logger *logger = &logger_stdout;

void usage(const char* program_name);
int dma_example(int slot_id, size_t buffer_size);

// utility func for loading instruction from hex file
void instrLoader(uint32_t hex_arr [], int inst_no){
    FILE *fptr = fopen("hex.txt", "r");
    // Assigning the instructions to array
    int i;
    for (i = 0; i < inst_no; ++i)
    {
        fscanf(fptr, "%X", &hex_arr[i]);
    }
    // Closing the file
    fclose(fptr);
}


int main(int argc, char **argv) {
    int rc;
    int slot_id = 0;
    int interrupt_n;

    switch (argc) {
    case 1:
        break;
    case 3:
        sscanf(argv[2], "%x", &slot_id);
        break;
    default:
        usage(argv[0]);
        return 1;
    }
    uint16_t  dip_sw_val   = 0U;


    /* setup logging to print to stdout */
    rc = log_init("DDR Loader");
    fail_on(rc, out, "Unable to initialize the log.");
    rc = log_attach(logger, NULL, 0);
    fail_on(rc, out, "%s", "Unable to attach to the log.");

    /* initialize the fpga_plat library */
    rc = fpga_mgmt_init();
    fail_on(rc, out, "Unable to initialize the fpga_mgmt library");

    /* check that the AFI is loaded */
    log_info("Checking to see if the right AFI is loaded...");


    dip_sw_val             = SU_RESET_OFF;

    rc = fpga_mgmt_set_vDIP(slot_id, dip_sw_val);
    fail_on(rc, out, "FAIL TO WRITE VDIP1");

    rc = fpga_mgmt_get_vDIP_status(slot_id, &dip_sw_val);
    fail_on(rc, out, "FAIL TO READ VDIP1");
    printf("NEW VDIP VALUE: 0x%02x \n", dip_sw_val);

    /* run the dma test example */
    rc = dma_example(slot_id, BUFFER_SIZE);
    fail_on(rc, out, "DMA example failed");

    printf("\n ------ ---- --- --- -- - -- TURNINGN DIP SWITCH / HYDRA RESET OFF  ---- --- -- -- - -- - - - --- - \n");
    
    dip_sw_val |= SU_RESET_ON;
    rc = fpga_mgmt_set_vDIP(0,dip_sw_val);
    fail_on(rc, out, "FAILED TO WRITE VDIP 2");

    rc = fpga_mgmt_get_vDIP_status(0, &dip_sw_val);
    fail_on(rc, out, "FAIL TO GET VDIP SWITCH VAL");
    printf("VDIP VALUE: 0x%02x \n", dip_sw_val);
    
    // rc = fpga_mgmt_get_vLED_status(0, &led_val);
    // fail_on(rc, out, "FAIL TO GET LEDs");
    // printf("VLED VALUE: 0x%02x \n", led_val);

    

out:
    log_info("TEST %s", (rc == 0) ? "PASSED" : "FAILED");
    return rc;
}

void usage(const char* program_name) {
    printf("usage: %s [--slot <slot>]\n", program_name);
}

/**
 * This example fills a buffer with random data and then uses DMA to copy that
 * buffer into each of the 4 DDR DIMMS.
 */
int dma_example(int slot_id, size_t buffer_size) {
    int write_fd, read_fd, dimm, rc;

    write_fd = -1;
    read_fd = -1;

    // uint8_t *write_buffer = malloc(buffer_size);
    // uint8_t *read_buffer = malloc(buffer_size);
    // if (write_buffer == NULL || read_buffer == NULL) {
    //     rc = -ENOMEM;
    //     goto out;
    // }

    read_fd = fpga_dma_open_queue(FPGA_DMA_XDMA, slot_id,
        /*channel*/ 0, /*is_read*/ true);
    fail_on((rc = (read_fd < 0) ? -1 : 0), out, "unable to open read dma queue");

    write_fd = fpga_dma_open_queue(FPGA_DMA_XDMA, slot_id,
        /*channel*/ 0, /*is_read*/ false);
    fail_on((rc = (write_fd < 0) ? -1 : 0), out, "unable to open write dma queue");

    // rc = fill_buffer_urandom(write_buffer, buffer_size);
    // fail_on(rc, out, "unabled to initialize buffer");

    uint32_t instructions_arr[TOTAL_INSTR];
    instrLoader(&instructions_arr, TOTAL_INSTR);
    u_int32_t instruction;

    uint32_t address       = DDR_START_ADDR;
    int i;
    for(i=0; i<TOTAL_INSTR; i++){
        instruction = instructions_arr[i];
        rc = fpga_dma_burst_write(write_fd, instruction, buffer_size,
           address );
        fail_on(rc, out, "DMA write failed on DIMM: %d",address);
        address = address + 4;
    }

    // for (dimm = 0; dimm < 1; dimm++) {
    //     rc = fpga_dma_burst_write(write_fd, write_buffer, buffer_size,
    //         dimm * MEM_16G);
    //     fail_on(rc, out, "DMA write failed on DIMM: %d", dimm);
    // }
    uint32_t expectedInstruction = 0U;
    bool passed = true;
    address = DDR_START_ADDR;

    int j;
    for (j=0; j < TOTAL_INSTR; j++){

        expectedInstruction = instructions_arr[j];
        rc = fpga_dma_burst_read(read_fd, instruction, buffer_size,
            address);

        fail_on(rc, out, "Unable to read from the fpga !");
        printf("READING FROM 0x%08x ", address);
        printf("VALUE 0x%08x ------", expectedInstruction);
        
        if(expectedInstruction == instruction){
            printf("PASSSED  - 0x%08x", instruction);
        } else {
            printf("FAILED  - 0x%08x", instruction);
        }
        printf("\n");

        address = address + 4;
    }



    // for (dimm = 0; dimm < 1; dimm++) {
    //     rc = fpga_dma_burst_read(read_fd, read_buffer, buffer_size,
    //         dimm * MEM_16G);
    //     fail_on(rc, out, "DMA read failed on DIMM: %d", dimm);

    //     uint64_t differ = buffer_compare(read_buffer, write_buffer, buffer_size);
    //     if (differ != 0) {
    //         log_error("DIMM %d failed with %lu bytes which differ", dimm, differ);
    //         passed = false;
    //     } else {
    //         log_info("DIMM %d passed!", dimm);
    //     }
    // }
    rc = (passed) ? 0 : 1;

out:
    // if (write_buffer != NULL) {
    //     free(write_buffer);
    // }
    // if (read_buffer != NULL) {
    //     free(read_buffer);
    // }
    if (write_fd >= 0) {
        close(write_fd);
    }
    if (read_fd >= 0) {
        close(read_fd);
    }
    /* if there is an error code, exit with status 1 */
    return (rc != 0 ? 1 : 0);
}
