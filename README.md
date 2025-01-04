## NEO1541
**Commodore 1541 emulator for Linux**
![neo1541](https://github.com/user-attachments/assets/e90d0642-ca69-4f0b-a322-80e541a55f26)
NEO1541 is a software emulator of the Commodore 1541 IEC protocol.

Currently you can load/save prg files directly from your hard disk,
or from D64 image files (saving to D64 is not supported yet).

To use it you need a hardware parallel port and an XM1541 cable.
It also works with PCIe expansion cards (at least I can say it works with my card bought on AliExpress based on the WCH382L chip).
I don't think it works with USB-LPT adapters.

To compile, just run make (you need gcc and ncurses libraries), then ./neo1541

If your parallel port is different from 0x378, you can try "*sudo cat /proc/ioports | grep parport*" to find your port.
For example in my case, the parallel port is on 0xd100

**USE EXAMPLES:**
To specify the parallel port to use: "*./neo1541 --port 0xd100*" (if the port is 0xd100)
To specify the root directory: "*./neo1541 --disk /home/user/c64prg/*"
For VIC20 mode: "*./neo1541 --vic20*"
