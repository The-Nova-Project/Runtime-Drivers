// Amazon FPGA Hardware Development Kit
//
// Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Amazon Software License (the "License"). You may not use
// this file except in compliance with the License. A copy of the License is
// located at
//
//    http://aws.amazon.com/asl/
//
// or in the "license" file accompanying this file. This file is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
// implied. See the License for the specific language governing permissions and
// limitations under the License.


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>


#include <fpga_pci.h>
#include <fpga_mgmt.h>
#include <utils/lcd.h>

#include <utils/sh_dpi_tasks.h>


// #define BRAM_STRT_ADDRESS UINT32_C(0x0)
#define TOCORE_ADDRESS UINT32_C(0x00000120)

const struct logger *logger = &logger_stdout;
static uint16_t pci_vendor_id = 0x1D0F; /* Amazon PCI Vendor ID */
static uint16_t pci_device_id = 0xF000; /* PCI Device ID preassigned by Amazon for F1 applications */

int check_afi_ready(int slot_id);

void usage(char* program_name) {
    printf("usage: %s [--slot <slot-id>][<poke-value>]\n", program_name);
}

uint32_t byte_swap(uint32_t value);


uint32_t byte_swap(uint32_t value) {
    uint32_t swapped_value = 0;
    int b;
    for (b = 0; b < 4; b++) {
        swapped_value |= ((value >> (b * 8)) & 0xff) << (8 * (3-b));
    }
    return swapped_value;
}
int peek_poke_example(int total_values, uint32_t values [], int slot_id, int pf_id, int bar_id);


int main(int argc, char **argv)
{

    uint32_t BRAM_STRT_ADDRESS = 0x0;

    printf("\n ------ ---- --- --- -- - -- TURNINGN DIP SWITCH / HYDRA RESET ON  ---- --- -- -- - -- - - - --- - \n");

    system("fpga-set-virtual-dip-switch -S 0 -D 0000000000000000");
    #ifdef SCOPE
      svScope scope;
      scope = svGetScopeFromName("tb");
      svSetScope(scope);
    #endif

    int total_instructions = 75;
    uint32_t final_hex[75]= {

0x0500006f,
0x340c1073,
0x00000c17,
0x108c0c13,
0x019c3023,
0x02000c37,
0x000c3023,
0x34202cf3,
0x00000c17,
0x0e8c0c13,
0x019c3023,
0x00000c17,
0x0ecc0c13,
0x00100c93,
0x019c3023,
0x00000c17,
0x0d4c0c13,
0x000c3c83,
0x34002c73,
0x30200073,
0x00100093,
0x00200113,
0x00300193,
0xf14028f3,
0x09101c63,
0xfff6e837,
0x5d58081b,
0x00c81813,
0xc3b80813,
0x00d81813,
0x54380813,
0x00c81813,
0x21080813,
0x34081073,
0x340028f3,
0x07181663,
0x00000817,
0xf7480813,
0x30581073,
0x30447073,
0x30047073,
0x00000a17,
0x074a0a13,
0x000a3023,
0x02000a37,
0x00100a93,
0x015a3023,
0x30446073,
0x30046073,
0x00000a17,
0x054a0a13,
0x000a3a83,
0xfe0a8ee3,
0x00000a17,
0x034a0a13,
0x000a3a83,
0x00100b13,
0x03fb1b13,
0x003b0b13,
0x015b1663,
0x00000f93,
0x0080006f,
0x00100f93,
0x00000c17,
0x024c0c13,
0x01fc2023,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000

};
    int slot_id = 0;
    int rc;

    /* initialize the fpga_mgmt library */
    rc = fpga_mgmt_init();
    fail_on(rc, out, "Unable to initialize the fpga_mgmt library");

#ifndef SV_TEST
    rc = check_afi_ready(slot_id);
    fail_on(rc, out, "AFI not ready");
#endif

   
    /* Accessing the CL registers via AppPF BAR0, which maps to sh_cl_ocl_ AXI-Lite bus between AWS FPGA Shell and the CL*/

    printf("===== Starting with writing in BAR 01 =====\n");


    rc = peek_poke_example( total_instructions, final_hex, slot_id, FPGA_APP_PF, APP_PF_BAR1);
    fail_on(rc, out, "peek-poke example failed");
   


    return rc;
   
out:
    return 1;

}


