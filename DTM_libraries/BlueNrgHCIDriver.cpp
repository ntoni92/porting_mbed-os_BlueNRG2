#include <stdio.h>
#include "CordioBLE.h"
#include "CordioHCIDriver.h"
#include "CordioHCITransportDriver.h"
#include "mbed.h"
#include "hci_api.h"
#include "hci_cmd.h"
#include "hci_core.h"
#include "dm_api.h"
#include "bstream.h"
//#include "hci_mbed_os_adaptation.h"
#include "DTM_boot.h"
#include "DTM_cmd_db.h"
#include "osal.h"
#include "bluenrg1_api.h"
#include "hci_defs.h"
//#include "transport_layer.h"

extern "C" void rcv_callback(uint8_t *data, uint16_t len){ble::vendor::cordio::CordioHCITransportDriver::on_data_received(data, len);}

#define HCI_RESET_RAND_CNT        4

//#define VENDOR_SPECIFIC_EVENT     0xFF
//#define EVT_BLUE_INITIALIZED      0x0001
#define ACI_READ_CONFIG_DATA_OPCODE 0xFC0D
#define ACI_WRITE_CONFIG_DATA_OPCODE 0xFC0C
//#define ACI_GATT_INIT_OPCODE 0xFD01
//#define ACI_GAP_INIT_OPCODE 0xFC8A

//#define PUBLIC_ADDRESS_OFFSET 0x00
#define RANDOM_STATIC_ADDRESS_OFFSET 0x80
#define LL_WITHOUT_HOST_OFFSET 0x2C
//#define ROLE_OFFSET 0x2D

/*exported define*/
//#define BLUE_FLAG_RAM_RESET             0x01010101

#define HCI_CMD_FLAG 0x01 ////antonio
//uint8_t mask_events[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10}; //antonio
//uint8_t le_mask_events[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //antonio

namespace ble {
namespace vendor {
namespace bluenrg {

/**
 * BlueNRG HCI driver implementation.
 * @see cordio::CordioHCIDriver
 */
class HCIDriver : public cordio::CordioHCIDriver
{
public:
    /**
     * Construction of the BlueNRG HCIDriver.
     * @param transport: Transport of the HCI commands.
     * @param rst: Name of the reset pin
     */
    HCIDriver(cordio::CordioHCITransportDriver& transport_driver) :
        cordio::CordioHCIDriver(transport_driver) { }

    /**
     * @see CordioHCIDriver::do_initialize
     */
    virtual void do_initialize() {
//    	/*enable link layer only */
//    	uint8_t Value = 1;
//    	aci_hal_write_config_data(0x2C, 1, &Value);
    }

    /**
     * @see CordioHCIDriver::start_reset_sequence
     */
    virtual void start_reset_sequence() {
        /* send an HCI Reset command to start the sequence */
        HciResetCmd();
    }

    /**
     * @see CordioHCIDriver::do_terminate
     */
    virtual void do_terminate() {

    }

