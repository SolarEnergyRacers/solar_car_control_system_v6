# Static USB Port Assignment for ESP32 Boards

## Check it

`ll /dev/ttyU* ; ll /dev/esp32-*`

## Explore it

```bash
udevadm info -a -n /dev/ttyUSB1 > ac.udevadm.log
udevadm info -a -n /dev/ttyUSB1 > dc.udevadm.log
code ac.udevadm.log dc.udevadm.log 
```

## Set it

```bash
sudo nano /etc/udev/rules.d/99-esp32.rules
```

```rules
# Rule for first ESP32 board (plugged into port with KERNELS 1-1.2)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", KERNELS=="1-2", SYMLINK+="esp32-ac"

# Rule for second ESP32 board (plugged into port with KERNELS 1-1.3)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", KERNELS=="1-1", SYMLINK+="esp32-dc"

```


```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```


If both ESP32 boards report identical attributes (e.g., the same ATTRS{serial}, ATTRS{idVendor}, and ATTRS{idProduct}), that's common for identical hardware. We can't rely on device-specific attributes for uniqueness, but we can use port-specific attributes from the USB hierarchy (like the physical USB port path) to distinguish them. This requires plugging each board into a different USB port and matching on the KERNELS or DEVPATH attribute, which represents the device's position in the USB tree.

Updated Steps to Create Unique udev Rules

Plug in only one ESP32 board to a specific USB port (e.g., the leftmost port on your hub or motherboard). Note which port you're using.

Identify the device and get full attributes:

Find the assigned /dev/ttyUSB* device:

`ls /dev/ttyUSB*`