int check_afi_ready(int slot_id) {
   struct fpga_mgmt_image_info info = {0};
   int rc;

   /* get local image description, contains status, vendor id, and device id. */
   rc = fpga_mgmt_describe_local_image(slot_id, &info,0);
   fail_on(rc, out, "Unable to get AFI information from slot %d. Are you running as root?",slot_id);

   /* check to see if the slot is ready */
   if (info.status != FPGA_STATUS_LOADED) {
     rc = 1;
     fail_on(rc, out, "AFI in Slot %d is not in READY state !", slot_id);
   }

   printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
          info.spec.map[FPGA_APP_PF].vendor_id,
          info.spec.map[FPGA_APP_PF].device_id);

   /* confirm that the AFI that we expect is in fact loaded */
   if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
       info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
     printf("AFI does not show expected PCI vendor id and device ID. If the AFI "
            "was just loaded, it might need a rescan. Rescanning now.\n");

     rc = fpga_pci_rescan_slot_app_pfs(slot_id);
     fail_on(rc, out, "Unable to update PF for slot %d",slot_id);
     /* get local image description, contains status, vendor id, and device id. */
     rc = fpga_mgmt_describe_local_image(slot_id, &info,0);
     fail_on(rc, out, "Unable to get AFI information from slot %d",slot_id);

     printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
            info.spec.map[FPGA_APP_PF].vendor_id,
            info.spec.map[FPGA_APP_PF].device_id);

     /* confirm that the AFI that we expect is in fact loaded after rescan */
     if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
         info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
       rc = 1;
       fail_on(rc, out, "The PCI vendor id and device of the loaded AFI are not "
               "the expected values.");
     }
   }
   
   return rc;
 out:
   return 1;
}



/*
 * An example to attach to an arbitrary slot, pf, and bar with register access.
 */
int peek_poke_example(int total_values, uint32_t values [], int slot_id, int pf_id, int bar_id) {
    int rc;
    /* pci_bar_handle_t is a handler for an address space exposed by one PCI BAR on one of the PCI PFs of the FPGA */
uint32_t BRAM_STRT_ADDRESS = 0X0;
    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;

    rc = fpga_pci_attach(slot_id, pf_id, bar_id, 0, &pci_bar_handle);
    fail_on(rc, out, "Unable to attach to the AFI on slot id %d", slot_id);
   
    /* write a value into the mapped address space */
    // uint32_t expected = value;
    printf("WRITING INRO BRAM BEGINSSSS !!!!\n");
    int i=0;
    for(i=0; i<total_values;i++){
        uint32_t value = values[i];
        printf("Writing 0x%08x to BRAM", value);
        printf("ON ADDRESS 0x%08x", BRAM_STRT_ADDRESS);
        rc = fpga_pci_poke(pci_bar_handle, BRAM_STRT_ADDRESS, value);
        printf("\n");

        fail_on(rc, out, "Unable to write to the fpga !");

        BRAM_STRT_ADDRESS = BRAM_STRT_ADDRESS+4;
    }


    uint32_t BRAM_STRT_ADDRESS_r = 0x0;
    int ii=0;
    for(ii=0; ii<total_values;ii++){
        uint32_t expectedValue = values[ii];
        uint32_t value;
        rc = fpga_pci_peek(pci_bar_handle, BRAM_STRT_ADDRESS_r, &value);

        printf("READING FROM 0x%08x ", BRAM_STRT_ADDRESS_r);
        printf("VALUE 0x%08x ------", expectedValue);
        if(expectedValue == value){
            printf("PASSSED  - 0x%08x", value);
        } else {
            printf("FAILED  - 0x%08x", value);
        }
        printf("\n");
        

        fail_on(rc, out, "Unable to write to the fpga !");

        BRAM_STRT_ADDRESS_r = BRAM_STRT_ADDRESS_r+4;
    }
   

   printf("\n ------ WRITING DEADBEEF TO TO-HOST -------- /n");
   rc = fpga_pci_poke(pci_bar_handle, TOCORE_ADDRESS, 0xdeadbeef);
    fail_on(rc, out, "Unable to write to the fpga !");


    /* ------------------------------------------------- WRITE DONE ---------------------------------------------------------- */

    printf("\n ------ ---- --- --- -- - -- TURNINGN DIP SWITCH / HYDRA RESET OFF  ---- --- -- -- - -- - - - --- - \n");
    system("fpga-set-virtual-dip-switch -S 0 -D 0000000000000011");

    uint32_t pass_val = 0;
    uint32_t fail_val = 1;
    uint32_t theValue;


printf("\n ------------------- PEEKING THRU TO-CORE ------------------\n");
int cccc = 0;
    for(cccc=0;cccc<20;cccc++){
        sleep(3);

        rc = fpga_pci_peek(pci_bar_handle, TOCORE_ADDRESS, &theValue);
        fail_on(rc, out, "Unable to read read from the fpga !");
        printf("=====  Entering peek_poke_example =====\n");
        printf("peeked value: 0x%x\n", theValue);
        if(theValue == pass_val) {
            printf("TEST PASSED");
            printf("Resulting value matched expected value 0x%x. It worked!\n", theValue);
            break;
        }
        if(theValue == fail_val) {
            printf("TEST FAILED");
            printf("Resulting value matched expected value 0x%x. It worked!\n", theValue);
        }
    }
    // while (theValue != pass_val && theValue != fail_val);

out:
    /* clean up */
    if (pci_bar_handle >= 0) {
        rc = fpga_pci_detach(pci_bar_handle);
        if (rc) {
            printf("Failure while detaching from the fpga.\n");
        }
    }

    /* if there is an error code, exit with status 1 */
    return (rc != 0 ? 1 : 0);
}