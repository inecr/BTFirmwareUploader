/*
 *
 * Copyright (C) 2012-2016 by EMlyDinEsHMG
 *
 * Bluetooth Firmware Uploader v3.1 for Mac OSX
 *
 * Patches some Atheros and Broadcom Bluetooth controller firmwares
 * which is needed for these Bluetooth controllers to identify itself as Bluetooth device and
 * to work with Apple Bluetooth drivers
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Trademarks are the property of their respective owners.
 *
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef __BTFirmwareUploader__
#define __BTFirmwareUploader__

#include <IOKit/IOLib.h>
#include <IOKit/IOMessage.h>

#include <IOKit/IOTimerEventSource.h>
#include <IOKit/usb/USBSpec.h>

#define TARGET_MAC_OSX_10_11 1

#if TARGET_MAC_OSX_10_11
#include <IOKit/usb/IOUSBHostDevice.h>
#include <IOKit/usb/IOUSBHostInterface.h>
#include <IOKit/usb/IOUSBHostPipe.h>
#else
#include <IOKit/usb/IOUSBDevice.h>
#include <IOKit/usb/IOUSBInterface.h>
#endif

#define USB_CNTRL_REQ_TIMEOUT   5000

/*
 * Atheros Bluetooth
 */

#define BULK_SIZE	1024
#define FW_HDR_SIZE 20

// AR3011
#define USB_REQ_DFU_DNLOAD      0x01
#define USB_REQ_GET_STATE       0x05
#define USB_FIRMWARE_RAM_MODE   0x0b
#define USB_FIRMWARE_FLASH_MODE 0x0c

#define BTUSB_ATH3011           "AR3011"

// AR3012
#define ATH3K_DNLOAD				0x01
#define ATH3K_GETSTATE				0x05
#define ATH3K_SET_NORMAL_MODE		0x07
#define ATH3K_GETVERSION			0x09
#define USB_REG_SWITCH_VID_PID		0x0a

#define ATH3K_MODE_MASK				0x3F
#define ATH3K_NORMAL_MODE			0x0E

#define ATH3K_PATCH_UPDATE			0x80
#define ATH3K_SYSCFG_UPDATE			0x40

#define ATH3K_XTAL_FREQ_26M			0x00
#define ATH3K_XTAL_FREQ_40M			0x01
#define ATH3K_XTAL_FREQ_19P2		0x02
#define ATH3K_NAME_LEN				0xFF

#define BTUSB_ATH3012		"AR3012"

struct ath3k_version {
    unsigned int	rom_version;
    unsigned int	build_version;
    unsigned int	ram_version;
    unsigned char	ref_clock;
    unsigned char	reserved[0x07];
};

/*
 * Broadcom Bluetooth
 */

#define FIRMWARE_VER            "Firmware"
#define DEVICE_SPEC_FW          "DeviceSpecificFw"
#define UPD_SYNC_DELAY          "UpdateSyncDelay"
#define ABORT_UPD_FAIL          "AbortOnFwUpdFail"

#define BTUSB_BCM_COMBO "BCMCOMBO"
#define BTUSB_BCM_STDAL "BCMSTDAL"

#define BCM_FW_PREFIX      ':' // 0x3A

/*
 * Standard HCI commands
 */

unsigned char  HCI_RESET[] = { 0x03, 0x0c, 0x00 };

/*
 * Broadcom vendor specific commands
 */

unsigned char  HCI_BCM_DOWNLOAD_MINI_DRIVER[] = { 0x2e, 0xfc, 0x00 };
unsigned char  HCI_BCM_HEX_TO_HCD_END[] = { 0x4e, 0xfc, 0x04, 0xff, 0xff, 0xff, 0xff };


class BTFirmwareUploader : public IOService
{
    OSDeclareDefaultStructors(BTFirmwareUploader)
    
protected:
#if APPLESDK
    AbsoluteTime    _now;
#else
    uint64_t    _now;
#endif
    
