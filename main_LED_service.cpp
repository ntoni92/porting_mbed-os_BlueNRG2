
#include "mbed.h"
#include "ble/services/iBeacon.h"
#include "ble/BLEProtocol.h"
//#include "bluenrg1_api.h"
#include "ble/services/iBeacon.h"

const static char     DEVICE_NAME[] = "ARM_Cordio BNRG2";

void printMacAddress()
{
    /* Print out device MAC address to the console*/
    Gap::AddressType_t addr_type;
    Gap::Address_t address;
    BLE::Instance().gap().getAddress(&addr_type, address);
    printf("DEVICE MAC ADDRESS: ");
    for (int i = 5; i >= 1; i--){
        printf("%02x:", address[i]);
    }
    printf("%02x\r\n", address[0]);
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE &ble          = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        return;
    }
    if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    /**
     * The Beacon payload has the following composition:
     * 128-Bit / 16byte UUID = E2 0A 39 F4 73 F5 4B C4 A1 2F 17 D1 AD 07 A9 61
     * Major/Minor  = 0x1122 / 0x3344
     * Tx Power     = 0xC8 = 200, 2's compliment is 256-200 = (-56dB)
     *
     * Note: please remember to calibrate your beacons TX Power for more accurate results.
     */
/*
    const uint8_t uuid[] = {0xE2, 0x0A, 0x39, 0xF4, 0x73, 0xF5, 0x4B, 0xC4,
                            0xA1, 0x2F, 0x17, 0xD1, 0xAD, 0x07, 0xA9, 0x61};
    uint16_t majorNumber = 1122;
    uint16_t minorNumber = 3344;
    uint16_t txPower     = 0xC8;
    iBeacon *ibeacon = new iBeacon(ble, uuid, majorNumber, minorNumber, txPower);

    ble.gap().setAdvertisingInterval(1000); // 1000ms.
    ble.gap().startAdvertising();
*/

    const BLEProtocol::AddressBytes_t address = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xC0};
    ble.gap().setAddress(BLEProtocol::AddressType::RANDOM_STATIC, address);
    ble.gap().enablePrivacy(true);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::GENERIC_HEART_RATE_SENSOR);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(500); // 500ms
    ble.gap().startAdvertising();

        //printMacAddress();

}

int main(void)
{
	BLE &ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
	ble.init(bleInitComplete);

    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    //while (!ble.hasInitialized()) { /* spin loop */ }

    while (true) {
        ble.waitForEvent(); // allows or low power operation
    }
}
