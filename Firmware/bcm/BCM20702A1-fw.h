//
//  BCM20702A1-fw.h 
//  BTFirmwareUploader
//
//  Copyright (c) 2013-2016 EMlyDinEsHMG. All rights reserved.
//

#ifndef BTFirmwareUploader_BCM20702A1_fw_h
#define BTFirmwareUploader_BCM20702A1_fw_h


//
// Declaring a char array with the size of a firmware which has large value (lenovo 43602)
// and this buffer is used to load the user chosen firmware dynamically.
// We can't declare this inside firmware upload method, doing so will give kernel panic.
//

char bcm_fw_buf[109062] = {NULL};


#endif
