//
// MacOS X standalone firmware downloader for the EZUSB device, 
// as found in MIDIMan MIDISPORT boxes.
//
// This is the MIDISPORT specific main function. The rest of the utility is applicable
// to all EZUSB boxes.
//
// By Leigh Smith <leigh@leighsmith.com>
//
#include <iostream>
#include "EZLoader.h"

#define midimanVendorID 0x0763

extern INTEL_HEX_RECORD firmware1x1[];
extern INTEL_HEX_RECORD firmware2x2[];
extern INTEL_HEX_RECORD firmware4x4[];

enum WarmFirmwareProductIDs {
    MIDISPORT1x1 = 0x1011,
    MIDISPORT2x2 = 0x1002,
    MIDISPORT4x4 = 0x1021,
    MIDISPORT8x8 = 0x1031
};

// Strictly speaking, the endPoint 2 can sustain 40 bytes output on the 8x8.
// There are 9 ports including the SMPTE control.
struct HardwareConfigurationDescription {
    enum WarmFirmwareProductIDs warmFirmwareProductID;   // product ID indicating the firmware has been loaded and is working.
    int coldBootProductID;                               // product ID indicating the firmware has not been loaded.
    std::string modelName;
    std::vector<INTEL_HEX_RECORD> firmware;
    std::string firmwareFileName;                        // Path to the Intel hex file of the firmware. NULL indicates no firmware needs to be downloaded.
} productTable[] = {
    { MIDISPORT1x1, 0x1010, "1x1", std::vector<INTEL_HEX_RECORD>(), "/Users/leigh/Sources/Drivers/MIDI/MIDISPORT/Linux/midisport-firmware-1.2/MidiSport1x1.ihx" },
    { MIDISPORT2x2, 0x1001, "2x2", std::vector<INTEL_HEX_RECORD>(), "/Users/leigh/Sources/Drivers/MIDI/MIDISPORT/Linux/midisport-firmware-1.2/MidiSport2x2.ihx" },
    { MIDISPORT4x4, 0x1020, "4x4", std::vector<INTEL_HEX_RECORD>(), "/Users/leigh/Sources/Drivers/MIDI/MIDISPORT/Linux/midisport-firmware-1.2/MidiSport4x4.ihx" },
    { MIDISPORT8x8, 0x1030, "8x8", std::vector<INTEL_HEX_RECORD>(), "" }
};

#define PRODUCT_TOTAL (sizeof(productTable) / sizeof(struct HardwareConfigurationDescription))

int main(int argc, const char * argv[])
{
    EZUSBLoader ezusb;
    std::vector <INTEL_HEX_RECORD> hexLoader;

    // TODO load config file supplied on command line.
    // TODO retrieve the hex loader filename from the config file.
    std::string hexLoaderFileName = "/Users/leigh/Sources/Drivers/MIDI/MIDISPORT/Linux/midisport-firmware-1.2/MidiSportLoader.ihx";
    std::cout << "Reading MIDISPORT Firmware Intel hex file " << hexLoaderFileName << std::endl;
    if (!ezusb.ReadFirmwareFromHexFile(hexLoaderFileName, hexLoader)) {
        return 1;
    }
    ezusb.SetApplicationLoader(hexLoader);
    
    std::cout << "Looking for uninitialised MIDISPORTs" << std::endl;
    
    // Determine if the MIDISPORT is in firmware downloaded or unloaded state.
    // If cold booted, we need to download the firmware and restart the device to
    // enable the firmware product code to be found.
    for (unsigned int productIndex = 0; productIndex < PRODUCT_TOTAL; productIndex++) {
        if (ezusb.FindVendorsProduct(midimanVendorID, productTable[productIndex].coldBootProductID, true)) {
            bool foundMIDSPORT = false;
            std::vector <INTEL_HEX_RECORD> firmwareToDownload;

            std::cout << "Found MIDISPORT " << productTable[productIndex].modelName << " in cold booted state." << std::endl;
            if (productTable[productIndex].firmwareFileName.length() != 0) {
                std::cout << "Reading MIDISPORT Firmware Intel hex file " << productTable[productIndex].firmwareFileName << std::endl;
                if (!ezusb.ReadFirmwareFromHexFile(productTable[productIndex].firmwareFileName, firmwareToDownload)) {
                    return 2;
                }
                std::cout << "Downloading firmware." << std::endl;
                ezusb.SetApplicationFirmware(firmwareToDownload);
                ezusb.StartDevice();
                // Wait up to 20 seconds for the firmware to boot & re-enumerate the USB bus properly.
                for (unsigned int testCount = 0; testCount < 10 && !foundMIDSPORT; testCount++) {
                    foundMIDSPORT = ezusb.FindVendorsProduct(midimanVendorID, productTable[productIndex].warmFirmwareProductID, false);
                    std::cout << "Waiting before searching" << std::endl;
                    sleep(2);
                }
                if (!foundMIDSPORT) {
                    std::cout << "Can't find re-enumerated MIDISPORT device, probable failure in downloading firmware." << std::endl;
                    return 3;
                }
            }
        }
    }
    return 0;
}

