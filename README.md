# Runtime Driver

This repo contains [AWS-FPGA](https://github.com/aws/aws-fpga) compatible Runtime C Drivers that will drive the AWS-FPGA CL Designs to interract with the Cloud FPGA via AFI.

# How to Run

1. First Copy the Driver you want to run. (Considering the AFI is loaded)
2. Paste it in your design's `software/runtime` directory.
3. Open Terminal in this directory.
4. First type `make driverName` (for example if the driver is bramLoader.c then type `make bramLoader`)
5. Then once the driver is compiled successfully type `sudo ./driverName` (for example if driver if bramLoader.c then type `sudo ./bramLoader`)
6. All the interactions that the driver has done with the FPGA will be shown on the terminal screen

## Drivers
This repo contains the following drivers

| Driver | Purpose |
| ------------- | ------------- |
| BRAM Loader | Reads a hex file and stores the data into BRAM (WORD aligned) |