    /**
     * @see CordioHCIDriver::handle_reset_sequence
     */
    virtual void handle_reset_sequence(uint8_t *pMsg) {
        uint16_t       opcode;
        static uint8_t randCnt;
        //wait_ms(5);

        /* if event is a command complete event */
        if (*pMsg != HCI_CMD_CMPL_EVT)
        	return;

        /* parse parameters */
        pMsg += HCI_EVT_HDR_LEN;
        pMsg++;                   /* skip num packets */
        BSTREAM_TO_UINT16(opcode, pMsg);
        pMsg++;                   /* skip status */

        /* decode opcode */
        switch (opcode)
        {
        case HCI_OPCODE_RESET: {
        	/* initialize rand command count */
        	randCnt = 0;
        	// important, the bluenrg_initialized event come after the
        	// hci reset event (not documented), but in this initialization
        	// procedure it is bypassed

        	/*enable link layer only */
//        	uint8_t Value = 1;
//        	aci_hal_write_config_data(0x2C, 1, &Value);
        	//aci_gatt_init();
        	aciReadConfigParameter(RANDOM_STATIC_ADDRESS_OFFSET);
        } break;

        // ACL packet ...
/*
        case ACI_WRITE_CONFIG_DATA_OPCODE:   //DEPRECATED
        	if (enable_link_layer_mode_ongoing) {
        		enable_link_layer_mode_ongoing = false;
        		//aciSetRole();
        		aciReadConfigParameter(RANDOM_STATIC_ADDRESS_OFFSET);
        	} else {
        		aciGattInit();
        		aciReadConfigParameter(RANDOM_STATIC_ADDRESS_OFFSET);
        	}
        	break;
*/
/*
        case ACI_GATT_INIT_OPCODE:           //DEPRECATED
        	aciGapInit();
        	break;

        case ACI_GAP_INIT_OPCODE:            //DEPRECATED
        	aciReadConfigParameter(RANDOM_STATIC_ADDRESS_OFFSET);
        	break;
*/
        case ACI_READ_CONFIG_DATA_OPCODE:
        	// note: will send the HCI command to send the random address
        	cordio::BLE::deviceInstance().getGap().setAddress(
        			BLEProtocol::AddressType::RANDOM_STATIC,
					pMsg
        	);
        	break;

        case HCI_OPCODE_LE_SET_RAND_ADDR:
//        	HciSetEventMaskCmd((uint8_t *) mask_events);
        	HciSetEventMaskCmd((uint8_t *) hciEventMask_BNRG2);
        	break;

        case HCI_OPCODE_SET_EVENT_MASK:
        	/* send next command in sequence */
//        	HciLeSetEventMaskCmd((uint8_t *) le_mask_events);
        	HciLeSetEventMaskCmd((uint8_t *) hciLeEventMask_BNRG2);
        	break;

        case HCI_OPCODE_LE_SET_EVENT_MASK:
        	// Note: the public address is not read because there is no valid public address
        	// provisioned by default on the target
        	// Enable if the
#if MBED_CONF_CORDIO_BLUENRG_VALID_PUBLIC_BD_ADDRESS == 1
        	/* send next command in sequence */
        	HciReadBdAddrCmd();
        	break;

        case HCI_OPCODE_READ_BD_ADDR:
        	/* parse and store event parameters */
        	BdaCpy(hciCoreCb.bdAddr, pMsg);

        	/* send next command in sequence */
#endif
        	HciLeReadBufSizeCmd();
        	break;

        case HCI_OPCODE_LE_READ_BUF_SIZE:
        	/* parse and store event parameters */
        	BSTREAM_TO_UINT16(hciCoreCb.bufSize, pMsg);
        	BSTREAM_TO_UINT8(hciCoreCb.numBufs, pMsg);

        	/* initialize ACL buffer accounting */
        	hciCoreCb.availBufs = hciCoreCb.numBufs;

        	/* send next command in sequence */
        	HciLeReadSupStatesCmd();
        	break;

        case HCI_OPCODE_LE_READ_SUP_STATES:
        	/* parse and store event parameters */
        	memcpy(hciCoreCb.leStates, pMsg, HCI_LE_STATES_LEN);

        	/* send next command in sequence */
        	HciLeReadWhiteListSizeCmd();
        	break;

        case HCI_OPCODE_LE_READ_WHITE_LIST_SIZE:
        	/* parse and store event parameters */
        	BSTREAM_TO_UINT8(hciCoreCb.whiteListSize, pMsg);

        	/* send next command in sequence */
        	HciLeReadLocalSupFeatCmd();
        	break;

        case HCI_OPCODE_LE_READ_LOCAL_SUP_FEAT:
        	/* parse and store event parameters */
        	BSTREAM_TO_UINT16(hciCoreCb.leSupFeat, pMsg);

        	/* send next command in sequence */
        	hciCoreReadResolvingListSize();
        	break;

        case HCI_OPCODE_LE_READ_RES_LIST_SIZE:
        	/* parse and store event parameters */
        	BSTREAM_TO_UINT8(hciCoreCb.resListSize, pMsg);

        	/* send next command in sequence */
        	hciCoreReadMaxDataLen();
        	break;

        case HCI_OPCODE_LE_READ_MAX_DATA_LEN:
        {
        	uint16_t maxTxOctets;
        	uint16_t maxTxTime;

        	BSTREAM_TO_UINT16(maxTxOctets, pMsg);
        	BSTREAM_TO_UINT16(maxTxTime, pMsg);

        	/* use Controller's maximum supported payload octets and packet duration times
        	 * for transmission as Host's suggested values for maximum transmission number
        	 * of payload octets and maximum packet transmission time for new connections.
        	 */
        	HciLeWriteDefDataLen(maxTxOctets, maxTxTime);
        }
        break;

        case HCI_OPCODE_LE_WRITE_DEF_DATA_LEN:
        	if (hciCoreCb.extResetSeq)
        	{
        		/* send first extended command */
        		(*hciCoreCb.extResetSeq)(pMsg, opcode);
        	}
        	else
        	{
        		/* initialize extended parameters */
        		hciCoreCb.maxAdvDataLen = 0;
        		hciCoreCb.numSupAdvSets = 0;
        		hciCoreCb.perAdvListSize = 0;

        		/* send next command in sequence */
        		HciLeRandCmd();
        	}
        	break;

        case HCI_OPCODE_LE_READ_MAX_ADV_DATA_LEN:
        case HCI_OPCODE_LE_READ_NUM_SUP_ADV_SETS:
        case HCI_OPCODE_LE_READ_PER_ADV_LIST_SIZE:
        	if (hciCoreCb.extResetSeq)
        	{
        		/* send next extended command in sequence */
        		(*hciCoreCb.extResetSeq)(pMsg, opcode);
        	}
        	break;

        case HCI_OPCODE_LE_RAND:
        	/* check if need to send second rand command */
        	if (randCnt < (HCI_RESET_RAND_CNT-1))
        	{
        		randCnt++;
        		HciLeRandCmd();
        	}
        	else
        	{
        		signal_reset_sequence_done();
        	}
        	break;

        default:
        	break;
        }
    }

private:
    void aciEnableLinkLayerModeOnly() {
        uint8_t data[1] = { 0x01 };
        //enable_link_layer_mode_ongoing = true;
        aciWriteConfigData(LL_WITHOUT_HOST_OFFSET, data);
    }

