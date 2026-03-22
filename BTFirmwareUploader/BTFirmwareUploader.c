//
//  BTFirmwareUploader.c
//  BTFirmwareUploader
//
//  Created by EMlyDinEsH on 8/15/16.
//  Copyright © 2016 EMlyDinEsH. All rights reserved.
//

#include <mach/mach_types.h>

kern_return_t BTFirmwareUploader_start(kmod_info_t * ki, void *d);
kern_return_t BTFirmwareUploader_stop(kmod_info_t *ki, void *d);

kern_return_t BTFirmwareUploader_start(kmod_info_t * ki, void *d)
{
    return KERN_SUCCESS;
}

kern_return_t BTFirmwareUploader_stop(kmod_info_t *ki, void *d)
{
    return KERN_SUCCESS;
}
