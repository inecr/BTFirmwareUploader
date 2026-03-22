/*
 *
 * Copyright (C) 2012-2016 by EMlyDinEsHMG
 *
 * Bluetooth Firmware Uploader v3.1 for Mac OSX
 *
 * After hardware reset, some BCM and Atheros Bluetooth adapters obtain their
 * initial firmware from OTPROM chip. Although some of the devices have an OTPROM
 * that contains required firmware, but it is found that these devices would not
 * have HFP/HSP support and other things unless a upgraded firmware is loaded.
 * Once this initial firmware is running, the firmware can be further upgraded
 * over HCI interface with the files provided by Broadcom and Atheros.
 * This is also known as "patch RAM" support.
 *
 * This program patches some Atheros and Broadcom Bluetooth controller firmwares
 * which is needed for these devices to work with full functionalities.
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


#include "BTFirmwareUploader.h"

#define DEBUG_ADV 0

#if DEBUG_ADV
#define DEBUG_LOG(fmt, args...) IOLog(fmt, ## args)
#else
#define DEBUG_LOG(fmt, args...)
#endif

#define DEBUG_BASIC 1

#if DEBUG_BASIC
#define DEBUG_BASIC_LOG(fmt, args...) IOLog(fmt, ## args)
#else
#define DEBUG_BASIC_LOG(fmt, args...)
#endif

// Atheros AR3011 fw
#include "ath3k-win-fw.h"
#include "ath3k-win-fw-2.h"
#include "ath3k-win-fw-3.h"
#include "ath3k-linux-fw.h"

// Atheros AR3012 fw
#include "AthrBT_0x01020001.h"
#include "AthrBT_0x0102020x.h"
#include "AthrBT_0x11020x00.h"
#include "AthrBT_0x31010x00.h"
#include "AthrBT_0x41020000.h"

#include "ramps_0x01020001.h"
#include "ramps_0x01020200.h"
#include "ramps_0x01020201.h"
#include "ramps_0x11020000.h"
#include "ramps_0x11020100.h"
#include "ramps_0x31010000.h"
#include "ramps_0x31010100.h"
#include "ramps_0x41020000.h"

// Broadcom fw
#include "BCM20702A1_001.002.014.1443.1479.hex.h"
#include "BCM20702A1_001.002.014.1443.1480.hex.h"
#include "BCM20702A1_001.002.014.1443.1481.hex.h"
#include "BCM20702A1_001.002.014.1443.1501.hex.h"
#include "BCM20702A1_001.002.014.1443.1502.hex.h"

#include "BCM20702A1_001.002.014.1443.1521.hex.h"
#include "BCM20702A1_001.002.014.1443.1572.hex.h"
#include "BCM20702A1_001.002.014.1443.1461.hex.h"
#include "BCM20702A1_001.002.014.1443.1463.hex.h"

#include "BCM20702A1_001.002.014.1443.1469.hex.h"
#include "BCM20702A1_001.002.014.1443.1447.hex.h"
#include "BCM20702A1_001.002.014.1443.1449.hex.h"
#include "BCM20702A1_001.002.014.1443.1492.hex.h"
#include "BCM20702A1_001.002.014.1443.1493.hex.h"
#include "BCM20702A1_001.002.014.1443.1494.hex.h"
#include "BCM20702A1_001.002.014.1443.1450.hex.h"
#include "BCM20702A1_001.002.014.1443.1467.hex.h"
#include "BCM20702A1_001.002.014.1443.1482.hex.h"
#include "BCM20702A1_001.002.014.1443.1485.hex.h"
#include "BCM20702A1_001.002.014.1443.1714.hex.h"

#include "BCM20702A1_001.002.014.1483.1651.hex.h"

#include "BCM20702A1_001.002.014.1502.1757.hex.h"
#include "BCM20702A1_001.002.014.1502.1762.hex.h"
#include "BCM20702A1_001.002.014.1502.1763.hex.h"
#include "BCM20702A1_001.002.014.1502.1764.hex.h"
#include "BCM20702A1_001.002.014.1502.1766.hex.h"
#include "BCM20702A1_001.002.014.1502.1768.hex.h"
#include "BCM20702A1_001.002.014.1502.1770.hex.h"
#include "BCM20702A1_001.002.014.1502.1780.hex.h"

#include "BCM20703A1_001.001.005.0214.0414.hex.h"

#include "BCM4335C0_003.001.009.0066.0105.hex.h"
#include "BCM4335C0_003.001.009.0066.0115.hex.h"


//
// Variables to monitor the upload
//

unsigned int gUploadCount = 0;
uint64_t gLastUploadTime  = 0;
bool  gLastUpdateSuccess      = true; // assume its success for first update
bool  gLastUpdResetSuccess    = false;


OSDefineMetaClassAndStructors(BTFirmwareUploader, IOService)
#define super IOService

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Override IOKit function which initializes the driver
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

bool BTFirmwareUploader::init(OSDictionary *propTable)
{
    DEBUG_LOG("BTFirmwareUploader :: Init\n");
    
    _USBDev          = NULL;
    _mDeviceLock     = NULL;
    
    _USBOutPipe      = NULL;
    _USBInPipe       = NULL;
    
    _readEventTimer  = NULL;
    _readMemBufDesc  = NULL;
    _serviceClosed   = false;
    _abortOnFail     = false;
    
    _productID  = 0;
    _vendorID   = 0;
    _uploadLoopCounts = 0;
    
    _updateSyncDelay = 2;
    
    return (super::init(propTable));
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Override IOKit function which finds the device
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

IOService* BTFirmwareUploader::probe(IOService *provider, SInt32 *score)
{
    
    DEBUG_LOG("%s :: probe\n", getName());
    return super::probe(provider, score);
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Override IOKit functions which will attach and de-attach the device
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

bool BTFirmwareUploader::attach(IOService *provider)
{
    DEBUG_LOG("%s :: attach\n", getName());
    return super::attach(provider);
}

void BTFirmwareUploader::detach(IOService *provider)
{
    DEBUG_LOG("%s :: detach\n", getName());
    return super::detach(provider);
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Override IOKit functions which handles IOService objects
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

bool BTFirmwareUploader::handleOpen(IOService *forClient, IOOptionBits options, void *arg )
{
    DEBUG_LOG("%s :: handleOpen\n", getName());
    return super::handleOpen(forClient, options, arg);
}

void BTFirmwareUploader::handleClose(IOService *forClient, IOOptionBits options )
{
    DEBUG_LOG("%s :: handleClose\n", getName());
    super::handleClose(forClient, options);
}

bool BTFirmwareUploader::terminate(IOOptionBits options)
{
    DEBUG_LOG("%s :: Terminate\n", getName());
    gLastUpdResetSuccess = true;
    return super::terminate(options);
}


bool BTFirmwareUploader::finalize(IOOptionBits options)
{
    DEBUG_LOG("%s :: Finalize\n", getName());
    return super::finalize(options);
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Override IOKit function which starts and stops the driver
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void BTFirmwareUploader::free()
{
    
#if TARGET_MAC_OSX_10_11
    // Release USB Pipes
    if (_USBOutPipe)
    {
        _USBOutPipe->release();
    }
    if (_USBInPipe)
    {
        _USBInPipe->release();
    }
#endif
    
    // Close the USB Interface
    if (_USBDevIntf)
    {
        _USBDevIntf->close(this);
        _USBDevIntf = NULL;
    }
    
    // Close the USB device
    if (_USBDev)
    {
        _USBDev->close(this);
        _USBDev = NULL;
    }
    
    // Release Mutex Lock
    if (_mDeviceLock)
    {
        IOLockFree(_mDeviceLock);
        _mDeviceLock = NULL;
    }
    
    DEBUG_LOG("%s :: Free\n", getName());
    return super::free();
}

void BTFirmwareUploader::stop(IOService *provider)
{
    DEBUG_LOG("%s :: Stop\n", getName());
    return super::stop(provider);
}

bool BTFirmwareUploader::start(IOService *provider)
{
    
#if APPLESDK
    clock_get_uptime(_now);
#else
    clock_get_uptime((uint64_t *)(&_now));
#endif
    
#if DEBUG_ADV
    uint64_t kextLoadTime  = _now;
#endif
    
    //
    // Last update was very recent ?
    //
    
    if(((_now - gLastUploadTime)/1000000) < 1000 && gUploadCount > 0)
    {
        // Return if last rest was perfect
        if (gLastUpdResetSuccess) {
            DEBUG_LOG("%s :: Firmware already updated, Ignoring.\n", getName());
            return false;
        }
#if !TARGET_MAC_OSX_10_11
        // Wait for some time
        DEBUG_BASIC_LOG("%s :: Last update was very recent, creating 1 sec delay to avoid issues\n", getName());
        IOSleep(1000);
#endif
    }
    
    IOLog("Bluetooth Firmware Uploader v3.1.5 Copyright (c) EMlyDinEsH <www.osxlatitude.com> 2012-2016.\n");
    
    //
    // Start the upload process
    //
    
    DEBUG_LOG("%s :: Start\n", getName());
    
    
    //
    // Get Product and  Vendor ID
    //
    
    OSNumber*   pID = OSDynamicCast(OSNumber, getProperty("idProduct"));
    OSNumber*   vID = OSDynamicCast(OSNumber, getProperty("idVendor"));
    
    if(pID) {
        _productID = pID->unsigned32BitValue();
    }
    if(pID) {
        _vendorID = vID->unsigned32BitValue();
    }
    
    DEBUG_BASIC_LOG("%s :: ProductID 0x%x, VendorID 0x%x\n", getName(), _productID, _vendorID);
    
    
    DEBUG_LOG("%s :: Last upload was done before %lld ms, Upload counts %d, LastTime %lld abs, Now %lld abs, Status -> Update %d Reset %d\n", getName(), (_now - gLastUploadTime)/1000000, gUploadCount, gLastUploadTime, _now, gLastUpdateSuccess, gLastUpdResetSuccess);
    
    //
    // Abort upload if last one was failed attempt,
    // so device won't get dead locked and breaking USB in second try.
    //
    
    if (_abortOnFail && !gLastUpdateSuccess) {
        IOLog("%s :: Last firmware update failed, aborting.\n", getName());
        return false;
    }
    
    //
    // Detect OS X version
    //
    
    detectOSXVersion();
    
    
#if TARGET_MAC_OSX_10_11
    if (_osxVersion < 11) {
        IOLog("%s :: This kext is not supported in OS X version OS X 10.10 (or) lower, so please install the kext built for 10.11 (or) greater.\n", getName());
        return false;
    }
#else
    if (_osxVersion > 10) {
        IOLog("%s :: This kext is not supported in OS X version OS X 10.11 (or) greater, so please install the kext built for OS X 10.10 (or) lower.\n", getName());
        return false;
    }
#endif
    
  
    //
    // Read Plist Config
    //
    
    if (!ReadPlistConfig())
        return false;
    
    //
    // Get the Lock to our device for exclusive access
    //
    _mDeviceLock    = IOLockAlloc();
    
    if (!_mDeviceLock) {
        IOLog("%s :: Failed to allocate mutex lock.\n", getName());
        return false;
    }
    
    //
    // Is this a USB Device ?
    //
    
#if !TARGET_MAC_OSX_10_11
    _USBDev = OSDynamicCast(IOUSBDevice, provider);
#else
    _USBDev = OSDynamicCast(IOUSBHostDevice, provider);
#endif
    
    if(_USBDev == NULL)
    {
        IOLog("%s :: Provider isn't a USB device.\n", getName());
        return false;
    }
    
    //
    // Initialize USB device
    //
    
    if(initializeUSBDevice())
    {
        IOLog("%s :: Failed to initialize the device.\n", getName());
        return false;
    }
    
    //
    // Note: In 10.10 (or) before for Broadcom
    // At boot the kext gets loaded 2 or 3 times in some systems.
    // If we get the IOMessage service status as closed then we
    // can assume already kext was loaded before to update firmware.
    // Becayse service status will be open when the kext was loaded first,
    // and its closed in consequent loads. So we need to avoid update being
    // applied more than one time which can break/KP some systems if the last
    // update was before less than 10 sec.
    //
    
    if(_serviceClosed && !_isAtherosController
       && gUploadCount > 0 && ((_now - gLastUploadTime)/1000000) < 10000)
    {
        IOLog("%s :: Firmware already updated, skipping.\n", getName());
        gLastUpdResetSuccess = !gLastUpdResetSuccess; // Clear/Set for next update check
    }
    else {
        
        //
        // Find the interface and pipes for bulk endpoint transfers
        //
        
        if (findInterface()) {
            return false;
        }
        
        if (findPipes()) {
            return false;
        }
        
        //
        // Process Atheros bluetooth
        //
        
        if(_isAtherosController)
        {
            if(!processAth3kFwUpdate()) {
                return false;
            }
        }
        
        //
        // Process Broadcom bluetooth
        //
        
        else {
            
            IOWorkLoop* readWorkLoop;
            
            //
            // Setup shared workloop for USB Read
            //
            
            readWorkLoop = getWorkLoop();
            
            //
            // Setup USB Read event timer
            //
            
            _readEventTimer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &BTFirmwareUploader::readHCIEvent));
            
            if (!readWorkLoop ||
                (readWorkLoop->addEventSource(_readEventTimer)) != kIOReturnSuccess)
            {
                IOLog("%s :: Failed to setup workloop for read timer.\n",this->getName());
                return false;
            }
            
            //
            // Process update
            //
            
            bcmProcessFirmwareUpdate();
            
            
            //
            // Release buffer
            //
            
            if (_readMemBufDesc)
                _readMemBufDesc->release();
            
            //
            // Release timer and workloop
            //
            
            if(_readEventTimer)
            {
                _readEventTimer->cancelTimeout();
                readWorkLoop->removeEventSource(_readEventTimer);
                _readEventTimer->release();
                _readEventTimer = 0;
            }
            
            if (!gLastUpdateSuccess) {
                IOLog("%s :: Firmware update failed.\n", getName());
                return false;
            }
        }
        
        IOSleep(100); // wait to sync
        
        
#if TARGET_MAC_OSX_10_11
        DEBUG_LOG("%s :: Getting device status ....\n", getName());
        
        UInt16 devStatus;
        
        devStatus = sendHCICommandRequest(makeDeviceRequestbmRequestType(kRequestDirectionIn, kRequestTypeStandard, kRequestRecipientDevice), kDeviceRequestGetStatus, &devStatus, sizeof(devStatus), kUSBHostStandardRequestCompletionTimeout);
        
        IOLog("%s :: Device status %d\n", getName(), (int)devStatus);
#endif
        
        IOReturn ret;
        
        //
        // Send standard device reset for new firmware to load after upload
        // (may be Optional, might not needed in Atheros)
        //
        
        
        DEBUG_LOG("%s :: Sending standard device reset ....\n", getName());
        gLastUpdResetSuccess = false;
        
#if !TARGET_MAC_OSX_10_11
        ret = _USBDev->ResetDevice();
#else
        ret = _USBDev->reset();
#endif
        
#if DEBUG_ADV
        if (ret) {
            IOLog("%s :: Failed to reset the device, error (0x%x).\n", getName(), ret);
        }
        else { IOLog("%s :: Device reset success.\n", getName()); }
#endif
        
        //
        // Get the device status (it may fail for some devices, but its important for operational device)
        //
        
        DEBUG_LOG("%s :: Getting device status ....\n", getName());
        
#if !TARGET_MAC_OSX_10_11

        USBStatus devStatus;
        
        ret = _USBDev->GetDeviceStatus(&devStatus);
        if (ret)
        {
            IOLog("%s :: Unable to get device status, error (0x%x)\n", getName(), ret);
        }
        else {
            IOLog("%s :: Device status %d\n", getName(), (int)devStatus);
        }
#endif
    }
    
    
#if APPLESDK
    clock_get_uptime(_now);
#else
    clock_get_uptime((uint64_t *)(&_now));
#endif
    
    gLastUploadTime = _now;
    
#if DEBUG_ADV
    IOLog("%s :: Total time taken by the kext is %lld ms\n", getName(), (_now - kextLoadTime)/1000000);
#endif
    
    return false;  // return false to allow a apple driver to load
}


//* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Method to detect OSX version using Darwin Kernel version from ioreg root.
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void BTFirmwareUploader::detectOSXVersion()
{
    
    DEBUG_LOG("%s :: Detcting OSX version...\n", getName());
    
    OSString    *iokitbuildObj  = 0;
    const char *iokitbuildStr   = NULL;
    char buildVersString[8]     = {NULL};
    int index = 0, strCount = 0;
    
    IORegistryEntry *ioregRoot = IORegistryEntry::getRegistryRoot();
    
    if (ioregRoot != NULL) {
        
        iokitbuildObj = OSDynamicCast(OSString, ioregRoot->getProperty("IOKitBuildVersion"));
        
        if (iokitbuildObj != NULL) {
            
            DEBUG_LOG("%s :: IOKitBuildVersion %s\n", getName(), iokitbuildObj->getCStringNoCopy());
            iokitbuildStr = iokitbuildObj->getCStringNoCopy();
            
            do {
                if (*iokitbuildStr == ' ' ||
                    *iokitbuildStr == '\0' ||
                    *iokitbuildStr == ':')
                {
                    
                    if (*iokitbuildStr == ':')
                    {
                        buildVersString[index] = '\0';
                    }
                    
                    index = 0;
                    strCount++;
                    
                    if (strCount == 4)
                    {
                        DEBUG_LOG("%s :: OSX Darwin Kernel version - %s\n", getName(), buildVersString);
                        break;
                    }
                    continue;
                }
                
                buildVersString[index] = *iokitbuildStr;
                index++;
                
            } while (*iokitbuildStr++);
            
            
            if (strncmp(buildVersString, "15.0.0", strlen(buildVersString)) >= 0)
            {
                _osxVersion = 11;
            }
            else if (strncmp(buildVersString, "14.0.0", strlen(buildVersString)) >= 0)
            {
                _osxVersion = 10;
            }
            else if (strncmp(buildVersString, "13.0.0", strlen(buildVersString)) >= 0)
            {
                _osxVersion = 9;
            }
            else if (strncmp(buildVersString, "12.0.0", strlen(buildVersString)) >= 0)
            {
                _osxVersion = 8;
            }
            else if (strncmp(buildVersString, "11.0.0", strlen(buildVersString)) >= 0)
            {
                _osxVersion = 7;
            }
            else {
                _osxVersion = 6;
            }
            
            DEBUG_LOG("%s :: OSX Version 10.%d\n", getName(), _osxVersion);
            
        }
        else
            IOLog("%s :: Failed to get IOKitBuildVersion\n", getName());
    }
    else
    {
        _osxVersion = 7;
        IOLog("%s :: Failed to get an object for IOReg root\n", getName());
    }
    
    // OSSafeRelease(ioregRoot);
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Method to read the configuration from plist.
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

bool BTFirmwareUploader::ReadPlistConfig()
{
    //
    // Get the Bluetooth controller name from plist
    //
    
    OSString *btCntrl = OSDynamicCast(OSString, getProperty("BTController"));
    
    if(btCntrl == NULL) {
        IOLog("%s :: Bluetooth controller name missing from plist, aborting.\n", getName());
        return false;
    }
    
    //
    // Check for the Broadcom Bluetooth controller from plist
    //
    
    if(!strncmp(btCntrl->getCStringNoCopy(), BTUSB_BCM_COMBO, strlen(btCntrl->getCStringNoCopy())))
    {
        DEBUG_BASIC_LOG("%s :: Broadcom WiFi/BT combo Bluetooth controller found\n", getName());
        _isAtherosController   = false;
        _isStandaloneBT        = false;
    }
    else
    if(!strncmp(btCntrl->getCStringNoCopy(), BTUSB_BCM_STDAL, strlen(btCntrl->getCStringNoCopy())))
    {
        DEBUG_BASIC_LOG("%s :: Broadcom Standalone Bluetooth controller found\n", getName());
        _isAtherosController   = false;
        _isStandaloneBT        = true;
    }

    //
    // Check for the Atheros Bluetooth controller from plist
    //
    
    else
    {
        if(!strncmp(btCntrl->getCStringNoCopy(), BTUSB_ATH3011, strlen(btCntrl->getCStringNoCopy())))
            _isAR3012Bluetooth = false;
        
        else if(!strncmp(btCntrl->getCStringNoCopy(), BTUSB_ATH3012, strlen(btCntrl->getCStringNoCopy())))
            _isAR3012Bluetooth = true;
        
        else {
            IOLog("%s :: Missing plist value to identify Bluetooth hardware.\n", getName());
            return false;
        }
        
        _isAtherosController = true;
        
        DEBUG_BASIC_LOG("%s :: Atheros %s Bluetooth detected\n", getName(), _isAR3012Bluetooth?"AR3012":"AR3011");
        
        //
        // Linux firmware to use?
        //
        
        if (kOSBooleanTrue == getProperty("Use Linux fw")) {
            _useLinuxFw = true;
        }
        else {
            
            if (!_isAR3012Bluetooth)
            {
                //
                // Windows firmare to use
                // Note: only firmware 1 works with Fn + F2 toggle in Asus, other fw doesn't
                //
                
                OSNumber *ath3kWinFwObj = OSDynamicCast(OSNumber, getProperty("Windows fw"));
                
                if(ath3kWinFwObj == NULL) {
                    IOLog("%s :: Windows fw value missing from plist, aborting.\n", getName());
                    return false;
                }
                else
                    _ath3kWinFw = ath3kWinFwObj->unsigned32BitValue();
            }
            _useLinuxFw = false;
        }
        
        DEBUG_BASIC_LOG("%s :: Using %s fimrware\n", getName(), _useLinuxFw?"Linux":"Windows");
    }
    
    //
    // Get the Pipes to use for In and Out from plist if found
    //
    
    OSNumber* in    = OSDynamicCast(OSNumber, getProperty("Pipe IN"));
    OSNumber* out   = OSDynamicCast(OSNumber, getProperty("Pipe OUT"));
    
    if(in == NULL) {
        _wInPipe = 0;
        DEBUG_LOG("%s :: Pipe IN value missing from plist, using standard Pipe\n", getName());
    }
    else {
        _wInPipe = in->unsigned32BitValue();
        DEBUG_LOG("%s :: Using Pipe %d for IN\n", getName(), _wInPipe);
    }
    
    if(out == NULL) {
        _wOutPipe = 0;
        DEBUG_LOG("%s :: Pipe OUT value missing from plist, using standard Pipe\n", getName());
    }
    else {
        _wOutPipe = out->unsigned32BitValue();
        
        if(_wOutPipe == 0)
            _useCtrlPipe = true;
        
        DEBUG_LOG("%s :: Using Pipe %d for OUT\n", getName(), _wOutPipe);
    }
    
    //
    // Plist had Broadcom sync delay ?
    //
    
    if (!_isAtherosController) {
        
        OSNumber *updSynDelay = OSDynamicCast(OSNumber, getProperty(UPD_SYNC_DELAY));
        
        if(updSynDelay == NULL) {
            DEBUG_LOG("%s :: Update sync delay value missing from plist, using standard value.\n", getName());
            _updateSyncDelay = 2;
        }
        else
            _updateSyncDelay = updSynDelay->unsigned8BitValue();
        
        DEBUG_LOG("%s :: Update Sync Delay %d ms\n", getName(), _updateSyncDelay);
    }
    
    //
    // Abort updates if last update failed ?
    //
    
    if (!_isAtherosController) {
        
        OSBoolean *abortUpd = OSDynamicCast(OSBoolean, getProperty(ABORT_UPD_FAIL));
        
        if(abortUpd == NULL) {
            IOLog("%s :: AbortOnFwUpadFail value missing from plist, using standard abort on fail.\n", getName());
            _abortOnFail = true;
        }
        else {
            _abortOnFail = abortUpd->getValue();
        }
        
        DEBUG_LOG("%s :: Abort updates if Last fw Update was fail -> %s\n", getName(), _abortOnFail?"Yes":"No");
    }
    
    return true;
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Override IOKit function which receives messages from the device.
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

IOReturn BTFirmwareUploader::message(UInt32 type, IOService *provider, void *argument)
{
    DEBUG_LOG("%s :: Notification message 0x%x, Service Status %s\n", getName(), type, _serviceClosed?"Closed":"Open");
    
    switch ( type )
    {
        case 0xe000400a:
            DEBUG_LOG("%s :: USB device reset finished.\n", getName());
            break;
            
        case kIOMessageServiceIsAttemptingOpen:
            DEBUG_LOG("%s :: Service attempting to open\n", getName());
            break;
            
        case kIOMessageServiceIsTerminated:
            DEBUG_LOG("%s :: Service is terminated\n", getName());
            break;
            
        case kIOMessageServiceWasClosed:
            DEBUG_LOG("%s :: Service closed\n", getName());
            _serviceClosed = true;
            break;
            
        case kIOMessageServiceIsSuspended:
        case kIOMessageServiceIsResumed:
        case kIOMessageServiceIsRequestingClose:
        case kIOMessageServiceBusyStateChange:
        default:
            break;
    }
    
    // return kIOReturnSuccess;
    return super::message(type, provider, argument);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Functions which does USB device Initialization.
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

IOReturn BTFirmwareUploader::initializeUSBDevice()
{
    IOReturn ret = kIOReturnSuccess;
    
#if !TARGET_MAC_OSX_10_11
    
    ret = _USBDev->ResetDevice();

    if (ret)
    {
        IOLog("%s :: Failed to reset the device (error %x)\n", getName(), ret);
        return ret;
    }
    else {
        DEBUG_LOG("%s :: Standard device reset success\n", getName());
    }
    
    // Find the configuations
    int numconf = 0;
    
    numconf = _USBDev->GetNumConfigurations();
    if (numconf < 1)
    {
        IOLog("%s :: No composite configurations found\n", getName());
        return kIOReturnError;
    }
    else {
        DEBUG_LOG("%s :: %d Configurations descriptors found\n", getName(), numconf);
    }
    
    // Get the details of first configuration descriptor
    const IOUSBConfigurationDescriptor *confDesc;
    
    confDesc = _USBDev->GetFullConfigurationDescriptor(0);
    if (!confDesc)
    {
        IOLog("%s :: No configuration descriptor found\n", getName());
        return kIOReturnError;
    }
    
    // Open the USB device to configure
    if (!_USBDev->open(this))
    {
        IOLog("%s :: Unable to open device for configuration\n", getName());
        return kIOReturnError;
    }
    
    // Set the configuration to the first config
    ret = _USBDev->SetConfiguration(this, confDesc->bConfigurationValue, true);
    if (ret)
    {
        IOLog("%s :: Unable to set the configuration\n", getName());
        return ret;
    }
    
    // Get the status of the USB device
    USBStatus devStatus;
    
    ret = _USBDev->GetDeviceStatus(&devStatus);
    if (ret)
    {
        IOLog("%s :: Unable to get device status, error (0x%x)\n", getName(), ret);
    }
    else {
        DEBUG_LOG("%s :: Device status %d\n", getName(), (int)devStatus);
    }

#else
    
    // Get the device descriptor
    const DeviceDescriptor*    _USBDevDesc = NULL;
    
    _USBDevDesc = _USBDev->getDeviceDescriptor();
    if (!_USBDevDesc) {
        IOLog("%s :: Failed to get the device descriptor.\n", getName());
        return kIOReturnError;
    }
    
    // Find the configuations
    UInt8 numconf = 0;
    
    numconf = _USBDevDesc->bNumConfigurations;
    if (numconf == 0)
    {
        IOLog("%s :: No composite configurations found.\n", getName());
        return kIOReturnError;
    }
    else {
        DEBUG_LOG("%s :: %d Configurations descriptors found\n", getName(), numconf);
    }
    
    // Get the details of first configuration descriptor
    const ConfigurationDescriptor *confDesc;
    
    confDesc = _USBDev->getConfigurationDescriptor(0);
    if (!confDesc)
    {
        IOLog("%s :: No configuration descriptor found\n", getName());
        return kIOReturnError;
    }
    
    // Open the USB device to configure
    if (!_USBDev->open(this))
    {
        IOLog("%s :: Unable to open device for configuration\n", getName());
        return kIOReturnError;
    }
    
    // Set the configuration to the first config
    ret = _USBDev->setConfiguration(confDesc->bConfigurationValue);
    if (ret)
    {
        IOLog("%s :: Unable to set the configuration\n", getName());
        return ret;
    }
    
    // Get the status of the USB device
    UInt16 devStatus;
    
    devStatus = sendHCICommandRequest(makeDeviceRequestbmRequestType(kRequestDirectionIn, kRequestTypeStandard, kRequestRecipientDevice), kDeviceRequestGetStatus, &devStatus, sizeof(devStatus), kUSBHostStandardRequestCompletionTimeout);
    
    DEBUG_LOG("%s :: Device status %d\n", getName(), devStatus);
#endif
    
    return ret;
}

IOReturn BTFirmwareUploader::findInterface()
{
    
#if !TARGET_MAC_OSX_10_11
    IOUSBFindInterfaceRequest intfRequest;
    
    intfRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
    intfRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    intfRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    intfRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;
    
    _USBDevIntf = _USBDev->FindNextInterface(NULL, &intfRequest);
    if (!_USBDevIntf) {
        IOLog("%s :: unable to find interface\n", getName());
        return kIOReturnError;
    }
    
    if (!_USBDevIntf->open(this))
    {
        IOLog("%s :: unable to open interface\n", getName());
        return kIOReturnError;
    }
#else
    OSIterator* iterator = _USBDev->getChildIterator(gIOServicePlane);
    OSObject* candidate = NULL;
    while(iterator != NULL && (candidate = iterator->getNextObject()) != NULL)
    {
        IOUSBHostInterface* interfaceCandidate = OSDynamicCast(IOUSBHostInterface, candidate);
        
        if(interfaceCandidate != NULL) {
            switch (interfaceCandidate->getInterfaceDescriptor()->bInterfaceClass)
            {
                case kUSBWirelessControllerInterfaceClass:
                case kUSBVendorSpecificInterfaceClass:
                    _USBDevIntf = interfaceCandidate;
                    break;
                    
                default:
                    IOLog("%s :: Unknown Interface classs %d\n", getName(), interfaceCandidate->getInterfaceDescriptor()->bInterfaceClass);
                    break;
            }
            break;
        }
    }
    OSSafeReleaseNULL(iterator);
    
    if (!_USBDevIntf) {
        IOLog("%s :: unable to find interface\n", getName());
        return kIOReturnError;
    }
    
    if (!_USBDevIntf->open(this))
    {
        IOLog("%s :: unable to open interface\n", getName());
        return kIOReturnError;
    }
#endif
    
    return kIOReturnSuccess;
}

#if TARGET_MAC_OSX_10_11
const EndpointDescriptor* BTFirmwareUploader::findEndPointDescriptor(UInt8 eType, UInt8 eDirection)
{
    const ConfigurationDescriptor* intfConfDesc;
    const InterfaceDescriptor* intDesc = NULL;
    const EndpointDescriptor* endpDesc = NULL;
    
    intfConfDesc = _USBDevIntf->getConfigurationDescriptor();
    if (!intfConfDesc)
    {
        IOLog("%s :: No interface configuration descriptor found\n", getName());
        return NULL;
    }
    
    bool found = false;
    
    while((intDesc = getNextInterfaceDescriptor(intfConfDesc, intDesc)) != NULL)
    {
        DEBUG_LOG("%s :: Interator desc found\n", getName());
        
        switch (intDesc->bInterfaceClass)
        {
            case kUSBWirelessControllerInterfaceClass:
            case kUSBVendorSpecificInterfaceClass:
                
                while((endpDesc = getNextEndpointDescriptor(intfConfDesc, intDesc, endpDesc)) != NULL)
                {
                    DEBUG_LOG("%s :: End Point Direction %d Type %d, Req Type %d\n", getName(), getEndpointDirection(endpDesc), getEndpointType(endpDesc), eType);
                    
                    if(getEndpointType(endpDesc) == eType && getEndpointDirection(endpDesc) == eDirection)
                    {
                        DEBUG_LOG("%s :: Requested End Point Direction %d Type %d found\n", getName(), eDirection, eType);
                        found = true;
                        break;
                    }
                }
                break;
                
            default:
                IOLog("%s :: Unknown Interface classs %d\n", getName(), intDesc->bInterfaceClass);
                break;
        }
        
        if (found)
            break;
    }
    
    if (!intDesc) {
        IOLog("%s :: unable to find interface descriptor\n", getName());
        return NULL;
    }
    
    return endpDesc;
}
#endif

IOReturn BTFirmwareUploader::findPipes()
{
    //
    // Device uses control pipe for out ?
    //
    
    if (_useCtrlPipe) {
        
#if !TARGET_MAC_OSX_10_11
        _USBOutPipe = _USBDevIntf->GetPipeObj(_wOutPipe);
#else
        _USBOutPipe = _USBDevIntf->copyPipe(_wOutPipe);
#endif
        if (!_USBOutPipe) {
            _useCtrlPipe = false;
            IOLog("%s :: Failed to find control pipe %d, will try to use Bulk out pipe\n", getName(), _wOutPipe);
        }
        else if(_isAtherosController)
            return kIOReturnSuccess;
    }
    
    //
    // Uses standard (or) user provided pipes from plist
    //
    
    if (!_useCtrlPipe || (_useCtrlPipe && !_USBOutPipe)) {
        
        DEBUG_LOG("%s :: Searching for the Bulk OUT end point...\n", getName());
        
#if !TARGET_MAC_OSX_10_11
        UInt8 transferType = 0;
        UInt8 interval = 0;
        UInt16 maxPacketSize = 0;
        IOReturn ret;
        
        // No user provided pipe value ?
        if(_wOutPipe == 0) {
            IOUSBFindEndpointRequest pipereq;
            pipereq.type = kUSBBulk;
            pipereq.direction = kUSBOut;
            
            _USBOutPipe = _USBDevIntf->FindNextPipe(NULL, &pipereq);
            
            if (_USBOutPipe == NULL) {
                IOLog("%s :: Failed to find Bulk OUT pipe\n", getName());
                return kIOReturnError;
            }
        }
        // Uses user provided Bulk OUT pipe from plist
        else {
            _USBOutPipe = _USBDevIntf->GetPipeObj(_wOutPipe);
            
            if (!_USBOutPipe) {
                IOLog("%s :: Failed to find user provided OUT pipe %d\n", getName(), _wOutPipe);
                return kIOReturnError;
            }
        }
        
        _wOutPipe = _USBOutPipe->GetEndpointNumber();
        ret = _USBDevIntf->GetEndpointProperties(0, _wOutPipe, kUSBOut, &transferType, &maxPacketSize, &interval);
        
        if (ret) {
            IOLog("%s :: Failed to get the Bulk OUT endpoint %d properties\n", getName(), _wOutPipe);
        }
        else {
            DEBUG_LOG("%s :: Bulk OUT End Point 0x%x TranfType %d PacketSize %d Interval %d\n", getName(), _wOutPipe, transferType, maxPacketSize, interval);
        }
#else
            const EndpointDescriptor* eBulkDesc = NULL;
            eBulkDesc = findEndPointDescriptor(kUSBBulk, kUSBOut);
            
            // No user provided pipe value ?
            if(_wOutPipe == 0) {
                
                if (eBulkDesc == NULL)
                {
                    IOLog("%s :: Failed to find the end point Bulk OUT pipe descriptor\n", getName());
                    return kIOReturnError;
                }
                
                _wOutPipe = eBulkDesc->bEndpointAddress;
                
                DEBUG_LOG("%s :: Bulk OUT End Point 0x%x PacketSize %d Interval %d\n", getName(), _wOutPipe, eBulkDesc->wMaxPacketSize, eBulkDesc->bInterval);
            }
            
            _USBOutPipe = _USBDevIntf->copyPipe(_wOutPipe);
            
            if (_USBOutPipe == NULL) {
                IOLog("%s :: Failed to find Bulk OUT pipe\n", getName());
                return kIOReturnError;
            }
#endif
        
        DEBUG_LOG("%s :: Searching for the IN end point...\n", getName());
        
        // We need Pipe IN for Broadcom but not for Atheros
        if(!_isAtherosController) {

#if !TARGET_MAC_OSX_10_11
            // No user provided pipe value ?
            if(_wInPipe == 0) {
                IOUSBFindEndpointRequest pipereq;
                pipereq.type = kUSBInterrupt;
                pipereq.direction = kUSBIn;
                
                _USBInPipe = _USBDevIntf->FindNextPipe(NULL, &pipereq);
                
                if (_USBInPipe == NULL) {
                    IOLog("%s :: Failed to find IN pipe\n", getName());
                    return kIOReturnError;
                }
            }
            // Uses user provided IN pipe from plist
            else {
                _USBInPipe = _USBDevIntf->GetPipeObj(_wInPipe);
                
                if (!_USBInPipe) {
                    IOLog("%s :: Failed to find user provided IN pipe\n", getName());
                    return kIOReturnError;
                }
            }
            
            _wInPipe = _USBInPipe->GetEndpointNumber();
            ret = _USBDevIntf->GetEndpointProperties(0, _wInPipe, kUSBIn, &transferType, &maxPacketSize, &interval);
            
            if (ret) {
                IOLog("%s :: Failed to get the IN endpoint %d properties\n", getName(), _wInPipe);
            }
            else {
                DEBUG_LOG("%s :: IN End Point 0x%x TranfType %d PacketSize %d Interval %d\n", getName(), _wInPipe, transferType, maxPacketSize, interval);
            }
#else
            const EndpointDescriptor* eInDesc = NULL;
            eInDesc = findEndPointDescriptor(kUSBInterrupt, kUSBIn);
            
            // No user provided pipe value ?
            if(_wInPipe == 0) {
                
                if (eInDesc == NULL)
                {
                    IOLog("%s :: Failed to find the end point Bulk IN pipe descriptor\n", getName());
                    return kIOReturnError;
                }
                _wInPipe = eInDesc->bEndpointAddress;
                
                DEBUG_LOG("%s :: IN End Point 0x%x PacketSize %d Interval %d\n", getName(), _wInPipe, eInDesc->wMaxPacketSize, eInDesc->bInterval);
            }
            
            _USBInPipe = _USBDevIntf->copyPipe(_wInPipe);
            
            if (_USBInPipe == NULL) {
                IOLog("%s :: Failed to find IN pipe\n", getName());
                return kIOReturnError;
            }
#endif
        }
    }
    
    return kIOReturnSuccess;
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Function which process Ath3k firmware update.
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

bool BTFirmwareUploader::processAth3kFwUpdate()
{
    unsigned char state = 0;
    bool success = true;
    
    if(!_useCtrlPipe) {
        
        //
        // Send request to control endpoint to check the device state
        //
        
        if(sendHCICommandRequest(0xc0, USB_REQ_GET_STATE, &state, 1, USB_CNTRL_REQ_TIMEOUT))
        {
            IOLog("%s :: Failed to get the device state.\n", getName());
            return false;
        }
        else {
            DEBUG_LOG("%s :: Device state 0x%x\n", getName(), state);
        }
    }
    
    //
    // AR3011 ?
    //
    
    if (!_isAR3012Bluetooth)
    {
        //
        // Device has fw ?
        //
        
        if (state == USB_FIRMWARE_RAM_MODE)
        {
            DEBUG_BASIC_LOG("%s :: Firmware already available in RAM, aborting upload.\n", getName());
            return false;
        }
        
        //
        // Device is in flash mode without fw ?
        //
        
        else {
            
            DEBUG_BASIC_LOG("%s :: Firmware not available in RAM, beginning upload...\n", getName());
            
            unsigned char* buf = NULL;
            unsigned int size = 0;
            char* fwString = NULL;
            
            //
            // Load firmware data based on plist option for Atheros card
            //
            
            if(_useLinuxFw) {
                buf = ath3k_linux_fw_buf;
                size = sizeof(ath3k_linux_fw_buf);
                fwString = (char *)"Linux";
            }
            else {
                switch(_ath3kWinFw) {
                    case 1:
                        buf = ath3k_win_fw_buf;
                        size = sizeof(ath3k_win_fw_buf);
                        fwString = (char *)"Windows (1)";
                        break;
                        
                    case 2:
                        buf = ath3k_win_fw_2_buf;
                        size = sizeof(ath3k_win_fw_2_buf);
                        fwString = (char *)"Windows (2)";
                        break;
                        
                    default:
                        buf = ath3k_win_fw_3_buf;
                        size = sizeof(ath3k_win_fw_3_buf);
                        fwString = (char *)"Windows (3)";
                        if(_ath3kWinFw != 3 && _ath3kWinFw != 0)
                        {
                            DEBUG_LOG("%s :: Wrong firmware value found in plist, using default fw\n", getName());
                        }
                        break;
                }
            }
            
            DEBUG_BASIC_LOG("%s :: Started the upload of %s firmware (%d bytes)......\n", getName(), fwString, size);
            
            //
            // Send HCI command request to control endpoint to initiate the firmware transfer
            //
            
            if(sendHCICommandRequest(0x40, ATH3K_DNLOAD, buf, FW_HDR_SIZE, USB_CNTRL_REQ_TIMEOUT))
            {
                IOLog("%s :: Failed to initiate firmware transfer.\n", getName());
                return false;
            }
            
            buf += FW_HDR_SIZE;
            size -= FW_HDR_SIZE;
            
            //
            // Lock device
            //
            
            IOLockLock(_mDeviceLock);
            
            //
            // Send firmware to device
            //
            
            if(writeFirmwareToDevice(buf, size))
            {
                IOLog("%s :: Failed to update the firmare.\n", getName());
                success = false;
            }
            else {
                IOLog("%s :: Firmware uploaded to device successfully.\n", getName());
                gUploadCount++;
            }
            
            //
            // UnLock device
            //
            
            IOLockUnlock(_mDeviceLock);
        }
    }
    
    //
    // AR3012
    //
    
    else
    {
        //
        // Patch applied and sysconfig Loaded ?
        //

        if (state & ATH3K_SYSCFG_UPDATE) {
            DEBUG_BASIC_LOG("%s :: Updated firmware already available in RAM, aborting upload.\n", getName());
            return false;
        }
        
        //
        // Patch applied ? Load sysconfig only
        //
        
        else if (state & ATH3K_PATCH_UPDATE) {
            
            DEBUG_BASIC_LOG("%s :: Firmware patch was already applied, just loading sysconfig...\n", getName());
            
            // Load system config files only
            if(!ath3012PatchAndSysConfig(false))
            {
                IOLog("%s :: Failed to load sysconfig.\n", getName());
                success = false;
            }
            else {
                gUploadCount++;
            }
        }
        
        //
        // AR3012 ? Apply patch and load sysconfig
        //
        
        else {
            if(!ath3012PatchAndSysConfig(true))
            {
                IOLog("%s :: Failed to patch the firmware.\n", getName());
                success = false;
            }
            else {
                gUploadCount++;
            }
        }
    }
    
    return success;
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Functions which process AR3012 firmware patch and sysconfig file load.
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

bool BTFirmwareUploader::ath3012PatchAndSysConfig(bool patchFw)
{
    unsigned int patch_fw_size = 0;
    struct ath3k_version patch_fw_version, fw_version;
    
    DEBUG_LOG("%s :: Reading the firmware version...\n", getName());
    
    //
    // Send HCI command request to control endpoint to check the device firmware version
    //
    
    if(sendHCICommandRequest(0xc0, ATH3K_GETVERSION, &fw_version, sizeof(fw_version), USB_CNTRL_REQ_TIMEOUT))
    {
        IOLog("%s :: Failed to get the firmware version\n", getName());
        return false;
    }
    
    struct ath3k_version *dev_fw_ver = &fw_version;
    
    DEBUG_LOG("%s :: Firmware version 0x%x, Ref Clock 0x%x\n", getName(), dev_fw_ver->rom_version, dev_fw_ver->ref_clock);
    
    //
    // Get the firmware update data to send
    //
    
    if(patchFw) {
        
        unsigned char * patch_buf = NULL;
        patch_fw_version.rom_version = 0;
        patch_fw_version.build_version = 0;
        
        switch(dev_fw_ver->rom_version) {
            case 0x11020000:
                if(_useLinuxFw)
                {
                    patch_buf = ath3k_linux_11020000_dfu_buf;
                    patch_fw_size = sizeof(ath3k_linux_11020000_dfu_buf);
                    patch_fw_version.rom_version = *(int *)(ath3k_linux_11020000_dfu_buf + sizeof(ath3k_linux_11020000_dfu_buf) - 8);
                    patch_fw_version.build_version = *(int *)(ath3k_linux_11020000_dfu_buf + sizeof(ath3k_linux_11020000_dfu_buf) - 4);
                } else {
                    patch_buf = ath3k_win_11020000_dfu_buf;
                    patch_fw_size = sizeof(ath3k_win_11020000_dfu_buf);
                    patch_fw_version.rom_version = *(int *)(ath3k_win_11020000_dfu_buf + sizeof(ath3k_win_11020000_dfu_buf) - 8);
                    patch_fw_version.build_version = *(int *)(ath3k_win_11020000_dfu_buf + sizeof(ath3k_win_11020000_dfu_buf) - 4);
                }
                break;
                
            case 0x11020100:
                patch_buf = ath3k_win_11020100_dfu_buf;
                patch_fw_size = sizeof(ath3k_win_11020000_dfu_buf);
                patch_fw_version.rom_version = *(int *)(ath3k_win_11020100_dfu_buf + sizeof(ath3k_win_11020100_dfu_buf) - 8);
                patch_fw_version.build_version = *(int *)(ath3k_win_11020100_dfu_buf + sizeof(ath3k_win_11020100_dfu_buf) - 4);
                
                break;
                
            case 0x010200001:
                patch_buf = ath3k_linux_01020001_dfu_buf;
                patch_fw_size = sizeof(ath3k_linux_01020001_dfu_buf);
                patch_fw_version.rom_version = *(int *)(ath3k_linux_01020001_dfu_buf + sizeof(ath3k_linux_01020001_dfu_buf) - 8);
                patch_fw_version.build_version = *(int *)(ath3k_linux_01020001_dfu_buf + sizeof(ath3k_linux_01020001_dfu_buf) - 4);
                
                break;
                
            case 0x01020200:
                if(_useLinuxFw)
                {
                    patch_buf = ath3k_linux_01020200_dfu_buf;
                    patch_fw_size = sizeof(ath3k_linux_01020200_dfu_buf);
                    patch_fw_version.rom_version = *(int *)(ath3k_linux_01020200_dfu_buf + sizeof(ath3k_linux_01020200_dfu_buf) - 8);
                    patch_fw_version.build_version = *(int *)(ath3k_linux_01020200_dfu_buf + sizeof(ath3k_linux_01020200_dfu_buf) - 4);
                } else {
                    patch_buf = ath3k_win_01020200_dfu_buf;
                    patch_fw_size = sizeof(ath3k_win_01020200_dfu_buf);
                    patch_fw_version.rom_version = *(int *)(ath3k_win_01020200_dfu_buf + sizeof(ath3k_win_01020200_dfu_buf) - 8);
                    patch_fw_version.build_version = *(int *)(ath3k_win_01020200_dfu_buf + sizeof(ath3k_win_01020200_dfu_buf) - 4);
                }
                break;
                
            case 0x01020201:
                if(_useLinuxFw)
                {
                    patch_buf = ath3k_linux_01020201_dfu_buf;
                    patch_fw_size = sizeof(ath3k_linux_01020201_dfu_buf);
                    patch_fw_version.rom_version = *(int *)(ath3k_linux_01020201_dfu_buf + sizeof(ath3k_linux_01020201_dfu_buf) - 8);
                    patch_fw_version.build_version = *(int *)(ath3k_linux_01020201_dfu_buf + sizeof(ath3k_linux_01020201_dfu_buf) - 4);
                } else {
                    patch_buf = ath3k_win_01020201_dfu_buf;
                    patch_fw_size = sizeof(ath3k_win_01020201_dfu_buf);
                    patch_fw_version.rom_version = *(int *)(ath3k_win_01020201_dfu_buf + sizeof(ath3k_win_01020201_dfu_buf) - 8);
                    patch_fw_version.build_version = *(int *)(ath3k_win_01020201_dfu_buf + sizeof(ath3k_win_01020201_dfu_buf) - 4);
                }
                break;
                
            case 0x31010000:
                if(_useLinuxFw)
                {
                    patch_buf = ath3k_linux_31010000_dfu_buf;
                    patch_fw_size = sizeof(ath3k_linux_31010000_dfu_buf);
                    patch_fw_version.rom_version = *(int *)(ath3k_linux_31010000_dfu_buf + sizeof(ath3k_linux_31010000_dfu_buf) - 8);
                    patch_fw_version.build_version = *(int *)(ath3k_linux_31010000_dfu_buf + sizeof(ath3k_linux_31010000_dfu_buf) - 4);
                } else {
                    patch_buf = ath3k_win_31010000_dfu_buf;
                    patch_fw_size = sizeof(ath3k_win_31010000_dfu_buf);
                    patch_fw_version.rom_version = *(int *)(ath3k_win_31010000_dfu_buf + sizeof(ath3k_win_31010000_dfu_buf) - 8);
                    patch_fw_version.build_version = *(int *)(ath3k_win_31010000_dfu_buf + sizeof(ath3k_win_31010000_dfu_buf) - 4);
                }
                break;
                
            case 0x31010100:
                patch_buf = ath3k_win_31010100_dfu_buf;
                patch_fw_size = sizeof(ath3k_win_31010100_dfu_buf);
                patch_fw_version.rom_version = *(int *)(ath3k_win_31010100_dfu_buf + sizeof(ath3k_win_31010100_dfu_buf) - 8);
                patch_fw_version.build_version = *(int *)(ath3k_win_31010100_dfu_buf + sizeof(ath3k_win_31010100_dfu_buf) - 4);
                break;
                
            case 0x41020000:
                if(_useLinuxFw)
                {
                    patch_buf = ath3k_linux_41020000_dfu_buf;
                    patch_fw_size = sizeof(ath3k_linux_41020000_dfu_buf);
                    patch_fw_version.rom_version = *(int *)(ath3k_linux_41020000_dfu_buf + sizeof(ath3k_linux_41020000_dfu_buf) - 8);
                    patch_fw_version.build_version = *(int *)(ath3k_linux_41020000_dfu_buf + sizeof(ath3k_linux_41020000_dfu_buf) - 4);
                } else {
                    patch_buf = ath3k_win_41020000_dfu_buf;
                    patch_fw_size = sizeof(ath3k_win_41020000_dfu_buf);
                    patch_fw_version.rom_version = *(int *)(ath3k_win_41020000_dfu_buf + sizeof(ath3k_win_41020000_dfu_buf) - 8);
                    patch_fw_version.build_version = *(int *)(ath3k_win_41020000_dfu_buf + sizeof(ath3k_win_41020000_dfu_buf) - 4);
                }
                break;
                
            default:
                break;
        }
        
        //
        // Check device and patch firmware version
        //
        
        if((patch_fw_version.rom_version != dev_fw_ver->rom_version) ||
           (patch_fw_version.build_version <= dev_fw_ver->build_version))
        {
            IOLog("%s :: Patch firmware version did not match with device firmware\n", getName());
            return false;
        }
        else {
            
            DEBUG_BASIC_LOG("%s :: Sending firmware update patch to device (dfu buffer size %d bytes)...\n", getName(), patch_fw_size);
            
            //
            // Send HCI command request to control endpoint to initiate the firmware transfer
            //
            
            if(sendHCICommandRequest(0x40, ATH3K_DNLOAD, patch_buf, FW_HDR_SIZE, USB_CNTRL_REQ_TIMEOUT))
            {
                IOLog("%s :: Failed to initiate firmware transfer.\n", getName());
                return false;
            }
            
            patch_buf += FW_HDR_SIZE;
            patch_fw_size -= FW_HDR_SIZE;
            
            //
            // Lock device
            //
            
            IOLockLock(_mDeviceLock);
            
            //
            // Send firmware update to device
            //
            
            if(writeFirmwareToDevice(patch_buf, patch_fw_size)) {
                IOLog("%s :: Failed to load firmware update patch into device.\n", getName());
                
                //
                // UnLock device
                //
                
                IOLockUnlock(_mDeviceLock);
                
                return false;
            }
            else {
                DEBUG_BASIC_LOG("%s :: Firmware update patch loaded successfully into device.\n", getName());
            }
            
            //
            // UnLock device
            //
            
            IOLockUnlock(_mDeviceLock);
        }
        
    }
    
    unsigned char dev_state = 0;
    
    //
    // Check the device state after the firmware update patch
    //
    
    if(sendHCICommandRequest(0xc0, USB_REQ_GET_STATE, &dev_state, 1, USB_CNTRL_REQ_TIMEOUT))
    {
        IOLog("%s :: Failed to get device state after patch.\n", getName());
        return false;
    }
    else {
        DEBUG_LOG("%s :: Device state after fw update patch 0x%x\n", getName(), dev_state);
    }
    
    unsigned char * ramps_buf = NULL;
    unsigned int ramps_buf_size = 0;
    
    //
    // Get the sysconfig data to send
    //
    
    switch(dev_fw_ver->rom_version) {
        case 0x11020000:
            if(dev_fw_ver->ref_clock == ATH3K_XTAL_FREQ_40M) {
                if(_useLinuxFw)
                {
                    ramps_buf = ath3k_linux_ramps_0x11020000_40_buf;
                    ramps_buf_size = sizeof(ath3k_linux_ramps_0x11020000_40_buf);
                } else {
                    ramps_buf = ath3k_win_ramps_0x11020000_40_buf;
                    ramps_buf_size = sizeof(ath3k_win_ramps_0x11020000_40_buf);
                }
            }
            else {
                IOLog("%s :: Unknown ref clock 0x%x\n", getName(),dev_fw_ver->ref_clock);
                return false;
            }
            break;
            
        case 0x11020100: // only Windows has these files
            if(dev_fw_ver->ref_clock == ATH3K_XTAL_FREQ_40M) {
                ramps_buf = ath3k_win_ramps_0x11020100_40_buf;
                ramps_buf_size = sizeof(ath3k_win_ramps_0x11020100_40_buf);
            }
            else {
                IOLog("%s :: Unknown ref clock 0x%x\n", getName(),dev_fw_ver->ref_clock);
                return false;
            }
            break;
            
        case 0x010200001: // only Linux has these files
            if(dev_fw_ver->ref_clock == ATH3K_XTAL_FREQ_26M) {
                ramps_buf = ath3k_linux_ramps_0x01020001_26_buf;
                ramps_buf_size = sizeof(ath3k_linux_ramps_0x01020001_26_buf);
            }
            else {
                IOLog("%s :: Unknown ref clock 0x%x\n", getName(),dev_fw_ver->ref_clock);
                return false;
            }
            break;
            
        case 0x01020200:
            if(dev_fw_ver->ref_clock == ATH3K_XTAL_FREQ_26M) {
                if(_useLinuxFw)
                {
                    ramps_buf = ath3k_linux_ramps_0x01020200_26_buf;
                    ramps_buf_size = sizeof(ath3k_linux_ramps_0x01020200_26_buf);
                } else {
                    ramps_buf = ath3k_win_ramps_0x01020200_26_buf;
                    ramps_buf_size = sizeof(ath3k_win_ramps_0x01020200_26_buf);
                }
                
            }
            else if(dev_fw_ver->ref_clock == ATH3K_XTAL_FREQ_40M) {
                if(_useLinuxFw)
                {
                    ramps_buf = ath3k_linux_ramps_0x01020200_40_buf;
                    ramps_buf_size = sizeof(ath3k_linux_ramps_0x01020200_40_buf);
                } else {
                    ramps_buf = ath3k_win_ramps_0x01020200_40_buf;
                    ramps_buf_size = sizeof(ath3k_win_ramps_0x01020200_40_buf);
                }
                
            }
            else {
                IOLog("%s :: Unknown ref clock 0x%x\n", getName(),dev_fw_ver->ref_clock);
                return false;
            }
            break;
            
        case 0x01020201:
            if(dev_fw_ver->ref_clock == ATH3K_XTAL_FREQ_26M) {
                if(_useLinuxFw)
                {
                    ramps_buf = ath3k_linux_ramps_0x01020201_26_buf;
                    ramps_buf_size = sizeof(ath3k_linux_ramps_0x01020201_26_buf);
                } else {
                    ramps_buf = ath3k_win_ramps_0x01020201_26_buf;
                    ramps_buf_size = sizeof(ath3k_win_ramps_0x01020201_26_buf);
                }
                
            }
            else if(dev_fw_ver->ref_clock == ATH3K_XTAL_FREQ_40M) {
                if(_useLinuxFw)
                {
                    ramps_buf = ath3k_linux_ramps_0x01020201_40_buf;
                    ramps_buf_size = sizeof(ath3k_linux_ramps_0x01020201_40_buf);
                } else {
                    ramps_buf = ath3k_win_ramps_0x01020201_40_buf;
                    ramps_buf_size = sizeof(ath3k_win_ramps_0x01020201_40_buf);
                }
                
            }
            else {
                IOLog("%s :: Unknown ref clock 0x%x\n", getName(),dev_fw_ver->ref_clock);
                return false;
            }
            break;
            
        case 0x31010000:
            if(dev_fw_ver->ref_clock == ATH3K_XTAL_FREQ_40M) {
                if(_useLinuxFw)
                {
                    ramps_buf = ath3k_linux_ramps_0x31010000_40_buf;
                    ramps_buf_size = sizeof(ath3k_linux_ramps_0x31010000_40_buf);
                } else {
                    ramps_buf = ath3k_win_ramps_0x31010000_40_buf;
                    ramps_buf_size = sizeof(ath3k_win_ramps_0x31010000_40_buf);
                }
                
            }
            else {
                IOLog("%s :: Unknown ref clock 0x%x\n", getName(),dev_fw_ver->ref_clock);
                return false;
            }
            break;
            
        case 0x31010100: // Only Windows has these files
            if(dev_fw_ver->ref_clock == ATH3K_XTAL_FREQ_40M) {
                ramps_buf = ath3k_win_ramps_0x31010100_40_buf;
                ramps_buf_size = sizeof(ath3k_win_ramps_0x31010100_40_buf);
            }
            else {
                IOLog("%s :: Unknown ref clock 0x%x\n", getName(),dev_fw_ver->ref_clock);
                return false;
            }
            break;
            
        case 0x41020000:
            if(dev_fw_ver->ref_clock == ATH3K_XTAL_FREQ_40M) {
                if(_useLinuxFw)
                {
                    ramps_buf = ath3k_linux_ramps_0x41020000_40_buf;
                    ramps_buf_size = sizeof(ath3k_linux_ramps_0x41020000_40_buf);
                } else {
                    ramps_buf = ath3k_win_ramps_0x41020000_40_buf;
                    ramps_buf_size = sizeof(ath3k_win_ramps_0x41020000_40_buf);
                }
            }
            else {
                IOLog("%s :: Unknown ref clock 0x%x\n", getName(),dev_fw_ver->ref_clock);
                return false;
            }
            break;
            
        default:
            break;
    }
    
    DEBUG_BASIC_LOG("%s :: Sending sysconfig data to device (ramps buffer size %d bytes)...\n", getName(), ramps_buf_size);
    
    //
    // Send HCI command request to control endpoint to initiate the firmware transfer
    //
    
    if(sendHCICommandRequest(0x40, ATH3K_DNLOAD, ramps_buf, FW_HDR_SIZE, USB_CNTRL_REQ_TIMEOUT))
    {
        IOLog("%s :: Failed to initiate firmware transfer.\n", getName());
        return false;
    }
    
    ramps_buf += FW_HDR_SIZE;
    ramps_buf_size -= FW_HDR_SIZE;
    
    //
    // Lock device
    //
    
    IOLockLock(_mDeviceLock);
    
    //
    // Send sysconfig data to device
    //
    
    if(writeFirmwareToDevice(ramps_buf, ramps_buf_size)) {
        IOLog("%s :: Failed to load sysconfig into device.\n", getName());
        
        //
        // UnLock device
        //
        
        IOLockUnlock(_mDeviceLock);
        
        return false;
    }
    else {
        DEBUG_BASIC_LOG("%s :: Sysconfig loaded successfully into device.\n", getName());
    }
    
    //
    // UnLock device
    //
    
    IOLockUnlock(_mDeviceLock);
    
    
    //
    // Set device to normal mode
    //
    
    DEBUG_LOG("%s :: Setting device state to normal mode...\n", getName());
    
    if(!ath3012SetNormalMode())
        return false;
    
    //
    // Switch PID and VID
    //
    
    DEBUG_LOG("%s :: Switching device PID and VID...\n", getName());
    
    if(sendHCICommandRequest(kUSBVendor, USB_REG_SWITCH_VID_PID, NULL, 0, 1000))
    {
        DEBUG_LOG("%s :: Failed to switch PID and VID, but mostly its Fine.\n", getName());
    }
    else {
        DEBUG_LOG("%s :: Switched PID and VID successfully\n", getName());
    }
    
    return true;
}

bool BTFirmwareUploader::ath3012SetNormalMode()
{
    unsigned char dev_State = 0;
    
    //
    // Get the device state
    //
    
    if(sendHCICommandRequest(0xc0, USB_REQ_GET_STATE, &dev_State, 1, USB_CNTRL_REQ_TIMEOUT))
    {
        IOLog("%s :: Failed to get the device state.\n", getName());
        return false;
    }
    else {
        DEBUG_LOG("%s :: Device state 0x%x(%d)\n", getName(), dev_State, dev_State);
    }
    
    
    if((dev_State & ATH3K_MODE_MASK) == ATH3K_NORMAL_MODE) {
        DEBUG_LOG("%s :: Device is already in normal mode (%d)\n", getName(), dev_State);
        return true;
    }
    
    //
    // Send HCI command request to control endpoint to set device to normal model
    //
    
    if(sendHCICommandRequest(kUSBVendor, ATH3K_SET_NORMAL_MODE, NULL, 0, USB_CNTRL_REQ_TIMEOUT))
    {
        IOLog("%s :: Failed to set device to normal mode.\n", getName());
        return false;
    }
    else {
        DEBUG_LOG("%s :: Device set to normal mode\n", getName());
    }
    
    return true;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Function which process Broadcom firmware update
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void BTFirmwareUploader::bcmProcessFirmwareUpdate()
{
    //
    // Create IOBufferMemoryDescriptor for interrupted read
    //
    
    _readMemBufDesc = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, 0x104);
    
    if (!_readMemBufDesc) {
        IOLog("%s :: Failed to map read memory descriptor.\n", getName());
        return;
    }
    
    //
    // Prepare IOMemoryDescriptor for interrupted read
    //
    
    if (_readMemBufDesc->prepare()) {
        IOLog("%s :: Failed to prepare read memory descriptor.\n", getName());
        return;
    }
    
    gLastUpdateSuccess  = false;
    _updateSyncDelay    = 3;
    _uploadLoopCounts   = 0;
    
    //
    // Send HCI reset to start
    //
    
    DEBUG_LOG("%s :: Sending HCI Reset command request...\n", getName());
    
    if(sendHCICommandRequest(0x20, 0, &HCI_RESET, sizeof(HCI_RESET), USB_CNTRL_REQ_TIMEOUT))
        goto END;
    
    if(readHCIEvent())
        goto END;
    
    //
    // Send download command for initiating the upload
    //
    
    DEBUG_LOG("%s :: Sending BCM mini driver download HCI command request...\n", getName());
    
    if(sendHCICommandRequest(0x20, 0, &HCI_BCM_DOWNLOAD_MINI_DRIVER, sizeof(HCI_BCM_DOWNLOAD_MINI_DRIVER), USB_CNTRL_REQ_TIMEOUT))
        goto END;
    
    if(readHCIEvent())
        goto END;
    
    IOSleep(300); // wait
    _updateSyncDelay = 1;
    
    DEBUG_LOG("%s :: Sending firmware patch to the device...\n", getName());
    
    //
    // Parse the firmware from hex buffer and send it to the device
    //
    
    if (!bcmParseFirmwareAndUpload()) {
        goto END;
    }
    
    _updateSyncDelay = 3;
    
    //
    // Finish upload with end of record command
    //
    
    DEBUG_LOG("%s :: Sending BCM HEX TO HCD END Record HCI command request...\n", getName());
    
    if(sendHCICommandRequest(0x20, 0, &HCI_BCM_HEX_TO_HCD_END, sizeof(HCI_BCM_HEX_TO_HCD_END), USB_CNTRL_REQ_TIMEOUT))
        goto END;
    
    if(readHCIEvent())
        goto END;
    
    //
    // Send HCI reset to end
    //
    
    DEBUG_LOG("%s :: Sending HCI Reset command request...\n", getName());
    
    if(sendHCICommandRequest(0x20, 0, &HCI_RESET, sizeof(HCI_RESET), USB_CNTRL_REQ_TIMEOUT))
        goto END;
    
    IOLog("%s :: Successfully patched the firmware.\n", getName());
    
    gLastUpdateSuccess = true;
    _uploadLoopCounts = 0;
    
END:
    //
    // Complete IOMemoryDescriptor used for interrupted read
    //
    
    if (_readMemBufDesc->complete() != kIOReturnSuccess) {
        IOLog("%s :: Failed to complete read memory descriptor.\n", getName());
    }
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Functions which parses the firmware data from the buffer to HCD format and
// uploads to the device.
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

UInt32 BTFirmwareUploader::bcmGetFirmwareBuffer()
{
    unsigned int fw_verion = 14, build_ver = 0; // 14 for BCM20702A1, 9 for BCM4335
    
    //
    // Should use Device specific firmware ?
    //
    
    int len =0;
    
    if (kOSBooleanFalse == getProperty(DEVICE_SPEC_FW)) {
        
        if (_isStandaloneBT) {
            len = sizeof(usb_bt_dongle_firmware_buf);
            build_ver = 5860;
            
            for(int indx = 0; indx < len; indx++)
                bcm_fw_buf[indx] = usb_bt_dongle_firmware_buf[indx];
        }
        else {
            len = sizeof(zebra_4352_bt_firmware_buf);
            build_ver = 5876;
            
            for(int indx = 0; indx < len; indx++)
                bcm_fw_buf[indx] = zebra_4352_bt_firmware_buf[indx];
        }
        
        DEBUG_BASIC_LOG("%s :: Using non-device specific higher firmware version v%d c%d\n", getName(), fw_verion, build_ver);
        
    }
    else {
        
        //
        // Choose the firmware to load based on Vendor and Product ID values
        //
        
        switch (_vendorID) {
            case 0x13d3:
                switch (_productID)
            {
                case 0x3404:
                    IOLog("%s :: Device is Azurewave 4352HMB combo\n", getName());
                    
                    len = sizeof(azurewave_4352_bt_firmware_buf); // azurewave_4352_bt_firmware_buf
                    build_ver = 5575;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = azurewave_4352_bt_firmware_buf[indx];
                    break;
                    
                case 0x3411:
                    IOLog("%s :: Device is Dell Alienware 4352 combo\n", getName());
                    
                    len = sizeof(dell_alien_4352_bt_firmware_buf);
                    build_ver = 5546;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = dell_alien_4352_bt_firmware_buf[indx];
                    break;
                    
                case 0x3413:
                    IOLog("%s :: Device is Azurewave 4360 combo\n", getName());
                    
                    len = sizeof(azurewave_4360_bt_firmware_buf);
                    build_ver = 5577;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = azurewave_4360_bt_firmware_buf[indx];
                    break;
                    
                case 0x3418:
                    IOLog("%s :: Device is Azurewave 4352 combo\n", getName());
                    
                    len = sizeof(azurewave_4352_bt_firmware_buf_2);
                    build_ver = 5576;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = azurewave_4352_bt_firmware_buf_2[indx];
                    break;
                    
                case 0x3435:
                    IOLog("%s :: Device is Azurewave 4352 combo\n", getName());
                    
                    len = sizeof(azurewave_4352_bt_firmware_buf_3);
                    build_ver = 5597;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = azurewave_4352_bt_firmware_buf_3[indx];
                    break;
                    
                case 0x3456:
                    IOLog("%s :: Device is Azurewave 4352 combo\n", getName());
                    
                    len = sizeof(azurewave_4352_bt_firmware_buf_4);
                    build_ver = 5598;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = azurewave_4352_bt_firmware_buf_4[indx];
                    break;
            }
                break;
                
            case 0x0b05:
                switch (_productID)
            {
                case 0x17cf:
                    IOLog("%s :: Device is Asus 4352 combo\n", getName());
                    
                    len = sizeof(asus_4352_bt_firmware_buf);
                    build_ver = 5565;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = asus_4352_bt_firmware_buf[indx];
                    break;
                    
                case 0x17cb:
                    IOLog("%s :: Device is Asus BT-400 USB dongle\n", getName());
                    
                    len = sizeof(asus_bt_400_firmware_buf);
                    build_ver = 5563;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = asus_bt_400_firmware_buf[indx];
                    break;
                    
                case 0x180a: // Asus Rampage V extreme
                    IOLog("%s :: Device is Azurewave 4360 combo\n", getName());
                    
                    len = sizeof(azurewave_4360_bt_firmware_buf_2);
                    build_ver = 5707;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = azurewave_4360_bt_firmware_buf_2[indx];
                    break;
            }
                break;
                
            case 0x04ca:
                switch (_productID)
            {
                case 0x200a:
                    IOLog("%s :: Device is LiteOn 4352 combo\n", getName());
                    
                    len = sizeof(LiteOn_a_4352_bt_firmware_buf);
                    build_ver = 5588;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = LiteOn_a_4352_bt_firmware_buf[indx];
                    break;
                    
                case 0x200b:
                    IOLog("%s :: Device is LiteOn 4352 combo\n", getName());
                    
                    len = sizeof(LiteOn_b_4352_bt_firmware_buf);
                    build_ver = 5589;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = LiteOn_b_4352_bt_firmware_buf[indx];
                    break;
                    
                case 0x200c:
                    IOLog("%s :: Device is LiteOn 4352 combo\n", getName());
                    
                    len = sizeof(LiteOn_c_4352_bt_firmware_buf);
                    build_ver = 5590;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = LiteOn_c_4352_bt_firmware_buf[indx];
                    break;
                    
                case 0x200f:
                    IOLog("%s :: Device is Acer LiteOn 4352 combo\n", getName());
                    
                    len = sizeof(acer_liteOn_4352_bt_firmware_buf);
                    build_ver = 5617;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = acer_liteOn_4352_bt_firmware_buf[indx];
                    break;
            }
                break;
                
            case 0x0489:
                switch (_productID)
            {
                case 0xe07a:
                    IOLog("%s :: Device is Lenovo China 4352 combo NGFF\n", getName());
                    
                    len = sizeof(lenovo_4352_ngff_bt_firmware_buf);
                    build_ver = 5747;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = lenovo_4352_ngff_bt_firmware_buf[indx];
                    break;
                    
                case 0xe079: // BCM4335C0
                    IOLog("%s :: Device is Lenovo China 43162 NGFF\n", getName());
                    
                    len = sizeof(lenovo_43162_ngff_bt_firmware_buf);
                    build_ver = 115;
                    fw_verion = 9;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = lenovo_43162_ngff_bt_firmware_buf[indx];
                    break;
                    
                case 0xe032:
                    IOLog("%s :: Device is BCM4313 combo\n", getName());
                    
                    len = sizeof(bcm4313_combo_bt_firmware_buf);
                    build_ver = 5581;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = bcm4313_combo_bt_firmware_buf[indx];
                    break;
                    
                case 0xe0a1:
                    IOLog("%s :: Device is Lenovo 43602 combo NGFF\n", getName());
                    
                    len = sizeof(lenovo_43602_ngff_bt_firmware_buf);
                    build_ver = 414;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = lenovo_43602_ngff_bt_firmware_buf[indx];
                    break;
                    
            }
                break;
                
            case 0x0a5c:
                switch (_productID)
            {
                case 0x6417:
                    IOLog("%s :: Device is Zebra 4352 combo\n", getName());
                    
                    len = sizeof(zebra_4352_bt_firmware_buf);
                    build_ver = 5876;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = zebra_4352_bt_firmware_buf[indx];
                    break;
                    
                case 0x22be:
                    IOLog("%s :: Device is Azurewave 4352/4360 combo\n", getName());
                    
                    len = sizeof(azurewave_4360_bt_firmware_buf);
                    build_ver = 5577;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = azurewave_4360_bt_firmware_buf[indx];
                    break;
                    
                case 0x21fb:
                    IOLog("%s :: Device is HP Supra 4352 combo\n", getName());
            
                    len = sizeof(hp_4352_bt_firmware_buf);
                    build_ver = 5862;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = hp_4352_bt_firmware_buf[indx];
                    break;
                    
                case 0x21e8:
                    IOLog("%s :: Device is USB BT dongles like Delock, IOGEAR etc.,\n", getName());
                    
                    len = sizeof(usb_bt_dongle_firmware_buf);
                    build_ver = 5860;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = usb_bt_dongle_firmware_buf[indx];
                    break;
                    
                case 0x21e6:
                    IOLog("%s :: Device is Lenovo BT in thinkpad notebooks\n", getName());
                    
                    len = sizeof(lenovo_thinkpad_bt_firmware_buf);
                    build_ver = 5853;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = lenovo_thinkpad_bt_firmware_buf[indx];
                    break;
                    
                case 0x21e1:
                    IOLog("%s :: Device is HP SoftSailing BT in Elite notebooks\n", getName());
                    
                    len = sizeof(hp_elite_nb_bt_firmware_buf);
                    build_ver = 5866;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = hp_elite_nb_bt_firmware_buf[indx];
                    break;
                    
                case 0x21de:
                    IOLog("%s :: Device is Some 4352 combo\n", getName());
                    
                    len = sizeof(unknown_4352_bt_firmware_buf);
                    build_ver = 5557;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = unknown_4352_bt_firmware_buf[indx];
                    break;
                    
                case 0x21fd:
                    IOLog("%s :: Device is Generic 4352z combo\n", getName());
                    
                    len = sizeof(generic_4352z_bt_firmware_buf);
                    build_ver = 5559;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = generic_4352z_bt_firmware_buf[indx];
                    break;
                    
                case 0x216b:
                    IOLog("%s :: Device is HP Rapture 4352z ngff combo\n", getName());
                    
                    len = sizeof(hp_4352_ngff_bt_firmware_buf);
                    build_ver = 5864;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = hp_4352_ngff_bt_firmware_buf[indx];
                    break;
                    
                case 0x216e: // BCM4335C0
                    IOLog("%s :: Device is HP Blackbird 43162 ngff combo\n", getName());
                    
                    len = sizeof(hp_43162_ngff_bt_firmware_buf);
                    build_ver = 105;
                    fw_verion = 9;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = hp_43162_ngff_bt_firmware_buf[indx];
                    break;
                    
                case 0x216f:
                    IOLog("%s :: Device is Dell DW1560 4352 M.2\n", getName());
                    
                    len = sizeof(dell_4352_ngff_bt_firmware_buf);
                    build_ver = 5668;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = dell_4352_ngff_bt_firmware_buf[indx];
                    break;
                    
            }
                break;
                
            case 0x413c:
                switch (_productID)
            {
                case 0x8143:
                    IOLog("%s :: Device is Dell DW1550 4352 combo\n", getName());
                    
                    len = sizeof(dell_4352_bt_firmware_buf);
                    build_ver = 5545;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = dell_4352_bt_firmware_buf[indx];
                    break;
                    
                case 0x8197:
                    IOLog("%s :: Device is Dell DW380 Standalone BT\n", getName());
                    
                    len = sizeof(dell_dw380_std_bt_firmware_buf);
                    build_ver = 5543;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = dell_dw380_std_bt_firmware_buf[indx];
                    break;
            }
                break;
                
            case 0x0930:
                switch (_productID)
            {
                case 0x0223:
                    IOLog("%s :: Device is Toshiba 4352 combo NGFF\n", getName());
                    
                    len = sizeof(toshiba_4352_ngff_bt_firmware_buf);
                    build_ver = 5859;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = toshiba_4352_ngff_bt_firmware_buf[indx];
                    break;
                    
                case 0x0221:
                    IOLog("%s :: Device is Toshiba 4352 combo\n", getName());
                    
                    len = sizeof(toshiba_4352_bt_firmware_buf);
                    build_ver = 5858;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = toshiba_4352_bt_firmware_buf[indx];
                    break;
            }
                break;
                
            case 0x050d:
                switch (_productID)
            {
                case 0x065a:
                    IOLog("%s :: Device is Belkin USB BT dongle\n", getName());
                    
                    len = sizeof(belkin_bt_firmware_buf);
                    build_ver = 5578;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = belkin_bt_firmware_buf[indx];
                    break;
            }
                break;
                
            default:
                IOLog("%s :: Unsupported supported card found in plist, using default firmware\n", getName());
                
                if (_isStandaloneBT) {
                    len = sizeof(lenovo_thinkpad_bt_firmware_buf);
                    build_ver = 5765;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = lenovo_thinkpad_bt_firmware_buf[indx];
                }
                else {
                    len = sizeof(azurewave_4360_bt_firmware_buf_2);
                    build_ver = 5810;
                    
                    for(int indx = 0; indx < len; indx++)
                        bcm_fw_buf[indx] = azurewave_4360_bt_firmware_buf_2[indx];
                }
                break;
        }
        
        DEBUG_BASIC_LOG("%s :: Using device specific firmware v%d c%d\n", getName(), fw_verion, build_ver);
    }
    
    return len;
}

// Convert a byte to int
static unsigned int asc_to_int(char a)
{
    if (a >= 'A')
        return (a - 'A') + 10;
    else
        return a - '0';
}

// Convert 2 bytes of hex to int
static unsigned int hex_to_int(const char *h)
{
    return asc_to_int(*h) * 0x10 + asc_to_int(*(h + 1));
}

// Convert 4 bytes of hex to int
static unsigned int lhex_to_int(const char *h)
{
    return hex_to_int(h) * 0x100 + hex_to_int(h + 2);
}

bool BTFirmwareUploader::bcmParseFirmwareAndUpload()
{
    unsigned int addr = 0;
    unsigned int tmpIndex = 0, pBufferSize = 0;
    unsigned int length = 0, address = 0, checksum = 0;
    ssize_t bytesProcessed = 0, len = 0;
    
    //
    // Get the firmware buffer for the device
    //
    
    len = bcmGetFirmwareBuffer();
    
    len = len - 2; // decrement by 2 for ignoring 2 checksum bytes at the end
    _readComplete = true;
    
    DEBUG_BASIC_LOG("%s :: Started the upload of firmware (%ld bytes)...\n", getName(), len);
    
    //
    // Lock device
    //
    
    IOLockLock(_mDeviceLock);
    
    //
    // Convert hex data to hcd and upload it to the device
    //
    
    while (len > 0) {
        int type;
        unsigned int dest_addr;
        
        char addressBuf[7], tempBuf[1];
        char dataBuf[1024] = {NULL};
        unsigned char parsedFwBuffer[1024] = {NULL};;
        
        //
        // If the first byte is not BCM_FW_PREFIX then increment array until we get that.
        //
        
        if(bcm_fw_buf[bytesProcessed] != BCM_FW_PREFIX)
        {
            bytesProcessed++;
            len--;
            continue;
        }
        
        //
        //  BCM_FW_PREFIX is found then parse the buffer for the data.
        //
        
        length = hex_to_int(&bcm_fw_buf[bytesProcessed + 1]);
        address = lhex_to_int(&bcm_fw_buf[bytesProcessed + 3]);
        type = hex_to_int(bcm_fw_buf + 7 + bytesProcessed);
        len--;
        
        tmpIndex = 0;
        pBufferSize = 0;
        
        switch (type) {
            case 4:
                addr = lhex_to_int(bcm_fw_buf + 9 + bytesProcessed) * 0x10000;
                
                // Increment pointer until we get the terminators 0D = \r and 0A = \n
                while(bcm_fw_buf[bytesProcessed] != 0x0D && bcm_fw_buf[bytesProcessed] != 0x0A)
                {
                    bytesProcessed++;
                    len--;
                }
                break;
                
            case 0:
                // Calculate the destination address (byte 3 to byte 7)
                dest_addr = addr + lhex_to_int(bcm_fw_buf + 3 + bytesProcessed);
                
                // Increment pointer by 1 to skip the prefix ":" from firmware buffer
                bytesProcessed++;
                
                // Extract and fill the data buffer by incrementing pointer until we get the terminators
                // 0D = \r and 0A = \n from firmware buffer
                while(bcm_fw_buf[bytesProcessed] != 0x0D && bcm_fw_buf[bytesProcessed] != 0x0A)
                {
                    dataBuf[tmpIndex] = bcm_fw_buf[bytesProcessed];
                    bytesProcessed++;
                    len--;
                    tmpIndex++;
                }
                
                addressBuf[0] = 0x4c;
                addressBuf[1] = 0xfc;
                addressBuf[2] = hex_to_int(dataBuf) + 4;
                addressBuf[3] = dest_addr;
                addressBuf[4] = dest_addr >> 8;
                addressBuf[5] = dest_addr >> 16;
                addressBuf[6] = dest_addr >> 24;
                
                // Calculate checksum
                checksum = hex_to_int(&bcm_fw_buf[bytesProcessed]);
                
                // Copy parsed address to buffer
                tmpIndex = 0;
                while(tmpIndex < 7)
                {
                    parsedFwBuffer[pBufferSize] = addressBuf[tmpIndex];
                    tmpIndex++;
                    pBufferSize++;
                }
                
                // Copy parsed data to buffer
                for (tmpIndex = 0; tmpIndex < hex_to_int(dataBuf); tmpIndex++)
                {
                    //
                    // Data is available from byte 9 (To get that from the buffer add count 8)
                    // because from buffer first 8 bytes are for the number of address and length bits.
                    //
                    
                    tempBuf[0] = hex_to_int(dataBuf + 8 + tmpIndex * 2);
                    parsedFwBuffer[pBufferSize] = tempBuf[0];
                    
                    pBufferSize++;
                }
                
                //
                // Write the fw data to device
                //
                
                if (writeFirmwareToDevice(parsedFwBuffer, pBufferSize))
                {
                    //
                    // Lock device
                    //
                    
                    IOLockUnlock(_mDeviceLock);
                    
                    return false;
                }
                else
                {
                    _uploadLoopCounts++;
                    
                    // Try to read
                    _readComplete = false;
                    _readEventTimer->setTimeoutMS(0);
                    
                    int loop = 1000; // 1000 * _updateSyncDelay = xxx? ms
                    
                    // Wait for read to complete
                    while(!_readComplete && loop > 0) {
                        IOSleep(_updateSyncDelay);
                        loop--;
                    }
                    
                    //
                    // If loop == 0 which is timeout for read, so finish and stop.
                    //
                    
                    if(loop == 0)
                    {
                        IOLog("%s :: Read event failed due to timeout (Transfers %d).\n", getName(), _uploadLoopCounts);
                        
#if !TARGET_MAC_OSX_10_11
                        IOReturn retAbrt = _USBInPipe->Abort();
                        // IOReturn retRst  = _USBInPipe->Reset(); // Additional clear
                        IOLog("%s :: Aborting pending IO requests, Response -> %d.\n", getName(), retAbrt);
#else
                        IOOptionBits options = IOUSBHostIOSource::kAbortSynchronous;
                        IOReturn retAbrt = _USBInPipe->abort(options);
                        IOLog("%s :: Aborting pending IO requests, Response -> %d.\n", getName(), retAbrt);
#endif

                        //
                        // UnLock device
                        //
                        
                        IOLockUnlock(_mDeviceLock);
                        
                        return false;
                    }
                    else
                        DEBUG_LOG("%s :: Read event finished after %d ms\n", getName(), (50 - loop) * _updateSyncDelay);
                }
                break;
                
            case 1: // Terminator
                
                // Increment pointer until we get the terminators 0D = \r and 0A = \n
                while(bcm_fw_buf[bytesProcessed] != 0x0D && bcm_fw_buf[bytesProcessed] != 0x0A)
                {
                    bytesProcessed++;
                    len--;
                }
                break;
                
            default:
                break;
        }
        
        // If the terminator we found was "\n" then increment 2 bytes to skip the "\r" and "\n"
        if (bcm_fw_buf[bytesProcessed] == 0x0D)
            bytesProcessed = bytesProcessed + 2;
        
        // If the terminator we found was "\n" then increment a byte to skip the "\n"
        else
            bytesProcessed++;
    }
    
    
    //
    // UnLock device
    //
    
    IOLockUnlock(_mDeviceLock);
    
    gUploadCount++;
    
    return true;
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Function which writes firmware into the device
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

IOReturn BTFirmwareUploader::writeFirmwareToDevice(unsigned char* fwBuffer, UInt32 buffer_size)
{
    IOReturn ret;
    int transferred_size = 1, send_size;
    
    if (!fwBuffer) {
        IOLog("%s :: Firmware buffer is empty.\n", getName());
        return kIOReturnError;
    }
    
    //
    // Create and prepare IOMemoryDescriptor for bulk transfer
    //
    
    IOMemoryDescriptor* writeMemDesc = IOMemoryDescriptor::withAddress(&_writeBuffer, BULK_SIZE, kIODirectionNone);
    
    if (!writeMemDesc) {
        IOLog("%s :: Failed to map memory descriptor\n", getName());
        return kIOReturnNoMemory;
    }
    
    ret = writeMemDesc->prepare();
    if (ret) {
        IOLog("%s :: Failed to prepare memory descriptor, error (0x%x)\n", getName(), ret);
        return ret;
    }
    
    //
    // Write the firmware to pipe
    //
    
    while (buffer_size) {
        send_size = buffer_size < BULK_SIZE ? buffer_size : BULK_SIZE;
        
        memcpy(_writeBuffer, fwBuffer, send_size); // Copy BULK_SIZE data to write buffer for sending

#if !TARGET_MAC_OSX_10_11
        ret = _USBOutPipe->Write(writeMemDesc, 3000, 3000, send_size);
#else
        ret = _USBOutPipe->io(writeMemDesc, send_size, NULL);
#endif
        if (ret) {
            IOLog("%s :: Failed to write firmware to bulk pipe with error (0x%x) after %d bytes transfer, Data left %d bytes\n", getName(), ret, transferred_size, buffer_size);
            break;
        }
        
        fwBuffer += send_size;
        buffer_size -= send_size;
        transferred_size += send_size;
    }
    
    //
    // Complete IOMemoryDescriptor used for bulk transfer
    //
    
    if (writeMemDesc->complete() != kIOReturnSuccess) {
        IOLog("%s :: Failed to complete memory descriptor\n", getName());
        ret = kIOReturnError;
    }
    
    //
    // Release buffer
    //
    
    if (writeMemDesc)
        writeMemDesc->release();
    
    return ret;
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Function which sends the HCI command request to the device.
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

IOReturn BTFirmwareUploader::sendHCICommandRequest(UInt8 reqType, UInt8 request, void* data, UInt16 length, UInt16 timeout)
{
    //
    // Lock device
    //
    
    IOLockLock(_mDeviceLock);
    
#if !TARGET_MAC_OSX_10_11
    IOUSBDevRequest ctlreq;
#else
    DeviceRequest ctlreq;
#endif
    
    IOReturn  ret;
    
    //
    // Send HCI command request to the USB control endpoint
    //
    
    ctlreq.bmRequestType = reqType;
    ctlreq.bRequest = request;
    ctlreq.wValue = 0;
    ctlreq.wIndex = 0;
    ctlreq.wLength = length;
    
#if !TARGET_MAC_OSX_10_11
    ctlreq.pData = data;
    ret = _USBDev->DeviceRequest(&ctlreq, timeout, timeout);
#else
    UInt32 bytesTransferred = 0;

    ret = _USBDev->deviceRequest(this, ctlreq, data, bytesTransferred);
#endif

    if (ret) {
        IOLog("%s :: HCI command request failed with error (0x%x).\n", getName(), ret);
    }
    else
    {
#if !TARGET_MAC_OSX_10_11
        DEBUG_LOG("%s :: HCI command request success\n", getName());
#else
        DEBUG_LOG("%s :: HCI command request success, Bytes transferred %d\n", getName(), bytesTransferred);
#endif
    }
    
    //
    // UnLock device
    //
    
    IOLockUnlock(_mDeviceLock);
    
    return ret;
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Functions which reads interrupt pipe from device to synchronize fw upload
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

IOReturn BTFirmwareUploader::readHCIEvent()
{
    //
    // Wait for some time after write before attempting to read the pipe.
    //
    
    IOSleep(_updateSyncDelay);
    
    //
    // Read Interrupt Interrupt pipe
    //
    
    /*
#if !TARGET_MAC_OSX_10_11
    IOUSBCompletion usbCompletionAction =
    {
        .target = this,
        .action = readHCIEventEndAction,
        .parameter = _readMemBufDesc
    };
#else
    IOUSBHostCompletion usbCompletionAction =
    {
        .owner = this,
        .action = readHCIEventEndAction,
        .parameter = _readMemBufDesc
    };
#endif
    */
    
    IOReturn ret = kIOReturnSuccess;
    
    //
    // Using interrupt handler process creating some problems especially after sleep in my testing,
    // so instead using NULL for completion action to avoid. Also we dont need worry about parsing
    // the data we get from read as there is nothing to consider for our update.
    //
    