    void aciSetRole() {
        // master and slave, simultaneous advertising and scanning
        // (up to 4 connections)
        uint8_t data[1] = { 0x04 };
        //aciWriteConfigData(ROLE_OFFSET, data);
    }
/*
    void aciGattInit() {
        uint8_t *pBuf = hciCmdAlloc(ACI_GATT_INIT_OPCODE, 0);
        if (!pBuf) {
            return;
        }
        hciCmdSend(pBuf);
    }
*/
/*
    void aciGapInit() {
        uint8_t *pBuf = hciCmdAlloc(ACI_GAP_INIT_OPCODE, 3);
        if (!pBuf) {
            return;
        }
        pBuf[3] = 0xF;
        pBuf[4] = 0;
        pBuf[5] = 0;
        hciCmdSend(pBuf);
    }
*/
    void aciReadConfigParameter(uint8_t offset) {
        uint8_t *pBuf = hciCmdAlloc(ACI_READ_CONFIG_DATA_OPCODE, 1);
        if (!pBuf) {
            return;
        }

        pBuf[3] = offset;
        hciCmdSend(pBuf);
    }

    template<size_t N>
    void aciWriteConfigData(uint8_t offset, uint8_t (&buf)[N]) {
        uint8_t *pBuf = hciCmdAlloc(ACI_WRITE_CONFIG_DATA_OPCODE, 2 + N);
        if (!pBuf) {
            return;
        }

        pBuf[3] = offset;
        pBuf[4] = N;
        memcpy(pBuf + 5, buf, N);
        hciCmdSend(pBuf);
    }

    void hciCoreReadResolvingListSize(void)
    {
        /* if LL Privacy is supported by Controller and included */
        if ((hciCoreCb.leSupFeat & HCI_LE_SUP_FEAT_PRIVACY) &&
            (hciLeSupFeatCfg & HCI_LE_SUP_FEAT_PRIVACY))
        {
            /* send next command in sequence */
            HciLeReadResolvingListSize();
        }
        else
        {
            hciCoreCb.resListSize = 0;

            /* send next command in sequence */
            hciCoreReadMaxDataLen();
        }
    }

