Little background information about firmware patch from linux:\
 

After hardware reset, some BCM and Atheros Bluetooth adapters obtain
their initial firmware from OTPROM chip. Although some of the devices
have an OTPROM that contains required firmware, but it is found that
these devices would not have HFP/HSP support and other things unless a
upgraded firmware is loaded.

Once this initial firmware is running, the firmware can be further
upgraded over HCI interface with the files provided by Broadcom and
Atheros.

  \
I\'ve developed this kext to update the firmware needed for those
Atheros AR3012 and Broadcom Bluetooth devices to work with all
functionalities that hardware supposed to do. Also integrated support
for the Atheros bluetooth AR3011 (from old kext by mac4mat) with some
improvements/fixes in my kext to make a single solution for bluetooth.\
 \
 

**[BUG REPORT (HAVE A PROBLEM?):]{.underline}**

Send me your system log file from /var/log, ioreg file saved using
IORegistryExplorer app v2.1 and Sys info saved from the app System
Information (found in Utilities) in File menu.

**NOTE: **

My kext is only meant to patch the firmware of the bluetooth devices,
but the [performance of the bluetooth]{.underline} mostly depends
on [Apple bluetooth drivers]{.underline}. However, i\'ve provided plist
option to choose different firmwares in order to test for any
improvements.   And Atheros Bluetooth uses Apple Generic bluetooth
drivers, so you will not get the option for turning bluetooth on/off and
you can only power on/off using Fn key if exists (which works in some
machines like Asus).\
  **If you have any pairing or connect issues then try to delete the
file *\"com.apple.Bluetooth.plist\"* from */Library/Preferences* and
reboot. Also try to remove Bluetooth PAN from network prefpane
interfaces list and see.**

  **To receive files from other bluetooth devices (like Mobiles), enable
the option \"Bluetooth Sharing\" in System Preferences-\>Sharing.**