(Assume it's /dev/ttyUSB0 for this example.)

Get the full udev info:

`udevadm info -a -n /dev/ttyUSB0`

Look for lines like:

```bash
KERNELS=="1-1.2"  # This is the USB port path (varies by port)DEVPATH=="/devices/pci0000:00/0000:00:14.0/usb1/1-1/1-1.2/1-1.2:1.0/ttyUSB0/tty/ttyUSB0"
The KERNELS value (e.g., 1-1.2) is key—it's unique to the physical port. Copy this value for the first board.
Unplug the first board, plug in the second board to a different USB port (e.g., the next port over). Repeat step 2 to get its KERNELS value (e.g., 1-1.3).
```

Create or update the udev rules file (as root):

`sudo nano /etc/udev/rules.d/99-esp32.rules`

Use the KERNELS values to make rules unique. Example (replace with your actual values):

```bash
# Rule for first ESP32 board (plugged into port with KERNELS 1-1.2)SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", KERNELS=="1-1.2", SYMLINK+="esp32-ac1"
# Rule for second ESP32 board (plugged into port with KERNELS 1-1.3)SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", KERNELS=="1-1.3", SYMLINK+="esp32-ac2"
```

If KERNELS isn't reliable (e.g., if ports are on different hubs), use DEVPATH instead (match the full path up to the tty device).
Save and exit.

Reload and test:

Reload rules:

`sudo udevadm control --reload-rules && sudo udevadm trigger`

## Static USB Ports for ESP32 Boards

Based on udevadm info for one ESP32 board (plugged into a specific port). Use KERNELS or other unique attributes to create udev rules for persistent naming.

**Note:** This log shows KERNELS=="1-1", which is the same as the first board. To distinguish, plug each board into different USB ports (e.g., different physical ports on your hub or motherboard) so KERNELS differs (e.g., 1-1 for one, 1-2 for the other).

---

Udevadm info starts with the device specified by the devpath and then
walks up the chain of parent devices. It prints for every device
found, all possible attributes in the udev rules key format.
A rule to match, can be composed by the attributes of the device
and the attributes from one single parent device.

```bash
  looking at device '/devices/pci0000:00/0000:00:14.0/usb1/1-1/1-1:1.0/ttyUSB1/tty/ttyUSB1':
    KERNEL=="ttyUSB1"
    SUBSYSTEM=="tty"
    DRIVER==""
    ATTR{power/async}=="disabled"
    ATTR{power/control}=="auto"
    ATTR{power/runtime_active_kids}=="0"
    ATTR{power/runtime_active_time}=="0"
    ATTR{power/runtime_enabled}=="disabled"
    ATTR{power/runtime_status}=="unsupported"
    ATTR{power/runtime_suspended_time}=="0"
    ATTR{power/runtime_usage}=="0"

  looking at parent device '/devices/pci0000:00/0000:00:14.0/usb1/1-1/1-1:1.0/ttyUSB1':
    KERNELS=="ttyUSB1"
    SUBSYSTEMS=="usb-serial"
    DRIVERS=="ch341-uart"
    ATTRS{port_number}=="0"
    ATTRS{power/async}=="enabled"
    ATTRS{power/control}=="auto"
    ATTRS{power/runtime_active_kids}=="0"
    ATTRS{power/runtime_active_time}=="0"
    ATTRS{power/runtime_enabled}=="disabled"
    ATTRS{power/runtime_status}=="unsupported"
    ATTRS{power/runtime_suspended_time}=="0"
    ATTRS{power/runtime_usage}=="0"

  looking at parent device '/devices/pci0000:00/0000:00:14.0/usb1/1-1/1-1:1.0':
    KERNELS=="1-1:1.0"
    SUBSYSTEMS=="usb"
    DRIVERS=="ch341"
    ATTRS{authorized}=="1"
    ATTRS{bAlternateSetting}==" 0"
    ATTRS{bInterfaceClass}=="ff"
    ATTRS{bInterfaceNumber}=="00"
    ATTRS{bInterfaceProtocol}=="02"
    ATTRS{bInterfaceSubClass}=="01"
    ATTRS{bNumEndpoints}=="03"
    ATTRS{physical_location/dock}=="no"
    ATTRS{physical_location/horizontal_position}=="right"
    ATTRS{physical_location/lid}=="no"
    ATTRS{physical_location/panel}=="left"
    ATTRS{physical_location/vertical_position}=="lower"
    ATTRS{power/async}=="enabled"
    ATTRS{power/runtime_active_kids}=="0"
    ATTRS{power/runtime_enabled}=="enabled"
    ATTRS{power/runtime_status}=="suspended"
    ATTRS{power/runtime_usage}=="0"
    ATTRS{supports_autosuspend}=="1"

  looking at parent device '/devices/pci0000:00/0000:00:14.0/usb1/1-1':
    KERNELS=="1-1"
    SUBSYSTEMS=="usb"
    DRIVERS=="usb"
    ATTRS{authorized}=="1"
    ATTRS{avoid_reset_quirk}=="0"
    ATTRS{bConfigurationValue}=="1"
    ATTRS{bDeviceClass}=="ff"
    ATTRS{bDeviceProtocol}=="00"
    ATTRS{bDeviceSubClass}=="00"
    ATTRS{bMaxPacketSize0}=="8"
    ATTRS{bMaxPower}=="98mA"
    ATTRS{bNumConfigurations}=="1"
    ATTRS{bNumInterfaces}==" 1"
    ATTRS{bcdDevice}=="0264"
    ATTRS{bmAttributes}=="80"
    ATTRS{busnum}=="1"
    ATTRS{configuration}==""
    ATTRS{devnum}=="33"
    ATTRS{devpath}=="1"
    ATTRS{idProduct}=="7523"
    ATTRS{idVendor}=="1a86"
    ATTRS{ltm_capable}=="no"
    ATTRS{maxchild}=="0"
    ATTRS{physical_location/dock}=="no"
    ATTRS{physical_location/horizontal_position}=="right"
    ATTRS{physical_location/lid}=="no"
    ATTRS{physical_location/panel}=="left"
    ATTRS{physical_location/vertical_position}=="lower"
    ATTRS{power/active_duration}=="14812"
    ATTRS{power/async}=="enabled"
    ATTRS{power/autosuspend}=="2"
    ATTRS{power/autosuspend_delay_ms}=="2000"
    ATTRS{power/connected_duration}=="14811"
    ATTRS{power/control}=="on"
    ATTRS{power/level}=="on"
    ATTRS{power/persist}=="1"
    ATTRS{power/runtime_active_kids}=="0"
    ATTRS{power/runtime_active_time}=="14304"
    ATTRS{power/runtime_enabled}=="forbidden"
    ATTRS{power/runtime_status}=="active"
    ATTRS{power/runtime_suspended_time}=="0"
    ATTRS{power/runtime_usage}=="1"
    ATTRS{product}=="USB Serial"
    ATTRS{quirks}=="0x0"
    ATTRS{removable}=="removable"
    ATTRS{remove}=="(not readable)"
    ATTRS{rx_lanes}=="1"
    ATTRS{speed}=="12"
    ATTRS{tx_lanes}=="1"
    ATTRS{urbnum}=="15"
    ATTRS{version}==" 1.10"

  looking at parent device '/devices/pci0000:00/0000:00:14.0/usb1':
    KERNELS=="usb1"
    SUBSYSTEMS=="usb"
    DRIVERS=="usb"
    ATTRS{authorized}=="1"
    ATTRS{authorized_default}=="1"
    ATTRS{avoid_reset_quirk}=="0"
    ATTRS{bConfigurationValue}=="1"
    ATTRS{bDeviceClass}=="09"
    ATTRS{bDeviceProtocol}=="01"
    ATTRS{bDeviceSubClass}=="00"
    ATTRS{bMaxPacketSize0}=="64"
    ATTRS{bMaxPower}=="0mA"
    ATTRS{bNumConfigurations}=="1"
    ATTRS{bNumInterfaces}==" 1"
    ATTRS{bcdDevice}=="0608"
    ATTRS{bmAttributes}=="e0"
    ATTRS{busnum}=="1"
    ATTRS{configuration}==""
    ATTRS{devnum}=="1"
    ATTRS{devpath}=="0"
    ATTRS{idProduct}=="0002"
    ATTRS{idVendor}=="1d6b"
    ATTRS{interface_authorized_default}=="1"
    ATTRS{ltm_capable}=="no"
    ATTRS{manufacturer}=="Linux 6.8.0-107-generic xhci-hcd"
    ATTRS{maxchild}=="12"
    ATTRS{power/active_duration}=="207132742"
    ATTRS{power/async}=="enabled"
    ATTRS{power/autosuspend}=="0"
    ATTRS{power/autosuspend_delay_ms}=="0"
    ATTRS{power/connected_duration}=="207132742"
    ATTRS{power/control}=="auto"
    ATTRS{power/level}=="auto"
    ATTRS{power/runtime_active_kids}=="4"
    ATTRS{power/runtime_active_time}=="207132725"
    ATTRS{power/runtime_enabled}=="enabled"
    ATTRS{power/runtime_status}=="active"
    ATTRS{power/runtime_suspended_time}=="0"
    ATTRS{power/runtime_usage}=="0"
    ATTRS{power/wakeup}=="disabled"
    ATTRS{power/wakeup_abort_count}==""
    ATTRS{power/wakeup_active}==""
    ATTRS{power/wakeup_active_count}==""
    ATTRS{power/wakeup_count}==""
    ATTRS{power/wakeup_expire_count}==""
    ATTRS{power/wakeup_last_time_ms}==""
    ATTRS{power/wakeup_max_time_ms}==""
    ATTRS{power/wakeup_total_time_ms}==""
    ATTRS{product}=="xHCI Host Controller"
    ATTRS{quirks}=="0x0"
    ATTRS{removable}=="unknown"
    ATTRS{remove}=="(not readable)"
    ATTRS{rx_lanes}=="1"
    ATTRS{serial}=="0000:00:14.0"
    ATTRS{speed}=="480"
    ATTRS{tx_lanes}=="1"
    ATTRS{urbnum}=="2228"
    ATTRS{version}==" 2.00"

  looking at parent device '/devices/pci0000:00/0000:00:14.0':
    KERNELS=="0000:00:14.0"
    SUBSYSTEMS=="pci"
    DRIVERS=="xhci_hcd"
    ATTRS{ari_enabled}=="0"
    ATTRS{broken_parity_status}=="0"
    ATTRS{class}=="0x0c0330"
    ATTRS{consistent_dma_mask_bits}=="64"
    ATTRS{d3cold_allowed}=="1"
    ATTRS{dbc}=="disabled"
    ATTRS{dbc_bInterfaceProtocol}=="01"
    ATTRS{dbc_bcdDevice}=="0010"
    ATTRS{dbc_idProduct}=="0010"
    ATTRS{dbc_idVendor}=="1d6b"
    ATTRS{dbc_poll_interval_ms}=="64"
    ATTRS{device}=="0x9d2f"
    ATTRS{dma_mask_bits}=="64"
    ATTRS{driver_override}=="(null)"
    ATTRS{enable}=="1"
    ATTRS{irq}=="130"
    ATTRS{local_cpulist}=="0-7"
    ATTRS{local_cpus}=="ff"
    ATTRS{msi_bus}=="1"
    ATTRS{msi_irqs/130}=="msi"
    ATTRS{msi_irqs/131}=="msi"
    ATTRS{msi_irqs/132}=="msi"
    ATTRS{msi_irqs/133}=="msi"
    ATTRS{msi_irqs/134}=="msi"
    ATTRS{msi_irqs/135}=="msi"
    ATTRS{msi_irqs/136}=="msi"
    ATTRS{msi_irqs/137}=="msi"
    ATTRS{numa_node}=="-1"
    ATTRS{power/async}=="enabled"
    ATTRS{power/control}=="auto"
    ATTRS{power/runtime_active_kids}=="2"
    ATTRS{power/runtime_active_time}=="207133325"
    ATTRS{power/runtime_enabled}=="enabled"
    ATTRS{power/runtime_status}=="active"
    ATTRS{power/runtime_suspended_time}=="0"
    ATTRS{power/runtime_usage}=="0"
    ATTRS{power/wakeup}=="enabled"
    ATTRS{power/wakeup_abort_count}=="0"
    ATTRS{power/wakeup_active}=="0"
    ATTRS{power/wakeup_active_count}=="0"
    ATTRS{power/wakeup_count}=="0"
    ATTRS{power/wakeup_expire_count}=="0"
    ATTRS{power/wakeup_last_time_ms}=="0"
    ATTRS{power/wakeup_max_time_ms}=="0"
    ATTRS{power/wakeup_total_time_ms}=="0"
    ATTRS{power_state}=="D0"
    ATTRS{remove}=="(not readable)"
    ATTRS{rescan}=="(not readable)"
    ATTRS{resource0}=="(not readable)"
    ATTRS{revision}=="0x21"
    ATTRS{subsystem_device}=="0x2259"
    ATTRS{subsystem_vendor}=="0x17aa"
    ATTRS{vendor}=="0x8086"

  looking at parent device '/devices/pci0000:00':
    KERNELS=="pci0000:00"
    SUBSYSTEMS==""
    DRIVERS==""
    ATTRS{power/async}=="enabled"
    ATTRS{power/control}=="auto"
    ATTRS{power/runtime_active_kids}=="11"
    ATTRS{power/runtime_active_time}=="0"
    ATTRS{power/runtime_enabled}=="disabled"
    ATTRS{power/runtime_status}=="unsupported"
    ATTRS{power/runtime_suspended_time}=="0"
    ATTRS{power/runtime_usage}=="0"
    ATTRS{waiting_for_supplier}=="0"
```

## Example udev Rules

Based on this output, use KERNELS=="1-1" (from the parent device) combined with idVendor/idProduct for matching. For the second board, plug it into a different port to get a different KERNELS (e.g., 1-2).

```bash
# /etc/udev/rules.d/99-esp32.rules
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", KERNELS=="1-1", SYMLINK+="esp32-ac1"
# For second board: SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", KERNELS=="1-2", SYMLINK+="esp32-ac2"
```
