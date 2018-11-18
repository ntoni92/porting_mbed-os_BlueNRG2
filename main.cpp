#include "mbed.h"
/*
#include "colorlib.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "bluenrg1_api.h"
//#include "BlueNRG1_it.h"
#include "BlueNRG1_conf.h"
#include "ble_const.h"
#include "bluenrg1_stack.h"
#include "Beacon_config.h"
#ifdef __cplusplus
}
#endif

extern "C" void Blue_Handler(void)
{
   // Call RAL_Isr
   RAL_Isr();
}

Serial pc(USBTX, USBRX);
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalIn but1(PUSH1);
DigitalIn but2(PUSH2);
//SPI spi(SPI_MOSI, SPI_MISO, SPI_SCK, SPI_CS); // mosi, miso, sclk
//DigitalOut cs(SPI_CS);

void init();
void Rx_interrupt();
uint8_t rx = 0;

//Private typedef
//Private define
#define BLE_BEACON_VERSION_STRING "1.1.0"
//Set to 1 for enabling Flags AD Type position at the beginning of the advertising packet
#define ENABLE_FLAGS_AD_TYPE_AT_BEGINNING 1
//Private macro
//Private variables
//Private function prototypes
//Private functions
static void Start_Beaconing(void);
void Device_Init(void);
void Beacon(void);
// MAIN

// main() runs in its own thread in the OS
int main() {
	led1 = 1;
	pc.printf("ANTONIO!!!\r\n");
	init();
    Beacon();
}

void Beacon (){
    uint8_t ret;
    	// BLE stack init
	ret = BlueNRG_Stack_Initialization(&BlueNRG_Stack_Init_params);
	if (ret != BLE_STATUS_SUCCESS) {
		pc.printf("Error in BlueNRG_Stack_Initialization() 0x%02x\r\n", ret);
		while(1);
	}
	//Init the BlueNRG-2 device
	Device_Init();

	Start_Beaconing();
	pc.printf("BlueNRG-2 BLE Beacon Application (version: %s)\r\n", BLE_BEACON_VERSION_STRING);
	while(1)
	{
		// BlueNRG-1 stack tick
		BTLE_StackTick();
		// Enable Power Save according the Advertising Interval
		// BlueNRG_Sleep(SLEEPMODE_NOTIMER, 0, 0);
	}
}

void init(){
	// Buttons
	but1.mode(NoPull);
	but2.mode(NoPull);
	// Serial interface
	pc.attach(&Rx_interrupt, Serial::RxIrq);
	// SPI Interface
	//cs=1;
	//spi.format(8, 0);
	//spi.frequency(100000);

	// Ready
	led2 = 1;
	pc.printf(CLS);
}

// Interrupt Routine to read in data from serial port
void Rx_interrupt() {
	rx = pc.getc();
}

//Private typedef -----------------------------------------------------------
//Private define ------------------------------------------------------------
#define BLE_BEACON_VERSION_STRING "1.1.0"

//Set to 1 for enabling Flags AD Type position at the beginning of the advertising packet
#define ENABLE_FLAGS_AD_TYPE_AT_BEGINNING 1

//Private macro -------------------------------------------------------------
//Private variables ---------------------------------------------------------
//Private function prototypes -----------------------------------------------
//Private functions ---------------------------------------------------------

void Device_Init(void)
{
	uint8_t ret;
	uint16_t service_handle;
	uint16_t dev_name_char_handle;
	uint16_t appearance_char_handle;

	//Set the TX Power to -2 dBm
	ret = aci_hal_set_tx_power_level(1,4);
	if(ret != 0) {
		pc.printf ("Error in aci_hal_set_tx_power_level() 0x%04xr\n", ret);
		while(1);
	}

	//Init the GATT
	ret = aci_gatt_init();
	if (ret != 0)
		pc.printf ("Error in aci_gatt_init() 0x%04xr\n", ret);
	else
		pc.printf ("aci_gatt_init() --> SUCCESS\r\n");

	//Init the GAP
	ret = aci_gap_init(0x01, 0x00, 0x08, &service_handle, &dev_name_char_handle, &appearance_char_handle);
	if (ret != 0)
		pc.printf ("Error in aci_gap_init() 0x%04x\r\n", ret);
	else
		pc.printf ("aci_gap_init() --> SUCCESS\r\n");
}

static void Start_Beaconing(void)
{
	uint8_t ret = BLE_STATUS_SUCCESS;

#if ENABLE_FLAGS_AD_TYPE_AT_BEGINNING
	//Set AD Type Flags at beginning on Advertising packet
	uint8_t adv_data[] = {
			//Advertising data: Flags AD Type
			0x02,
			0x01,
			0x06,
			//Shortened Local Name

			0x06, //len
			AD_TYPE_SHORTENED_LOCAL_NAME,
			0x50,	//P
			0x52,	//R
			0x4F,	//O
			0x56,	//V
			0x41	//A

			//Advertising data: manufacturer specific data

//			26, //len
//			AD_TYPE_MANUFACTURER_SPECIFIC_DATA,  //manufacturer type
//			0x30, 0x00, //Company identifier code (Default is 0x0030 - STMicroelectronics: To be customized for specific identifier)
//			0x02,       // ID
//			0x15,       //Length of the remaining payload
//			0xE2, 0x0A, 0x39, 0xF4, 0x73, 0xF5, 0x4B, 0xC4, //Location UUID
//			0xA1, 0x2F, 0x17, 0xD1, 0xAD, 0x07, 0xA9, 0x61,
//			0x00, 0x00, // Major number
//			0x00, 0x00, // Minor number
//			0xC8        //2's complement of the Tx power (-56dB)};
	};
#else
	uint8_t manuf_data[] = {
			26, //len
			AD_TYPE_MANUFACTURER_SPECIFIC_DATA, //manufacturer type
			0x30, 0x00, //Company identifier code (Default is 0x0030 - STMicroelectronics: To be customized for specific identifier)
			0x02,       // ID
			0x15,       //Length of the remaining payload
			0xE2, 0x0A, 0x39, 0xF4, 0x73, 0xF5, 0x4B, 0xC4, //Location UUID
			0xA1, 0x2F, 0x17, 0xD1, 0xAD, 0x07, 0xA9, 0x61,
			0x00, 0x00, // Major number
			0x00, 0x00, // Minor number
			0xC8        //2's complement of the Tx power (-56dB)};
	};
#endif

	//disable scan response
	ret = hci_le_set_scan_response_data(0,NULL);
	if (ret != BLE_STATUS_SUCCESS)
	{
		pc.printf ("Error in hci_le_set_scan_resp_data() 0x%04x\r\n", ret);
		return;
	}
	else
		pc.printf ("hci_le_set_scan_resp_data() --> SUCCESS\r\n");

	//put device in non connectable mode
	ret = aci_gap_set_discoverable(ADV_NONCONN_IND, 160, 160, PUBLIC_ADDR, NO_WHITE_LIST_USE,
			0, NULL, 0, NULL, 0, 0);
	if (ret != BLE_STATUS_SUCCESS)
	{
		pc.printf ("Error in aci_gap_set_discoverable() 0x%04x\r\n", ret);
		return;
	}
	else
		pc.printf ("aci_gap_set_discoverable() --> SUCCESS\r\n");

#if ENABLE_FLAGS_AD_TYPE_AT_BEGINNING
	//Set the  ADV data with the Flags AD Type at beginning of the advertsing packet,  followed by the beacon manufacturer specific data
	ret = hci_le_set_advertising_data (sizeof(adv_data), adv_data);
	if (ret != BLE_STATUS_SUCCESS)
	{
		pc.printf ("Error in hci_le_set_advertising_data() 0x%04x\r\n", ret);
		return;
	}
	else
		pc.printf ("hci_le_set_advertising_data() --> SUCCESS\r\n");
#else
	//Delete the TX power level information
	ret = aci_gap_delete_ad_type(AD_TYPE_TX_POWER_LEVEL);
	if (ret != BLE_STATUS_SUCCESS)
	{
		pc.printf ("Error in aci_gap_delete_ad_type() 0x%04x\r\n", ret);
		return;
	}
	else
		pc.printf ("aci_gap_delete_ad_type() --> SUCCESS\r\n");

	//Update the ADV data with the BEACON manufacturing data
	ret = aci_gap_update_adv_data(27, manuf_data);
	if (ret != BLE_STATUS_SUCCESS)
	{
		pc.printf ("Error in aci_gap_update_adv_data() 0x%04x\r\n", ret);
		return;
	}
	else
		printf ("aci_gap_update_adv_data() --> SUCCESS\r\n");
#endif
}
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mbed.h"

DigitalOut led1(LED1);
DigitalOut led2(LED2);
InterruptIn button(BUTTON1);
Serial pc(USBTX,USBRX);
//Thread thread;

void flip(){
	led2 = !led2;
}

void on(){
	led2 = 1;
}

void off(){
	led2 = 0;
}

//void led2_thread() {
//    while (true) {
//        led2 = 1;
//        pc.printf("thread2\r\n");
//        //wait(1);
//    }
//}

// main() runs in its own thread in the OS
int main() {
	//thread.start(led2_thread);
	on();
	button.rise(&flip);
	button.fall(&flip);
    while (true) {
        led1 = !led1;
        wait(1);
        pc.printf("thread main\r\n");
        //wait(5);
        //pc.printf("CIAO\r\n");
        //while(1){;}
     }
}


//#include "mbed_mem_trace.h"
//
//DigitalOut led1(LED1);
//
//void print_memory_info() {
//    // allocate enough room for every thread's stack statistics
//    int cnt = osThreadGetCount();
//    mbed_stats_stack_t *stats = (mbed_stats_stack_t*) malloc(cnt * sizeof(mbed_stats_stack_t));
//
//    cnt = mbed_stats_stack_get_each(stats, cnt);
//    for (int i = 0; i < cnt; i++) {
//        printf("Thread: 0x%lX, Stack size: %lu / %lu\r\n", stats[i].thread_id, stats[i].max_size, stats[i].reserved_size);
//    }
//    free(stats);
//
//    // Grab the heap statistics
//    mbed_stats_heap_t heap_stats;
//    mbed_stats_heap_get(&heap_stats);
//    printf("Heap size: %lu / %lu bytes\r\n", heap_stats.current_size, heap_stats.reserved_size);
//}
//
//// main() runs in its own thread in the OS
//int main() {
//	//view memory regions
//	while(1){
//		print_memory_info();
//
//		// Rest of the program
//		led1 = !led1;
//		wait(0.5);
//	}
//
//	//debug heap allocation error
//
//	print_memory_info();
//
//	size_t malloc_size = 16 * 1024;
//	while (true) {
//	    wait(0.2);
//	    // fill up the memory
//	    void *ptr = malloc(malloc_size);
//	    printf("Allocated %d bytes (success %d)\n", malloc_size, ptr != NULL);
//	    if (ptr == NULL) {
//	        malloc_size /= 2;
//	        print_memory_info();
//	    }
//	    // and then allocate an object on the heap
//	    DigitalOut* led = new DigitalOut(LED1);
//	}
//
//
//	//debug stack overflow error
//
//	uint8_t big_arr[1024];
//	for (size_t ix = 0; ix < sizeof(big_arr); ix++) big_arr[ix] = ix; // fill the memory
//	print_memory_info();
//
//	while (true) {
//	    wait(1.0);
//	    // allocate an array that does not fit on the stack
//	    char another_array[2048 + (rand() % 1024)];
//	    for (size_t ix = 0; ix < sizeof(another_array); ix++) another_array[ix] = ix;
//
//	    // some random operations on the arrays to prevent them from being optimized away
//	    another_array[rand() % sizeof(another_array)] = big_arr[rand() % sizeof(big_arr)];
//	    printf("random number is %d\n", another_array[rand() % sizeof(another_array)]);
//	}
//
//
//}
//