**Want to know why you can\'t pair iPhone?** [[Read
here](https://discussions.apple.com/docs/DOC-7722)]{.underline}

**Your Combo card WiFi hardware is detected and working, but Bluetooth
hardware was not detected in ioreg and not working ? **[[Read this
post](https://osxlatitude.com/index.php?/topic/2925-bluetooth-firmware-uploader/page-31&do=findComment&comment=44897)]{.underline} (Credits:
tomwho)

**[Following are the Bluetooth cards supported by my
kext:]{.underline}**

**[AR3011:]{.underline}**

*Atheros AR5B195 Combo (AR9285 Wifi + AR3011 Bluetooth) *\
*Atheros AR5B197 Combo (AR9287 Wifi + AR3011 Bluetooth)*\
*Other Atheros AR3011 cards with ids list below*

**[AR3012:]{.underline}**

*Atheros WB225 Combo (AR9485 Wifi(doesn\'t work) + AR3012 Bluetooth)*\
*Other Atheros AR3012 cards with ids list below*

**Atheros AR3011 30 sec delay fix (or) after sleep issues patch: (Should
be working in 10.9.1+)**

**Enter the following two commands in terminal and reboot:**

    sudo perl -pi -e 's|\x0F\x85\x07\x01|\x0F\x84\x07\x01|g' /System/Library/Extensions/IOBluetoothFamily.kext/Contents/PlugIns/IOBluetoothHostControllerUSBTransport.kext/Contents/MacOS/IOBluetoothHostControllerUSBTransport
    sudo touch /System/Library/Extensions/

**For Clover, add this to kext patches instead:**

    Name: IOBluetoothHostControllerUSBTransport
    Find: 0F850701
    Replace: 0F840701
    Comment: Fix Bluetooth 30 seconds delay patch

Credits to TimeWalker for the patch idea.

**[NOTE:]{.underline}**

**BCM4352 WiFi 5Ghz access is now working (Credit goes to SKVO
fromapplife.ru. For complete details on making this WiFI card work,
follow this thread:** [[BCM4352
WiFi](https://osxlatitude.com/index.php?/topic/1916-the-perfect-mini-pcie-wifi-and-bluetooth-combo-card-for-notebooks/)]{.underline}

***[BROADCOM WiFi/BT Combo cards Bluetooth:]{.underline}***

Broadcom BCM4352 Combo\
Broadcom BCM4360 Combo\
Broadcom BCM43162 Combo\
Broadcom BCM4313 Combo (Not all cards need Update)\
 

***[BROADCOM USB/Integrated Bluetooth:]{.underline}***

Asus BT-400 USB Bluetooth\
Belkin 20702 USB Bluetooth\
USB Dongles with 20702A1 Bluetooth (Delock, IOGear etc.,)\
HP BRCM1043 Integrated Bluetooth (found in HP Elite Notebooks)\
 

**[Supported Bluetooth Vendor and Product ID: (Few device ids
below]{.underline}[ ]{.underline}[work only from v2.8)]{.underline}**

Broadcom:

*VID: 13d3  PID: 3404 (Azurewave 4352)*\
*VID: 13d3  PID: 3413 (Azurewave 4360)*\
*VID: 13d3  PID: 3418 (Azurewave 4352)*\
*VID: 13d3  PID: 3435 (Azurewave 4352)*\
*VID: 13d3  PID: 3456 (Azurewave 4352)*

*VID: 0b05  PID: 17cb (Asus USB)*\
*VID: 0b05  PID: 17cf  (Asus 4352)*\
*VID: 0b05  PID: 180a  (Asus 4360)*

*VID: 050d  PID: 065a (Belkin) *

*VID: 13d3  PID: 3411 (Dell Alienware 4352)*\
*VID: 413c  PID: 8143 (Dell DW1550 4352)*\
*VID: 0a5c  PID: 216f (Dell DW1560 4352 NGFF)*

*VID: 0a5c  PID: 21e1 (HP Elite NB)*\
*VID: 0a5c  PID: 21fb (HP 4352)*\
*VID: 0a5c  PID: 216b (HP 4352 NGFF)*\
*VID: 0a5c  PID: 216e (HP BCM43162 NGFF)*\
*VID: 0a5c  PID: 21fd  (Generic 4352)*\
*VID: 0a5c  PID: 21de (4352)*\
*VID: 0a5c  PID: 22be (4352/4360)*

*VID: 04ca  PID: 200a (LiteOn 4352)*\
*VID: 04ca  PID: 200b (LiteOn 4352)*\
*VID: 04ca  PID: 200c (LiteOn 4352)*\
*VID: 04ca  PID: 200f (LiteOn 4352)*

*VID: 0a5c  PID: 21e6 (ThinkPad NB)*\
*VID: 0489  PID: e079 (Lenovo BCM43162 NGFF)*\
*VID: 0489  PID: e07a (Lenovo NGFF)*\
*VID: 0930  PID: 0223 (Toshiba NGFF)*\
*VID: 0930  PID: 0221 (Toshiba)*

*VID: 0a5c  PID: 21e8 (Dongles Delock, IOGear etc.,)*\
*VID: 0489  PID: e032 (BCM4313)*

*AR3011:*

*VID: 03f0  PID: 311d*\
*VID: 0489 PID: e027*\
*VID: 0489 PID: e02c*\
*VID: 0489 PID: e03d*\
*VID: 0930 PID: 0215*\
*VID: 0b05 PID: 179c*\
*VID: 0cf3  PID: 3000*\
*VID: 0cf3  PID: 3002*\
*VID: 0cf3  PID: 3005*\
*VID: 13d3 PID: 3304*\
*VID: 04f2 PID: aff1  *\
*VID: 04ca  PID: 3002*

*AR3012:*

*VID: 0489 PID: e056*\
*VID: 0489 PID: e036*\
*VID: 0489 PID: e03c*\
*VID: 0489 PID: e078*\
*VID: 04ca PID: 3005*\
*VID: 04ca PID: 3006*\
*VID: 0cf3  PID: 311d*\
*VID: 0cf3  PID: 0036*\
*VID: 0cf3  PID: 3004*\
*VID: 0cf3  PID: 3010*\
*VID: 0cf3  PID: e004*\
*VID: 13d3 PID: 3362*\
*VID: 13d3 PID: 3402*\
*VID: 13d3 PID: 3474*

 *VID: 0b05 PID: 17d0*\
 

**[Kext Customization:]{.underline}**

You can customize the kext for choosing firmware by editing the
file \"info.plist\" located inside my
kext \"BTFirmwareUploader.kext/Contents/\".\
Go to the section \"IOKitPersonalities\" and edit the entry which has
your vendor and device id values.\
 

**Atheros AR3011 and AR3012 BT:**

You can choose firmware from either linux (or) windows. **(Linux
firmware is recommended mostly)**

- For Linux firmware, edit the plist entry \"Use Linux fw\" to Yes (or)
  true. )

- For the Windows firmware, edit the plist entry \"Use Linux fw\" to No
  (or) false for both AR3011 & AR3012.

- For AR3011, you can also choose three firmwares from windows by
  editing the plist entry \"Windows fw\" to the values 0-2. Value \'0\'
  is the most recent firmware. **\[Note: Only Windows firmware 1
  supports Fn key toggle for BT Off/on in Asus\]**

**Broadcom  BT:**

**Firmware to use:**

- You can choose either firmware which is meant for that specific device
  (or) higher version available so far by editing the plist entry
  ***\"DeviceSpecificFw\"*** to Yes/No (or) true/false.

- You can abort (or) ignore subsequent firmware updates if the last
  update was failed by editing the plist entry **\"AbortOnFwUpdFail\"**
  to Yes/No (or) true/false. This is optional and its meant only in case
  if any system needed update process to continue even if last update
  was failed for success.(Available from v3.0.5)

**[NOTE:]{.underline}**

If you have an Atheros AR3011/AR3012 and Broadcom BCM20702Ax BT which is
not working (or) your card Vendor and Product ID are not listed above in
the supported list, then PM me about your card details so i can add
support to your card. **You can also try yourself to add support by
editing plist with the following details [on your own risk]{.underline},
so its recommended to contact me for support.**\
 

**[You need to edit the following details from plist by
editing]{.underline}[ ]{.underline}[existing BCM entry (or) adding an
BCM entry using the existing BCM entry values to your bluetooth: (v2.6
or later)]{.underline}**

- **BTController** - Depending on your card, the Value should be AR3011,
  AR3012, BCMCOMBO (WiFi / BT combo card) and BCMSTDAL (BT only card).

- **idProduct** - Enter the decimal value of your product ID

- **idVendor** - Enter the decimal value of your vendor ID

**[Plist files which are needed to edit:]{.underline} **

**\"BTFirmwareUploader.kext/Contents/Info.plist\" and**

**\"BTFirmwareUploader.kext/Contents/PlugIns/BluetoothDevInfoInjector.kext/Contents/Info.plist\"**

 **From v3.0:**

**\"BTFirmwareUploader.kext/Contents/PlugIns/BluetoothDevBroadcomInjector.kext/Contents/Info.plist\"**

**[How to install:]{.underline}**

1\) Remove any patched kexts (or) Bluetooth enablers installed for
Bluetooth and use Apple vanilla Bluetooth kext.\
2) Install my kext (BTFirmwareUploader.kext) using your favorite kext
installers to /System/Library/Extensions.** (Don\'t install my kext
through E/E (or) clover injection).**

**[Supports:]{.underline}**

**â€‹Atheros BT is supported from 10.6.8 or later**\
**Broadcom BT is supported from 10.8.5 or later **

**NOTE for 10.11: If you have any problems with my kext firmware update
for Broadcom BT in 10.11 then use only
\"BluetoothDevBroadcomInjector.kext\" instead which is just an injector
to load Apple BT driver.**

**[Change Log:]{.underline}[ ]{.underline}**

**Update: v3.1 5 Released (Legacy version is for 10.10 or lower)**

- Added support for new device

- â€‹Supports for 10.6.8

- â€‹Fixed legacy version which was not working 

- Some Minor bug fixes and optimizations

**Update: v3.1 (Legacy version is for 10.10 or lower)**

- Updated non-device specific firmware version to latest available

- Added support new devices (check the updated list)

- Improved 10.11 support

- Some Minor bug fixes and optimizations

**Update: v3.0.5 Released**

- Fixed a major bug affecting Atheros Bluetooth

- Improved 10.11 support

- Added a plist option for Broadcom Bluetooth to abort update process on
  fail (Optional)

- Some Minor bug fixes and optimizations

**Update: v3.0**

- Updated support for 10.11 using new SDK and USB API (you have to use
  kext compiled for 10.11)

- Made several Improvements and optimizations

- Fixed some minor bugs

**Update: v2.8.7**

- Added OS X detection to prevent/solve issues in 10.11

- Fixed some bugs and made few improvements

**Update: v2.8.5**

- Fixed a major bug affecting some asus systems with KP

- Fixed issues related to firmware not getting updated sometimes

- Fixed some bugs in Atheros firmware update

- Added few new Atheros AR3012 bluetooth device ids to supported list

- Some improvements and code optimizations

**Update: v2.8**

- Updated firmware for some devices and higher version option

- Added support for some new devices

- Improvements to upload process time which finishes quicker now as
  i\'ve reduced the time needed by doing more beta tests so works better

- Some minor bug fixes and optimizations

**Update: v2.7 **

- Updated firmware for some devices and higher version option

- Added support for some new devices

- Fixes a bug when dual booting from windows which breaks BT

- Fixed a bug in BluetoothDevInfo kext injection and renamed the kext
  name

- Removed unnecessary \"UpdAfterSleep\" option from plist

- Some minor bug fixes and optimizations

**Update: v2.6**

- Added support for new Broadcom bluetooth devices

- Added support for new Atheros bluetooth devices

- Updated kext to use firmware specific to the Broadcom BT devices

- Removed unnecessary patch for HP BT (PID:21e3)

- Fixed some bugs

- Optimized code

**Update: v2.5.2 Final**

- Added support for two Broadcom bluetooth devices from HP(*VID: 0a5c
   PID: 21e3/PID: 21e1)*

- Fixed a minor bug in BluetoothDevInfo kext

**Update: v2.5 Final**

- Added plist option \"UseStandaloneFw\" to choose standalone firmware
  meant for BT only cards like Asus BT-400.

- Added support for two new BCM4352 BT **(Dell alienware BCM4352
  combo)**

- Fixed device reset error bugs

- Some minor bug fixes and optimizations

**Update: v2.4**

- â€‹Added support for two new AR3011 BT***(VID: 0489 PID:
  e02c* and *VID: 0930 PID: 0215)***

- Added support for two new BCM4352 BT **(HP and NGFF Toshiba)**

- Added support for new AR3012 BT** *(VID: 0cf3 PID: 311d)***

- Minor optimizations

**Update: v2.3**

- Improved performance of upload

- â€‹Fixed issues related to BCM

- Fixed issues related to firmware update after sleep

- Updated Broadcom firmware to new version

- Added LiteOn BCM4352 and an AR3011(PID: e027) card 

- Moved bluetooth info to a plugin kext located inside the PlugIns
  folder

- Optimized code

**Update: v2.1**

- Added fully working support for AR3012

- Updated new firmware in Broadcom

- Added Dell DW 1550 BCM 94352HMB card and AR3012 card ID\'s

- Fixed an issue with AR3011 bluetooth

- Optimized code

**Update: v2beta**

- Added experimental support for AR3012(still in testing phase)

- Added firmware option for Broadcom and updated to new firmware

- Added new firmware for Atheros

- Added Asus BCM4352 card support

- Fixed an issue with AR3011 bluetooth which is failing for some cards
  like (dev:0x3000 & 0x179c)

- Apple bluetooth kext patch is not needed for Broadcom bluetooth
  anymore

**Update: v1.5**

- Added more Atheros Bluetooth devices (VEN:0cf3, DEV:3000,3002,3005)

- Fixed an issue with Atheros AR3011 Bluetooth after sleep

- Fixed an issue with Broadcom Bluetooth which prevented successful
  system boot without -v flag

**First release: v1.3**

- Enables Bluetooth in the new ac Broadcom BCM4352 card

- Added support for AR3011 from old kext(credits: mac4mat)

- Fixes an issue for AR3011 with the old uploader kext which is breaking
  during reboots in Asus Notebooks

- Added options to choose firmware for Atheros Bluetooth in the plist