    IOTimerEventSource*         _readEventTimer;
    IOBufferMemoryDescriptor*   _readMemBufDesc;
    IOLock* _mDeviceLock;
    
#if TARGET_MAC_OSX_10_11
    IOUSBHostDevice*     _USBDev;
    IOUSBHostInterface*  _USBDevIntf;
    IOUSBHostPipe*       _USBOutPipe;
    IOUSBHostPipe*       _USBInPipe;
#else
    IOUSBDevice*    _USBDev;
    IOUSBInterface* _USBDevIntf;
    IOUSBPipe*      _USBOutPipe;
    IOUSBPipe*      _USBInPipe;
#endif
    
    UInt16  _wInPipe, _wOutPipe;
    UInt32  _productID, _vendorID;
    
    // Note: Declaring these buffers inside local scope are causing KP sometimes
    char    bcm_fw_buf[78656]; // Using the size of big firmware buffer
    char    _writeBuffer[BULK_SIZE];
    
    UInt8   _osxVersion;            // OS Version
    
    bool    _useCtrlPipe;           // Use control pipe for out?
    bool    _readComplete;          // BCM Read status
    bool    _isStandaloneBT;         // Standalone BT?
    
    bool    _isAtherosController;    // Atheros? if true else Broadcom
    bool    _isAR3012Bluetooth;      // AR3012 BT?
    
    bool    _useLinuxFw;             // Use linux fw?
    bool    _serviceClosed;          // kIOMessageServiceWasClosed ?
    bool    _abortOnFail;            // Abort next fw update if last upd failed ?

    UInt8   _ath3kWinFw;             // Windows fw number?
    UInt8   _updateSyncDelay;        // Update sync delay
    UInt32  _uploadLoopCounts;
    
private:
    
    bool ReadPlistConfig();
    
    // Methods related to Atheros bluetooth
    bool processAth3kFwUpdate();
    
    // AR3012
    bool ath3012PatchAndSysConfig(bool patchFw);
    bool ath3012SetNormalMode();
    
    // Methods related to Broadcom bluetooth
    void bcmProcessFirmwareUpdate();
    bool bcmParseFirmwareAndUpload();
    UInt32 bcmGetFirmwareBuffer();
    
    // Method to interact with USB device
    IOReturn    initializeUSBDevice();
    IOReturn    findInterface();
    IOReturn    findPipes();
    IOReturn    writeFirmwareToDevice(unsigned char* fwBuffer, UInt32 buffer_size);
    IOReturn    sendHCICommandRequest(UInt8 reqType, UInt8 request, void* data, UInt16 length, UInt16 timeout);
    IOReturn    readHCIEvent();
    /*
#if TARGET_MAC_OSX_10_11
    static void  readHCIEventEndAction(void* target, void* parameter, IOReturn status, UInt32 bufferSizeRemaining);
    void     readHCIEventInterruptHandler(void* parameter, IOReturn status, UInt32 bufferSizeRemaining);
#else
    static void  readHCIEventEndAction(void* owner, void* parameter, IOReturn status, UInt32 bufferSizeRemaining);
    void     readHCIEventInterruptHandler(void* parameter, IOReturn status, UInt32 bufferSizeRemaining);
#endif
    */
#if TARGET_MAC_OSX_10_11
    const EndpointDescriptor*  findEndPointDescriptor(UInt8 etype, UInt8 eDirection);
#endif
    
    void detectOSXVersion();
    
public:
    // IOKit methods
    virtual bool        init(OSDictionary *propTable);
    virtual IOService*  probe(IOService *provider, SInt32 *score );
    
    virtual bool attach(IOService *provider);
    virtual void detach(IOService *provider);
    
    virtual bool start(IOService *provider);
    virtual void stop(IOService *provider);
    virtual void free();
    
    virtual bool handleOpen(IOService *forClient, IOOptionBits options = 0, void *arg = 0 );
    virtual void handleClose(IOService *forClient, IOOptionBits options = 0 );
    
    virtual bool terminate(IOOptionBits options = 0);
    virtual bool finalize(IOOptionBits options);
    
    virtual IOReturn message(UInt32 type, IOService *provider, void *argument);
    
};

#endif