#if !TARGET_MAC_OSX_10_11
    IOByteCount bytesRead = 0;

    UInt32 pipeStatus = _USBInPipe->GetPipeStatus();
    
    if (pipeStatus == kIOReturnSuccess)
    {
        // ret = _USBInPipe->Read(_readMemBufDesc, 0, 0, 0x104, &usbCompletionAction, &bytesRead);
        ret = _USBInPipe->Read(_readMemBufDesc, 0, 0, 0x104, (IOUSBCompletion*)NULL, &bytesRead);
        // ret = _USBInPipe->Read(_readMemBufDesc);
        
        switch(ret) {
            case kIOReturnOverrun:
                IOLog("%s :: Read returned kIOReturnOverrun, clearing stall.\n", getName());
                _USBInPipe->ClearPipeStall(false);
                break;
                
            case kIOReturnSuccess:
                _readComplete = true;
                DEBUG_LOG("%s :: Read Success with %lld bytes.\n", getName(), bytesRead);
                break;
                
            case kIOReturnNotResponding:
                IOLog("%s :: Read failed with response kIOReturnNotResponding.\n", getName());
                break;
                
            default:
                IOLog("%s :: Read failed with error %x.\n", getName(), ret);
                break;
        }
    }
    else if (pipeStatus == kIOUSBPipeStalled) {
        IOLog("%s :: Pipe stalled, clearing...\n", getName());
        _USBInPipe->ClearPipeStall(false);
    }
