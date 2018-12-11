#include <events/mbed_events.h>
#include <mbed.h>
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/services/HeartRateService.h"
#include "mbed_printf.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "bluenrg1_api.h"
#include "bluenrg1_events.h"
#include "bluenrg1_stack.h"
#ifdef __cplusplus
}
#endif
/*
extern "C" void Blue_Handler(void)
{
   // Call RAL_Isr
   RAL_Isr();
}
*/
DigitalOut led1(LED1);

const static char     DEVICE_NAME[] = "HRM";
static const uint16_t uuid16_list[] = {GattService::UUID_HEART_RATE_SERVICE};

static uint8_t hrmCounter = 100; // init HRM to 100bps
static HeartRateService *hrServicePtr;

static EventQueue eventQueue(/* event count */ 8 * EVENTS_EVENT_SIZE);

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    BLE::Instance().gap().startAdvertising(); // restart advertising
}

void updateSensorValue() {
    // Do blocking calls or whatever is necessary for sensor polling.
    // In our case, we simply update the HRM measurement.
    hrmCounter++;

    //  100 <= HRM bps <=175
    if (hrmCounter == 175) {
        hrmCounter = 100;
    }

    hrServicePtr->updateHeartRate(hrmCounter);
}

void periodicCallback(void)
{
    led1 = !led1; /* Do blinky on LED1 while we're waiting for BLE events */
    BTLE_StackTick();
    if (BLE::Instance().getGapState().connected) {
        eventQueue.call(updateSensorValue);
    }
}

void onBleInitError(BLE &ble, ble_error_t error)
{
    (void)ble;
    (void)error;
   /* Initialization error handling should go here */
}

void printMacAddress()
{
    /* Print out device MAC address to the console*/
    Gap::AddressType_t addr_type;
    Gap::Address_t address;
    BLE::Instance().gap().getAddress(&addr_type, address);
    mbed_printf("DEVICE MAC ADDRESS: ");
    for (int i = 5; i >= 1; i--){
        mbed_printf("%02x:", address[i]);
    }
    mbed_printf("%02x\r\n", address[0]);
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        onBleInitError(ble, error);
        return;
    }

    if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    ble.gap().onDisconnection(disconnectionCallback);

    /* Setup primary service. */
    hrServicePtr = new HeartRateService(ble, hrmCounter, HeartRateService::LOCATION_FINGER);

    /* Setup advertising. */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::GENERIC_HEART_RATE_SENSOR);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms */
    ble.gap().startAdvertising();

    printMacAddress();
}

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    BLE &ble = BLE::Instance();
    eventQueue.call(Callback<void()>(&ble, &BLE::processEvents));
}

int main()
{
	/* System Init */
	//DTM_SystemInit();

	/* Stack Initialization */
	//DTM_StackInit();

	eventQueue.call_every(500, periodicCallback);
	led1=0;
    BLE &ble = BLE::Instance();  //inside there is the creation of the hci driver
    ble.onEventsToProcess(scheduleBleEventsProcessing);
    ble.init(bleInitComplete);  //inside there is a reference to the hci driver

    eventQueue.dispatch_forever();

    return 0;
}
