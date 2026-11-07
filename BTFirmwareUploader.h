#ifndef __BTFirmwareUploader__
#define __BTFirmwareUploader__

#include <IOKit/IOLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/usb/USBSpec.h>

#define TARGET_MAC_OSX_10_11 0

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
 * Atheros AR3012 Bluetooth
 */

#define BULK_SIZE	1024
#define FW_HDR_SIZE 20

// AR3012 definitions
#define USB_REQ_GET_STATE			0x05
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

class BTFirmwareUploader : public IOService
{
    OSDeclareDefaultStructors(BTFirmwareUploader)
    
protected:
#if APPLESDK
    AbsoluteTime    _now;
#else
    uint64_t    _now;
#endif
    
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
    
    char    _writeBuffer[BULK_SIZE];
    UInt8   _osxVersion;
    
    bool    _useCtrlPipe;
    bool    _useLinuxFw;
    bool    _serviceClosed;
    bool    _abortOnFail;
    
private:
    bool ReadPlistConfig();
    
    // AR3012 methods
    bool processAth3kFwUpdate();
    bool ath3012PatchAndSysConfig(bool patchFw);
    bool ath3012SetNormalMode();
    
    // USB device interactions
    IOReturn    initializeUSBDevice();
    IOReturn    findInterface();
    IOReturn    findPipes();
    IOReturn    writeFirmwareToDevice(unsigned char* fwBuffer, UInt32 buffer_size);
    IOReturn    sendHCICommandRequest(UInt8 reqType, UInt8 request, void* data, UInt16 length, UInt16 timeout);
    
#if TARGET_MAC_OSX_10_11
    const EndpointDescriptor*  findEndPointDescriptor(UInt8 etype, UInt8 eDirection);
#endif
    
    void detectOSXVersion();
    
public:
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