#else
    UInt32 bytesRead = 0;

    UInt8 ioStatus = getState();
    
    if (ioStatus == IOUSBHostIOSource::kStateReady) {
        DEBUG_LOG("%s :: IO is ready..\n", getName());
        
        ret = _USBInPipe->io(_readMemBufDesc, 0x104, bytesRead, 0); // Read(_readMemBufDesc, 0, 0, 0x104, (IOUSBHostCompletion*)NULL, &bytesRead);
        
        switch(ret) {
            case kIOReturnOverrun:
                IOLog("%s :: Read returned kIOReturnOverrun, clearing stall.\n", getName());
                _USBInPipe->clearStall(false);
                break;
                
            case kIOReturnSuccess:
                _readComplete = true;
                DEBUG_LOG("%s :: Read Success with %lld bytes.\n", getName(), bytesRead);
                break;
                
            case kIOReturnNotResponding:
                IOLog("%s :: Read failed with response kIOReturnNotResponding.\n", getName());
                break;
                
            default:
                IOLog("%s :: Read failed with error %x.\n", getName(), ret);
                break;
        }
    }
    else {
        IOLog("%s :: IO is not ready, Status %d..\n", getName(), ioStatus);
    }
#endif
    
    return ret;
}

/*
void BTFirmwareUploader::readHCIEventEndAction(void* target, void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    if (target)
        ((BTFirmwareUploader*)target)->readHCIEventInterruptHandler(parameter, status, bufferSizeRemaining);
}

void BTFirmwareUploader::readHCIEventInterruptHandler(void* parameter, IOReturn status, UInt32 bufferSizeRemaining)
{
    IOLog("%s :: Reading interrupt pipe finished with Status (0x%x), BufferSizeRemaining %d\n", getName(), status, bufferSizeRemaining);
    
    switch(status) {
        case kIOReturnOverrun:
            IOLog("%s :: Device read returned kIOReturnOverrun, clearing stall\n", getName());
            if (_USBInPipe != NULL) {
#if !TARGET_MAC_OSX_10_11
                _USBInPipe->ClearPipeStall(false);
#else
                _USBInPipe->clearStall(false);
#endif
            }
            break;
          
        case kIOReturnNotResponding:
            IOLog("%s :: Device read returned kIOReturnNotResponding\n", getName());
            break;
            
        default:
            break;
            
        case kIOReturnSuccess:
            IOBufferMemoryDescriptor* readBuffer = (IOBufferMemoryDescriptor*)parameter;
            if (!readBuffer)
            {
                IOLog("%s :: Read buffer is Empty.\n", getName());
                return;
            }
            DEBUG_LOG("%s :: Device read is Success\n", getName());
            break;
    }
    
    _readComplete = true;
}
 */

