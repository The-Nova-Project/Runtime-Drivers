# Runtime Driver

This repo contains [AWS-FPGA](https://github.com/aws/aws-fpga) compatible Runtime C Drivers that will drive the AWS-FPGA CL Designs to interract with the Cloud FPGA via AFI.

# How to Run

1. First Copy the Driver you want to run. (Considering the AFI is loaded)
2. Paste it in your design's `software/runtime` directory.
3. Open Terminal in this directory.
4. First type `make driverName` (for example if the driver is loader.c then type `make loader`)
5. Then once the driver is compiled successfully type `sudo ./driverName` (for example if driver if bramLoader.c then type `sudo ./loader *args`)
6. All the interactions that the driver has done with the FPGA will be shown on the terminal screen

## Drivers
This repo contains the following drivers

| Driver | Format | Purpose |
| ------------- | ------------- | ------------- |
| Loader | ./loader &lt;hex-file&gt;/&lt;elf-file&gt; dma/bram | Reads a hex/elf file and stores the data into BRAM/DMA (WORD aligned) |
| UART   | ./uart_runtime | Transmit and Receive data through UART via OCL Interface |
