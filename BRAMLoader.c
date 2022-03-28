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


#define BRAM_STRT_ADDRESS UINT32_C(0x0)
#define TOCORE_ADDRESS UINT32_C(0x0000000080000120)

const struct logger *logger = &logger_stdout;
static uint16_t pci_vendor_id = 0x1D0F; /* Amazon PCI Vendor ID */
static uint16_t pci_device_id = 0xF000; /* PCI Device ID preassigned by Amazon for F1 applications */

int check_afi_ready(int slot_id);

void usage(char* program_name) {
    printf("usage: %s [--slot <slot-id>][<poke-value>]\n", program_name);
}

uint32_t byte_swap(uint32_t value);

int peek_poke_example(uint32_t value, int slot_id, int pf_id, int bar_id);

uint32_t byte_swap(uint32_t value) {
    uint32_t swapped_value = 0;
    int b;
    for (b = 0; b < 4; b++) {
        swapped_value |= ((value >> (b * 8)) & 0xff) << (8 * (3-b));
    }
    return swapped_value;
}

int main(int argc, char **argv)
{

    system("fpga-set-virtual-dip-switch -S 0 -D 1111111111111111");
    #ifdef SCOPE
      svScope scope;
      scope = svGetScopeFromName("tb");
      svSetScope(scope);
    #endif

    int total_instructions = 6;
    uint32_t final_hex[total_instructions]= {

0x400027B7,
0x00078793,
0x00A00413,
0x01900493,
0x0087A423,
0x0097A223,

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

    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;

    rc = fpga_pci_attach(slot_id, pf_id, bar_id, 0, &pci_bar_handle);
    fail_on(rc, out, "Unable to attach to the AFI on slot id %d", slot_id);
    
    /* write a value into the mapped address space */
    uint32_t expected = value;
    printf("WRITING INRO BRAM BEGINSSSS !!!!");

    for(int i=0; i<total_values;i++){
        uint32_t value = values[i];
        printf("Writing 0x%08x to BRAM\n", value);
        rc = fpga_pci_poke(pci_bar_handle, BRAM_STRT_ADDRESS, value);

        fail_on(rc, out, "Unable to write to the fpga !");
    }
    


    /* ------------------------------------------------- WRITE DONE ---------------------------------------------------------- */

    system("fpga-set-virtual-dip-switch -S 0 -D 0000000000000000");

    uint32_t pass_val = 0;
    uint32_t fail_val = 1;
    uint32_t theValue;

    do {
        sleep(3);

        rc = fpga_pci_peek(pci_bar_handle, TOCORE_ADDRESS, &theValue);
        fail_on(rc, out, "Unable to read read from the fpga !");
        printf("=====  Entering peek_poke_example =====\n");
        printf("register: 0x%x\n", theValue);
        if(value == pass_val) {
            printf("TEST PASSED");
            printf("Resulting value matched expected value 0x%x. It worked!\n", theValue);
        }
        if(value == fail_val) {
            printf("TEST PASSED");
            printf("Resulting value matched expected value 0x%x. It worked!\n", theValue);
        }
    }
    while (theValue == pass_val || theValue == fail_val);

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

#ifdef SV_TEST
/*This function is used transfer string buffer from SV to C.
  This function currently returns 0 but can be used to update a buffer on the 'C' side.*/
int send_rdbuf_to_c(char* rd_buf)
{
   return 0;
}

#endif