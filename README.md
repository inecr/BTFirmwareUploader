# BTFirmwareUploader

Little background information about firmware patch from linux:
After hardware reset, some BCM and Atheros Bluetooth adapters obtain their initial firmware from OTPROM chip. Although some of the devices have an OTPROM that contains required firmware, but it is found that these devices would not have HFP/HSP support and other things unless a upgraded firmware is loaded.
Once this initial firmware is running, the firmware can be further upgraded over HCI interface with the files provided by Broadcom and Atheros.

I've developed this kext to update the firmware needed for those Atheros AR3012 and Broadcom Bluetooth devices to work with all functionalities that hardware supposed to do. Also integrated support for the Atheros bluetooth AR3011 (from old kext by mac4mat) with some improvements/fixes in my kext to make a single solution for bluetooth.

Following are the Bluetooth cards supported by my kext:
AR3011:
Atheros AR5B195 Combo (AR9285 Wifi + AR3011 Bluetooth) 
Atheros AR5B197 Combo (AR9287 Wifi + AR3011 Bluetooth)
Other Atheros AR3011 cards with ids list below
 
AR3012:
Atheros WB225 Combo (AR9485 Wifi(doesn't work) + AR3012 Bluetooth)
Other Atheros AR3012 cards with ids list below

BROADCOM WiFi/BT Combo cards Bluetooth:

Broadcom BCM4352 Combo
Broadcom BCM4360 Combo
Broadcom BCM43162 Combo
Broadcom BCM4313 Combo (Not all cards need Update)

 
BROADCOM USB/Integrated Bluetooth:

Asus BT-400 USB Bluetooth
Belkin 20702 USB Bluetooth
USB Dongles with 20702A1 Bluetooth (Delock, IOGear etc.,)
HP BRCM1043 Integrated Bluetooth (found in HP Elite Notebooks)

For more details check GUIDEME.md

## License

This project is licensed under the GNU General Public License v2.0 (GPL-2.0).

---

## Credits

This project is based on the Linux `ath3k` Bluetooth firmware loader.

Original work and concepts by Atheros Communications and Linux kernel contributors.

---

## Dependencies

This project uses Apple's IOKit framework provided by macOS.

IOKit is part of the macOS operating system and is not distributed with this project.

---

## Firmware Notice

Firmware files included in or used by this project are provided by Qualcomm Atheros and may be subject to their respective licenses.

This project does not claim ownership of any firmware binaries.

---

## Disclaimer

This project is an independent work and is not affiliated with or endorsed by Apple or Qualcomm Atheros.

All trademarks and product names are the property of their respective owners.

---

## Status

Development is complete and the project is stable. No active development is currently planned.
