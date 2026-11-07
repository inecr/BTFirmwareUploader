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
    
    _serviceClosed   = false;
    _abortOnFail     = false;
    
    _productID  = 0;
    _vendorID   = 0;
    _useLinuxFw = true;
    _useCtrlPipe = false;
    
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
    if(vID) {
        _vendorID = vID->unsigned32BitValue();
    }
    
    // Strict match for 13d3:3362 (Vendor 0x13d3 = 5075, Product 0x3362 = 13154)
    if (_vendorID != 0x13d3 || _productID != 0x3362) {
        DEBUG_LOG("%s :: Unsupported device 0x%04x:0x%04x, ignoring.\n", getName(), _vendorID, _productID);
        return false;
    }
    
    DEBUG_BASIC_LOG("%s :: Target AR3012 (13d3:3362) detected\n", getName());
    
    
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
    // Supporting OS X 10.9 (Mavericks) and 10.10 (Yosemite)
    if (_osxVersion > 10) {
        DEBUG_LOG("%s :: Warning: running on OS X > 10 in legacy USB mode.\n", getName());
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
    // Find the interface and pipes for bulk endpoint transfers
    //
    
    if (findInterface()) {
        return false;
    }
    
    if (findPipes()) {
        return false;
    }
    
    //
    // Process Atheros AR3012 bluetooth
    //
    if (!processAth3kFwUpdate()) {
        return false;
    }
    
    IOSleep(100); // wait to sync
    
    
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
    OSString *btCntrl = OSDynamicCast(OSString, getProperty("BTController"));
    
    if(btCntrl == NULL || strncmp(btCntrl->getCStringNoCopy(), BTUSB_ATH3012, strlen(BTUSB_ATH3012))) {
        IOLog("%s :: Only AR3012 controller is supported.\n", getName());
        return false;
    }
    
    DEBUG_BASIC_LOG("%s :: Atheros AR3012 Bluetooth detected\n", getName());
    
    if (kOSBooleanTrue == getProperty("Use Linux fw")) {
        _useLinuxFw = true;
    }
    else {
        _useLinuxFw = false;
    }
    
    DEBUG_BASIC_LOG("%s :: Using %s firmware\n", getName(), _useLinuxFw?"Linux":"Windows");
    
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
        DEBUG_LOG("%s :: Using Pipe %d for OUT\n", getName(), _wOutPipe);
    }
    
    _useCtrlPipe = false;
    _abortOnFail = false;
    
    return true;
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Functions to interact with USB device (send control requests and write firmware)
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

IOReturn BTFirmwareUploader::sendHCICommandRequest(UInt8 reqType, UInt8 request, void* data, UInt16 length, UInt16 timeout)
{
    IOReturn ret = kIOReturnSuccess;
    
#if !TARGET_MAC_OSX_10_11
    IOUSBDevRequest devReq;
    devReq.bmRequestType = reqType;
    devReq.bRequest = request;
    devReq.wValue = 0;
    devReq.wIndex = 0;
    devReq.wLength = length;
    devReq.pData = data;
    
    ret = _USBDev->DeviceRequest(&devReq);
#else
    ret = _USBDev->deviceRequest(this, reqType, request, 0, 0, length, data, timeout);
#endif
    
    return ret;
}

IOReturn BTFirmwareUploader::writeFirmwareToDevice(unsigned char* fwBuffer, UInt32 buffer_size)
{
    IOReturn ret = kIOReturnSuccess;
    UInt32 sent = 0;
    UInt32 remaining = buffer_size;
    
    if (!_USBOutPipe) {
        IOLog("%s :: Bulk OUT pipe is null.\n", getName());
        return kIOReturnNotReady;
    }
    
    while (remaining > 0) {
        UInt32 chunkSize = (remaining < BULK_SIZE) ? remaining : BULK_SIZE;
        
#if !TARGET_MAC_OSX_10_11
        IOMemoryDescriptor *memDesc = IOMemoryDescriptor::withAddress((void*)(fwBuffer + sent), chunkSize, kIODirectionOut);
        if (memDesc) {
            memDesc->prepare(kIODirectionOut);
            ret = _USBOutPipe->Write(memDesc);
            memDesc->complete(kIODirectionOut);
            memDesc->release();
        } else {
            ret = kIOReturnNoMemory;
        }
#else
        uint64_t bytesWritten = 0;
        ret = _USBOutPipe->io((void*)(fwBuffer + sent), chunkSize, bytesWritten, 5000);
#endif
        if (ret != kIOReturnSuccess) {
            IOLog("%s :: Failed to write firmware chunk at offset %u (err: 0x%08x)\n", getName(), sent, ret);
            break;
        }
        sent += chunkSize;
        remaining -= chunkSize;
    }
    
    return ret;
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
    // Do NOT issue USB ResetDevice() on initialization for AR3012, as it disrupts
    // ACPI / USB root hub power state on laptops like ASUS X550.
    
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
        else
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
        
        DEBUG_LOG("%s :: Pipe IN is not needed for AR3012\n", getName());
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
    // AR3012 Process
    //
    if (state & ATH3K_SYSCFG_UPDATE) {
        DEBUG_BASIC_LOG("%s :: Updated firmware already available in RAM, aborting upload.\n", getName());
        return false;
    }
    else if (state & ATH3K_PATCH_UPDATE) {
        DEBUG_BASIC_LOG("%s :: Firmware patch was already applied, just loading sysconfig...\n", getName());
        
        if(!ath3012PatchAndSysConfig(false))
        {
            IOLog("%s :: Failed to load sysconfig.\n", getName());
            success = false;
        }
        else {
            gUploadCount++;
        }
    }
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
    unsigned char fw_state = 0;
    
    if (sendHCICommandRequest(0xc0, ATH3K_GETSTATE, &fw_state, 1, USB_CNTRL_REQ_TIMEOUT)) {
        DEBUG_LOG("%s :: Can't get state to change to normal mode\n", getName());
        return false;
    }
    
    if ((fw_state & ATH3K_MODE_MASK) == ATH3K_NORMAL_MODE) {
        DEBUG_LOG("%s :: Firmware already initialized in normal mode\n", getName());
        return true;
    }
    
    if (sendHCICommandRequest(0x40, ATH3K_SET_NORMAL_MODE, NULL, 0, USB_CNTRL_REQ_TIMEOUT)) {
        DEBUG_LOG("%s :: Can't set normal mode\n", getName());
        return false;
    }
    
    return true;
}