    void hciCoreReadMaxDataLen(void)
    {
    /* if LE Data Packet Length Extensions is supported by Controller and included */
        if ((hciCoreCb.leSupFeat & HCI_LE_SUP_FEAT_DATA_LEN_EXT) &&
            (hciLeSupFeatCfg & HCI_LE_SUP_FEAT_DATA_LEN_EXT))
        {
            /* send next command in sequence */
            HciLeReadMaxDataLen();
        }
        else
        {
            /* send next command in sequence */
            HciLeRandCmd();
        }
    }
    /* this mask takes into account the inverted bit 61 on 2.0 DK */
    const uint8_t hciEventMask_BNRG2[HCI_EVT_MASK_LEN] =
    {
      HCI_EVT_MASK_DISCONNECT_CMPL |                  /* Byte 0 */
      HCI_EVT_MASK_ENC_CHANGE,                        /* Byte 0 */
      HCI_EVT_MASK_READ_REMOTE_VER_INFO_CMPL |        /* Byte 1 */
      HCI_EVT_MASK_HW_ERROR,                          /* Byte 1 */
      0,                                              /* Byte 2 */
      HCI_EVT_MASK_DATA_BUF_OVERFLOW,                 /* Byte 3 */
      0,                                              /* Byte 4 */
      HCI_EVT_MASK_ENC_KEY_REFRESH_CMPL,              /* Byte 5 */
      0,                                              /* Byte 6 */
      0         // HCI_EVT_MASK_LE_META                /* Byte 7 */   ///////// this bit should be 1, but it must be neglected to enable LE meta events
    };
    /* LE mask according to 4.2 specification */
    const uint8_t hciLeEventMask_BNRG2[HCI_LE_EVT_MASK_LEN] =
    {
      HCI_EVT_MASK_LE_CONN_CMPL_EVT |                 /* Byte 0 */
      HCI_EVT_MASK_LE_ADV_REPORT_EVT |                /* Byte 0 */
      HCI_EVT_MASK_LE_CONN_UPDATE_CMPL_EVT |          /* Byte 0 */
      HCI_EVT_MASK_LE_READ_REMOTE_FEAT_CMPL_EVT |     /* Byte 0 */
      HCI_EVT_MASK_LE_LTK_REQ_EVT |                   /* Byte 0 */
      HCI_EVT_MASK_LE_REMOTE_CONN_PARAM_REQ_EVT |     /* Byte 0 */
      HCI_EVT_MASK_LE_DATA_LEN_CHANGE_EVT |           /* Byte 0 */
      HCI_EVT_MASK_LE_READ_LOCAL_P256_PUB_KEY_CMPL,   /* Byte 0 */
      HCI_EVT_MASK_LE_GENERATE_DHKEY_CMPL |           /* Byte 1 */
      HCI_EVT_MASK_LE_ENHANCED_CONN_CMPL_EVT |        /* Byte 1 */
      HCI_EVT_MASK_LE_DIRECT_ADV_REPORT_EVT |         /* Byte 1 */
      HCI_EVT_MASK_LE_PHY_UPDATE_CMPL_EVT |           /* Byte 1 */
      HCI_EVT_MASK_LE_EXT_ADV_REPORT_EVT |            /* Byte 1 */
      HCI_EVT_MASK_LE_PER_ADV_SYNC_EST_EVT |          /* Byte 1 */
      HCI_EVT_MASK_LE_PER_ADV_REPORT_EVT |            /* Byte 1 */
      HCI_EVT_MASK_LE_PER_ADV_SYNC_LOST_EVT,          /* Byte 1 */
      HCI_EVT_MASK_LE_SCAN_TIMEOUT_EVT |              /* Byte 2 */
      HCI_EVT_MASK_LE_ADV_SET_TERM_EVT |              /* Byte 2 */
      HCI_EVT_MASK_LE_SCAN_REQ_RCVD_EVT |             /* Byte 2 */
      HCI_EVT_MASK_LE_CH_SEL_ALGO_EVT,                /* Byte 2 */
      0,                                              /* Byte 3 */
      0,                                              /* Byte 4 */
      0,                                              /* Byte 5 */
      0,                                              /* Byte 6 */
      0                                               /* Byte 7 */
    };
};

/**
 * Transport driver of the ST BlueNRG shield.
 * @important: With that driver, it is assumed that the SPI bus used is not shared
 * with other SPI peripherals. The reasons behind this choice are simplicity and
 * performance:
 *   - Reading from the peripheral SPI can be challenging especially if other
 *     threads access the same SPI bus. Indeed it is common that the function
 *     spiRead yield nothings even if the chip has signaled data with the irq
 *     line. Sharing would make the situation worse and increase the risk of
 *     timeout of HCI commands / response.
 *   - This driver can be used even if the RTOS is disabled or not present it may
 *     may be usefull for some targets.
 *
 * If The SPI is shared with other peripherals then the best option would be to
 * handle SPI read in a real time thread woken up by an event flag.
 *
 * Other mechanisms might also be added in the future to handle data read as an
 * event from the stack. This might not be the best solution for all BLE chip;
 * especially this one.
 */
class TransportDriver : public cordio::CordioHCITransportDriver {
public:
    /**
     * Construct the transport driver required by a BlueNRG module.
     * @param mosi Pin of the SPI mosi
     * @param miso Pin of the SPI miso
     * @param sclk Pin of the SPI clock
     * @param irq Pin used by the module to signal data are available.
     */
    TransportDriver() {
    }

