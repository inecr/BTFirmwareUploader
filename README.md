# BTFirmwareUploader

macOS kernel extension (kext) for uploading **Bluetooth firmware** to supported devices in Hackintosh environments.

---

## Overview

BTFirmwareUploader enables Bluetooth functionality on unsupported hardware by loading required firmware into compatible chipsets at runtime.

Some Broadcom and Atheros Bluetooth adapters rely on initial firmware from OTPROM, which may lack full feature support (e.g., HFP/HSP). This kext uploads updated firmware over the HCI interface to unlock full device capabilities.

---

## Features

- Runtime Bluetooth firmware upload  
- Support for Atheros and Broadcom chipsets  
- Enables full Bluetooth functionality (HFP/HSP, etc.)  
- Unified support for multiple device families  

> 📘 For detailed configuration, usage, and device-specific notes, see **GUIDEME.md**

---

## Supported Hardware

### Atheros

**AR3011**
- AR5B195 (AR9285 WiFi + AR3011 Bluetooth)  
- AR5B197 (AR9287 WiFi + AR3011 Bluetooth)  
- Other AR3011-based devices  

**AR3012**
- WB225 (AR9485 WiFi + AR3012 Bluetooth)  
- Other AR3012-based devices  

---

### Broadcom (Combo Cards)

- BCM4352  
- BCM4360  
- BCM43162  
- BCM4313 *(some models may not require firmware upload)*  

---

### Broadcom (USB / Integrated)

- ASUS BT-400 USB  
- Belkin 20702 USB  
- USB dongles with BCM20702A1 (Delock, IOGear, etc.)  
- HP BRCM1043 (EliteBook series)  

---

## Technical Details

- Language: C++  
- Framework: macOS IOKit  
- Interfaces: USB / Bluetooth (HCI)  
- Type: Kernel Extension (kext)  
- Function: Firmware upload during device initialization  
- Environment: Hackintosh (OpenCore / Clover)  

---

## Installation

1. Copy `BTFirmwareUploader.kext` to:
   - `/Library/Extensions/`  
   or  
   - EFI (`Clover` / `OpenCore`)

2. Rebuild cache: `sudo kextcache -i /`  
3. Reboot  

---

## Status

Stable. No active development planned.

---

## License

This project is licensed under the **GNU General Public License v2.0 (GPL-2.0)**.

---

## Credits

- Based on the Linux **ath3k Bluetooth firmware loader**  
- Original concepts by Atheros Communications and Linux kernel contributors  

---

## Dependencies

- Apple macOS **IOKit framework** (system-provided)

---

## Firmware Notice

Firmware files used by this project are provided by Qualcomm Atheros and Broadcom and may be subject to their respective licenses.

This project does not claim ownership of any firmware binaries.

---

## Disclaimer

This project is an independent work and is not affiliated with or endorsed by Apple, Qualcomm Atheros, or Broadcom.

All trademarks and product names are the property of their respective owners.
