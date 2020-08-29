# N64 GameCat

This is a free, open-source implementation of a Nintendo 64 cheating device, akin to the GameShark. It's currently based around the Kendryte K210 SoC (using the Sipeed MAiX BiT breakout), which I chose due to the high number of configurable IO (the N64 cartridge has 50 pins, though many of those aren't necessary for this project). I may eventually add support for ESP32 chips to the GameCat as well.

## Building
This is a PlatformIO project (https://platformio.org/), so building and uploading are relatively straightforward. Simply install PlatformIO (and make sure to add it to your $PATH), and then run `platformio run --target upload --target monitor` with the MAiX BiT plugged in. If it complains about `udev` rules, follow the link it provides.

## Circuits/schematics
I have some test PCBs on order (files are in the `/pcb` directory, made with KiCad EDA). Once I get the design for those finalized it should be as simple as ordering a PCB, soldering the MCU on, and soldering on a cartridge slot
#### Note:
When ordering a cartridge slot, it's important to get one with 2.5mm pin spacing. I've seen several only with 2.54mm spacing, which may not work on this PCB (or with the cartridges in general). I'm using this $2.50 version from AliExpress: https://www.aliexpress.com/item/33002915153.html?spm=a2g0s.9042311.0.0.1ec84c4dN5ezxJ

## FAQ
### Why do this?
I was bored, and that's literally the only reason. If you want to cheat at N64 games, you're likely much better off with an emulator.

### I looked at the PCB. Won't the cartridge end up being parallel with the console if I solder the cartridge slot on it?
Yes.
### Why?
I have no idea what I'm doing.