    virtual ~TransportDriver(){}

    /**
     * @see CordioHCITransportDriver::initialize
     */
    virtual void initialize() {
    	/* System Init */
    	//DTM_SystemInit();

    	/* Stack Initialization */
    	DTM_StackInit();
    }

    /**
     * @see CordioHCITransportDriver::terminate
     */
    virtual void terminate() { }

    /**
     * @see CordioHCITransportDriver::write
     */
    virtual uint16_t write(uint8_t type, uint16_t len, uint8_t *pData) {
    	uint8_t resp_len;
    	if(type== HCI_CMD_FLAG){
    		buffer[0]=pData[0];  //command OCF, big endian
    		buffer[1]=pData[1];   //command OGF, big endian endian
    		for(int i=0; i<len; i++){
    			buffer[i] = pData[i];
    		}
    	}
    	else{
    		//should never enter here
    		while(1){}
    	}
    	resp_len = process_command(buffer, len, buffer_out, 255);
    	send_event(buffer_out, resp_len);
    	//se è un HCI reset bisogna inviare il secondo evento aci blue initialized event
    	////////ma SE non c'è spazio nella coda, per cui uso l'hci reset command complete event
    	////////per notificare al cordio l'avvenuto reset
//    	if(buffer[0] == 0x03 && buffer[1] == 0x0c){
//    		uint8_t aci_blue_initialize_event[6] = {0x04, VENDOR_SPECIFIC_EVENT, 3, 0x01, 0x00, 0x01};
//        	on_data_received(aci_blue_initialize_event, aci_blue_initialize_event[2] + 3);
//    	}

    	return len;
    }

private:
    uint16_t process_command(uint8_t *buffer_in, uint16_t buffer_in_length, uint8_t *buffer_out, uint16_t buffer_out_max_length){
    	uint32_t i;
    	  uint16_t ret_val, opCode;

    	  Osal_MemCpy(&opCode, buffer_in, 2);
    	  for (i = 0; i < (sizeof(hci_command_table)/sizeof(hci_command_table_type)); i++) {
    	    if (opCode == hci_command_table[i].opcode) {
    	      ret_val = hci_command_table[i].execute(buffer_in+2, buffer_in_length-2, buffer_out, buffer_out_max_length);
//    	      if (opCode == 0x0c03) {
//    	        // For HCI_RESET, set flag to issue a sys reset
//    	        ///////////////////////////////////////////////////////reset_pending = 1;
//    	      }
    	      //add get crash handler
    	      return ret_val;
    	    }
    	  }
    	  // Unknown command length
    	  buffer_out[0] = 0x04;
    	  buffer_out[1] = 0x0F;
    	  buffer_out[2] = 0x04;
    	  buffer_out[3] = 0x01;
    	  buffer_out[4] = 0x01;
    	  Osal_MemCpy(&buffer_out[5], &opCode, 2);
    	  return 7;
    }

    void send_event(uint8_t *buffer_out, uint16_t buffer_out_length)
    {
    	on_data_received(buffer_out, buffer_out_length);
    }

    uint8_t buffer[259], buffer_out[258];
    /* Exported constants --------------------------------------------------------*/

    //uint32_t __blueflag_RAM;
    //uint8_t reset_pending; //TBR
};

} // namespace bluenrg
} // namespace vendor
} // namespace ble

/**
 * Cordio HCI driver factory
 */
ble::vendor::cordio::CordioHCIDriver& ble_cordio_get_hci_driver() {
    static ble::vendor::bluenrg::TransportDriver transport_driver;
    static ble::vendor::bluenrg::HCIDriver hci_driver(transport_driver);
    return hci_driver;
}
