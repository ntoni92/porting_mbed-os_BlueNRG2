/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI command module.
 *
 *  Copyright (c) 2009-2018 Arm Ltd. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "wsf_queue.h"
#include "wsf_timer.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "hci_cmd.h"
#include "hci_tr.h"
#include "hci_api.h"
#include "hci_main.h"
#include "bluenrg1_api.h"
#include "DTM_cmd_db.h"

/**************************************************************************************************
  Macros
 **************************************************************************************************/

/* HCI command timeout in seconds */
#define HCI_CMD_TIMEOUT           2

/**************************************************************************************************
  Data Types
 **************************************************************************************************/

/* Local control block type */
typedef struct
{
	wsfTimer_t      cmdTimer;       /* HCI command timeout timer */
	wsfQueue_t      cmdQueue;       /* HCI command queue */
	uint16_t        cmdOpcode;      /* Opcode of last HCI command sent */
	uint8_t         numCmdPkts;     /* Number of outstanding HCI commands that can be sent */
} hciCmdCb_t;

/**************************************************************************************************
  Local Variables
 **************************************************************************************************/

/* Local control block */
hciCmdCb_t hciCmdCb;

//antonio variabili
uint8_t output_size;
uint8_t buffer_out[255];

/*************************************************************************************************/
/*!
 *  \brief  Allocate an HCI command buffer and set the command header fields.
 *
 *  \param  opcode  Command opcode.
 *  \param  len     length of command parameters.
 *
 *  \return Pointer to WSF msg buffer.
 */
/*************************************************************************************************/
uint8_t *hciCmdAlloc(uint16_t opcode, uint16_t len)
{
	uint8_t   *p;

	/* allocate buffer */
	if ((p = WsfMsgAlloc(len + HCI_CMD_HDR_LEN)) != NULL)
	{
		/* set HCI command header */
		UINT16_TO_BSTREAM(p, opcode);
		UINT8_TO_BSTREAM(p, len);
		p -= HCI_CMD_HDR_LEN;
	}

	return p;
}

/*************************************************************************************************/
/*!
 *  \brief  Send an HCI command and service the HCI command queue.
 *
 *  \param  pData  Buffer containing HCI command to send or NULL.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciCmdSend(uint8_t *pData)
{
	uint8_t         *p;
	wsfHandlerId_t  handlerId;

	/* queue command if present */
	if (pData != NULL)
	{
		/* queue data - message handler ID 'handerId' not used */
		WsfMsgEnq(&hciCmdCb.cmdQueue, 0, pData);
	}

	/* service the HCI command queue; first check if controller can accept any commands */
	if (hciCmdCb.numCmdPkts > 0)
	{
		/* if queue not empty */
		if ((p = WsfMsgDeq(&hciCmdCb.cmdQueue, &handlerId)) != NULL)
		{
			/* decrement controller command packet count */
			hciCmdCb.numCmdPkts--;

			/* store opcode of command we're sending */
			BYTES_TO_UINT16(hciCmdCb.cmdOpcode, p);

			/* start command timeout */
			WsfTimerStartSec(&hciCmdCb.cmdTimer, HCI_CMD_TIMEOUT);

			/* send command to transport */
			hciTrSendCmd(p);
		}
	}
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize the HCI cmd module.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciCmdInit(void)
{
	WSF_QUEUE_INIT(&hciCmdCb.cmdQueue);

	/* initialize numCmdPkts for special case of first command */
	hciCmdCb.numCmdPkts = 1;

	/* initialize timer */
	hciCmdCb.cmdTimer.msg.event = HCI_MSG_CMD_TIMEOUT;
	hciCmdCb.cmdTimer.handlerId = hciCb.handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief  Process an HCI command timeout.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciCmdTimeout(wsfMsgHdr_t *pMsg)
{
	HCI_TRACE_INFO0("hciCmdTimeout");
}

/*************************************************************************************************/
/*!
 *  \brief  Process an HCI Command Complete or Command Status event.
 *
 *  \param  numCmdPkts  Number of commands that can be sent to the controller.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciCmdRecvCmpl(uint8_t numCmdPkts)
{
	/* stop the command timeout timer */
	WsfTimerStop(&hciCmdCb.cmdTimer);

	/*
	 * Set the number of commands that can be sent to the controller.  Setting this
	 * to 1 rather than incrementing by numCmdPkts allows only one command at a time to
	 * be sent to the controller and simplifies the code.
	 */
	hciCmdCb.numCmdPkts = 1;

	/* send the next queued command */
	hciCmdSend(NULL);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI disconnect command.
 */
/*************************************************************************************************/
void HciDisconnectCmd(uint16_t handle, uint8_t reason)
{
	output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 3);

	*status = hci_disconnect(handle, reason);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0F;
	buffer_out[2] = output_size + 3;
	buffer_out[4] = 0x01;
	buffer_out[5] = 0x06;
	buffer_out[6] = 0x04;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE add device white list command.
 */
/*************************************************************************************************/
void HciLeAddDevWhiteListCmd(uint8_t addrType, uint8_t *pAddr)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_add_device_to_white_list(addrType /* 1 */, *pAddr /* 6 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x11;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE clear white list command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeClearWhiteListCmd(void)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_clear_white_list();
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x10;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI connection update command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeConnUpdateCmd(uint16_t handle, hciConnSpec_t *pConnSpec)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 3);

	*status = hci_le_connection_update(handle /* 2 */,
									   pConnSpec->connIntervalMin /* 2 */,
									   pConnSpec->connIntervalMax /* 2 */,
									   pConnSpec->connLatency /* 2 */,
									   pConnSpec->supTimeout /* 2 */,
									   pConnSpec->minCeLen /* 2 */,
									   pConnSpec->maxCeLen /* 2 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0F;
	buffer_out[2] = output_size + 3;
	buffer_out[4] = 0x01;
	buffer_out[5] = 0x13;
	buffer_out[6] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE create connection command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeCreateConnCmd(uint16_t scanInterval, uint16_t scanWindow, uint8_t filterPolicy,
		uint8_t peerAddrType, uint8_t *pPeerAddr, uint8_t ownAddrType,
		hciConnSpec_t *pConnSpec)
{
	  int output_size = 1;
	  /* Output params */
	  uint8_t *status = (uint8_t *) (buffer_out + 3);

	  *status = hci_le_create_connection(scanInterval /* 2 */,
	                                     scanWindow /* 2 */,
	                                     filterPolicy /* 1 */,
	                                     peerAddrType /* 1 */,
	                                     *pPeerAddr /* 6 */,
	                                     ownAddrType /* 1 */,
	                                     pConnSpec->connIntervalMin /* 2 */,
										 pConnSpec->connIntervalMax /* 2 */,
										 pConnSpec->connLatency /* 2 */,
										 pConnSpec->supTimeout /* 2 */,
	                                     pConnSpec->minCeLen /* 2 */,
	                                     pConnSpec->maxCeLen /* 2 */);
	  buffer_out[0] = 0x04;
	  buffer_out[1] = 0x0F;
	  buffer_out[2] = output_size + 3;
	  buffer_out[4] = 0x01;
	  buffer_out[5] = 0x0d;
	  buffer_out[6] = 0x20;
	  //return (output_size + 6);
	  onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE create connection cancel command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeCreateConnCancelCmd(void)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_create_connection_cancel();
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x0e;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE remote connection parameter request reply command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoteConnParamReqReply(uint16_t handle, uint16_t intervalMin, uint16_t intervalMax, uint16_t latency,
		uint16_t timeout, uint16_t minCeLen, uint16_t maxCeLen)
{
	/*
	uint8_t *pBuf;
	uint8_t *p;

	if ((pBuf = hciCmdAlloc(HCI_OPCODE_LE_REM_CONN_PARAM_REP, HCI_LEN_LE_REM_CONN_PARAM_REP)) != NULL)
	{
		p = pBuf + HCI_CMD_HDR_LEN;
		UINT16_TO_BSTREAM(p, handle);
		UINT16_TO_BSTREAM(p, intervalMin);
		UINT16_TO_BSTREAM(p, intervalMax);
		UINT16_TO_BSTREAM(p, latency);
		UINT16_TO_BSTREAM(p, timeout);
		UINT16_TO_BSTREAM(p, minCeLen);
		UINT16_TO_BSTREAM(p, maxCeLen);
		hciCmdSend(pBuf);
	}
	*/
	while(1){;}
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE remote connection parameter request negative reply command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoteConnParamReqNegReply(uint16_t handle, uint8_t reason)
{
	/*
	uint8_t *pBuf;
	uint8_t *p;

	if ((pBuf = hciCmdAlloc(HCI_OPCODE_LE_REM_CONN_PARAM_NEG_REP, HCI_LEN_LE_REM_CONN_PARAM_NEG_REP)) != NULL)
	{
		p = pBuf + HCI_CMD_HDR_LEN;
		UINT16_TO_BSTREAM(p, handle);
		UINT8_TO_BSTREAM(p, reason);
		hciCmdSend(pBuf);
	}
	*/
	while(1){;}
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set data len command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetDataLen(uint16_t handle, uint16_t txOctets, uint16_t txTime)
{
	int output_size = 1 + 2;
	/* Output params */
	hci_le_set_data_length_rp0 *rp0 = (hci_le_set_data_length_rp0 *) (buffer_out + 6);

	rp0->Status = hci_le_set_data_length(handle /* 2 */,
										 txOctets /* 2 */,
										 txTime /* 2 */);
	rp0->Connection_Handle = handle;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x22;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read suggested default data len command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadDefDataLen(void)
{
	int output_size = 1 + 2 + 2;
	/* Output params */
	hci_le_read_suggested_default_data_length_rp0 *rp0 = (hci_le_read_suggested_default_data_length_rp0 *) (buffer_out + 6);
	uint16_t SuggestedMaxTxOctets = 0;
	uint16_t SuggestedMaxTxTime = 0;

	rp0->Status = hci_le_read_suggested_default_data_length(&SuggestedMaxTxOctets,
			&SuggestedMaxTxTime);
	rp0->SuggestedMaxTxOctets = SuggestedMaxTxOctets;
	rp0->SuggestedMaxTxTime = SuggestedMaxTxTime;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x23;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE write suggested default data len command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeWriteDefDataLen(uint16_t suggestedMaxTxOctets, uint16_t suggestedMaxTxTime)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_write_suggested_default_data_length(suggestedMaxTxOctets /* 2 */,
														 suggestedMaxTxTime /* 2 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x24;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read local P-256 public key command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadLocalP256PubKey(void)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 3);

	*status = hci_le_read_local_p256_public_key();
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0F;
	buffer_out[2] = output_size + 3;
	buffer_out[4] = 0x01;
	buffer_out[5] = 0x25;
	buffer_out[6] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE generate DHKey command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeGenerateDHKey(uint8_t *pPubKeyX, uint8_t *pPubKeyY)
{
	////////antonio
	uint8_t Remote_P256_Public_Key[64];
	for(int i=0; i<32; i++){
		Remote_P256_Public_Key[i] = pPubKeyX[i];
		Remote_P256_Public_Key[32+i] = pPubKeyY[i];
	}
	////////antonio
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 3);

	*status = hci_le_generate_dhkey(Remote_P256_Public_Key /* 64 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0F;
	buffer_out[2] = output_size + 3;
	buffer_out[4] = 0x01;
	buffer_out[5] = 0x26;
	buffer_out[6] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read maximum data len command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadMaxDataLen(void)
{
	int output_size = 1 + 2 + 2 + 2 + 2;
	/* Output params */
	hci_le_read_maximum_data_length_rp0 *rp0 = (hci_le_read_maximum_data_length_rp0 *) (buffer_out + 6);
	uint16_t supportedMaxTxOctets = 0;
	uint16_t supportedMaxTxTime = 0;
	uint16_t supportedMaxRxOctets = 0;
	uint16_t supportedMaxRxTime = 0;

	rp0->Status = hci_le_read_maximum_data_length(&supportedMaxTxOctets,
			&supportedMaxTxTime,
			&supportedMaxRxOctets,
			&supportedMaxRxTime);
	rp0->supportedMaxTxOctets = supportedMaxTxOctets;
	rp0->supportedMaxTxTime = supportedMaxTxTime;
	rp0->supportedMaxRxOctets = supportedMaxRxOctets;
	rp0->supportedMaxRxTime = supportedMaxRxTime;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x2f;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE encrypt command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeEncryptCmd(uint8_t *pKey, uint8_t *pData)
{
	int output_size = 1 + 16;
	/* Output params */
	hci_le_encrypt_rp0 *rp0 = (hci_le_encrypt_rp0 *) (buffer_out + 6);
	uint8_t Encrypted_Data[16];

	rp0->Status = hci_le_encrypt(*pKey /* 16 */,
								 *pData /* 16 */,
								 Encrypted_Data);
	Osal_MemCpy((void *) rp0->Encrypted_Data,(const void *) Encrypted_Data, 16);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x17;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE long term key request negative reply command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeLtkReqNegReplCmd(uint16_t handle)
{
	int output_size = 1 + 2;
	/* Output params */
	hci_le_long_term_key_requested_negative_reply_rp0 *rp0 = (hci_le_long_term_key_requested_negative_reply_rp0 *) (buffer_out + 6);

	rp0->Status = hci_le_long_term_key_requested_negative_reply(handle /* 2 */);
	rp0->Connection_Handle = handle;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x1b;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE long term key request reply command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeLtkReqReplCmd(uint16_t handle, uint8_t *pKey)
{
	int output_size = 1 + 2;
	/* Output params */
	hci_le_long_term_key_request_reply_rp0 *rp0 = (hci_le_long_term_key_request_reply_rp0 *) (buffer_out + 6);

	rp0->Status = hci_le_long_term_key_request_reply(handle /* 2 */, *pKey /* 16 */);
	rp0->Connection_Handle = handle;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x1a;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE random command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRandCmd(void)
{
	int output_size = 1 + 8;
	/* Output params */
	hci_le_rand_rp0 *rp0 = (hci_le_rand_rp0 *) (buffer_out + 6);
	uint8_t Random_Number[8];

	rp0->Status = hci_le_rand(Random_Number);
	Osal_MemCpy((void *) rp0->Random_Number,(const void *) Random_Number, 8);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x18;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	//onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read advertising TX power command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadAdvTXPowerCmd(void)
{
	int output_size = 1 + 1;
	/* Output params */
	hci_le_read_advertising_channel_tx_power_rp0 *rp0 = (hci_le_read_advertising_channel_tx_power_rp0 *) (buffer_out + 6);
	uint8_t Transmit_Power_Level = 0;

	rp0->Status = hci_le_read_advertising_channel_tx_power(&Transmit_Power_Level);
	rp0->Transmit_Power_Level = Transmit_Power_Level;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x07;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read buffer size command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadBufSizeCmd(void)
{
	int output_size = 1 + 2 + 1;
	/* Output params */
	hci_le_read_buffer_size_rp0 *rp0 = (hci_le_read_buffer_size_rp0 *) (buffer_out + 6);
	uint16_t HC_LE_ACL_Data_Packet_Length = 0;
	uint8_t HC_Total_Num_LE_ACL_Data_Packets = 0;

	rp0->Status = hci_le_read_buffer_size(&HC_LE_ACL_Data_Packet_Length,
			&HC_Total_Num_LE_ACL_Data_Packets);
	rp0->HC_LE_ACL_Data_Packet_Length = HC_LE_ACL_Data_Packet_Length;
	rp0->HC_Total_Num_LE_ACL_Data_Packets = HC_Total_Num_LE_ACL_Data_Packets;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x02;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read channel map command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadChanMapCmd(uint16_t handle)
{
	int output_size = 1 + 2 + 5;
	/* Output params */
	hci_le_read_channel_map_rp0 *rp0 = (hci_le_read_channel_map_rp0 *) (buffer_out + 6);
	uint8_t LE_Channel_Map[5];

	rp0->Status = hci_le_read_channel_map(handle /* 2 */,
			LE_Channel_Map);
	rp0->Connection_Handle = handle;
	Osal_MemCpy((void *) rp0->LE_Channel_Map,(const void *) LE_Channel_Map, 5);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x15;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read local supported feautre command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadLocalSupFeatCmd(void)
{
	int output_size = 1 + 8;
	/* Output params */
	hci_le_read_local_supported_features_rp0 *rp0 = (hci_le_read_local_supported_features_rp0 *) (buffer_out + 6);
	uint8_t LE_Features[8];

	rp0->Status = hci_le_read_local_supported_features(LE_Features);
	Osal_MemCpy((void *) rp0->LE_Features,(const void *) LE_Features, 8);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x03;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read remote feature command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadRemoteFeatCmd(uint16_t handle)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 3);

	*status = hci_le_read_remote_used_features(handle /* 2 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0F;
	buffer_out[2] = output_size + 3;
	buffer_out[4] = 0x01;
	buffer_out[5] = 0x16;
	buffer_out[6] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read supported states command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadSupStatesCmd(void)
{
	int output_size = 1 + 8;
	/* Output params */
	hci_le_read_supported_states_rp0 *rp0 = (hci_le_read_supported_states_rp0 *) (buffer_out + 6);
	uint8_t LE_States[8];

	rp0->Status = hci_le_read_supported_states(LE_States);
	Osal_MemCpy((void *) rp0->LE_States,(const void *) LE_States, 8);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x1c;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read white list size command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadWhiteListSizeCmd(void)
{
	int output_size = 1 + 1;
	/* Output params */
	hci_le_read_white_list_size_rp0 *rp0 = (hci_le_read_white_list_size_rp0 *) (buffer_out + 6);
	uint8_t White_List_Size = 0;

	rp0->Status = hci_le_read_white_list_size(&White_List_Size);
	rp0->White_List_Size = White_List_Size;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x0f;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE remove device white list command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoveDevWhiteListCmd(uint8_t addrType, uint8_t *pAddr)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_remove_device_from_white_list(addrType /* 1 */, *pAddr /* 6 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x12;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set advanced enable command. //antonio advertise??
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAdvEnableCmd(uint8_t enable)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_set_advertise_enable(enable /* 1 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x0a;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set advertising data command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAdvDataCmd(uint8_t len, uint8_t *pData)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_set_advertising_data(len /* 1 */,
			*pData /* 31 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x08;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set advertising parameters command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAdvParamCmd(uint16_t advIntervalMin, uint16_t advIntervalMax, uint8_t advType,
		uint8_t ownAddrType, uint8_t peerAddrType, uint8_t *pPeerAddr,
		uint8_t advChanMap, uint8_t advFiltPolicy)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_set_advertising_parameters(advIntervalMin /* 2 */,
			advIntervalMax /* 2 */,
			advType /* 1 */,
			ownAddrType /* 1 */,
			peerAddrType /* 1 */,
			*pPeerAddr /* 6 */,
			advChanMap /* 1 */,
			advFiltPolicy /* 1 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x06;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set event mask command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetEventMaskCmd(uint8_t *pLeEventMask)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_set_event_mask(*pLeEventMask /* 8 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x01;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI set host channel class command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetHostChanClassCmd(uint8_t *pChanMap)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_set_host_channel_classification(*pChanMap /* 5 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x14;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set random address command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetRandAddrCmd(uint8_t *pAddr)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_set_random_address(*pAddr /* 6 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x05;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set scan enable command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetScanEnableCmd(uint8_t enable, uint8_t filterDup)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_set_scan_enable(enable /* 1 */,
			filterDup /* 1 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x0c;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI set scan parameters command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetScanParamCmd(uint8_t scanType, uint16_t scanInterval, uint16_t scanWindow,
		uint8_t ownAddrType, uint8_t scanFiltPolicy)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_set_scan_parameters(scanType /* 1 */,
			scanInterval /* 2 */,
			scanWindow /* 2 */,
			ownAddrType /* 1 */,
			scanFiltPolicy /* 1 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x0b;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set scan response data.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetScanRespDataCmd(uint8_t len, uint8_t *pData)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_set_scan_response_data(len /* 1 */,
			*pData /* 31 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x09;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI LE start encryption command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeStartEncryptionCmd(uint16_t handle, uint8_t *pRand, uint16_t diversifier, uint8_t *pKey)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 3);

	*status = hci_le_start_encryption(handle /* 2 */,
									  *pRand /* 8 */,
									  diversifier /* 2 */,
									  *pKey /* 16 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0F;
	buffer_out[2] = output_size + 3;
	buffer_out[4] = 0x01;
	buffer_out[5] = 0x19;
	buffer_out[6] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read BD address command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadBdAddrCmd(void)
{
	int output_size = 1 + 6;
	/* Output params */
	hci_read_bd_addr_rp0 *rp0 = (hci_read_bd_addr_rp0 *) (buffer_out + 6);
	uint8_t BD_ADDR[6];

	rp0->Status = hci_read_bd_addr(BD_ADDR);
	Osal_MemCpy((void *) rp0->BD_ADDR,(const void *) BD_ADDR, 6);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x09;
	buffer_out[5] = 0x10;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read buffer size command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadBufSizeCmd(void)
{
	/*
	uint8_t *pBuf;

	if ((pBuf = hciCmdAlloc(HCI_OPCODE_READ_BUF_SIZE, HCI_LEN_READ_BUF_SIZE)) != NULL)
	{
		hciCmdSend(pBuf);
	}
	*/
	while(1){;}
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read local supported feature command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadLocalSupFeatCmd(void)
{
	int output_size = 1 + 8;
	/* Output params */
	hci_read_local_supported_features_rp0 *rp0 = (hci_read_local_supported_features_rp0 *) (buffer_out + 6);
	uint8_t LMP_Features[8];

	rp0->Status = hci_read_local_supported_features(LMP_Features);
	Osal_MemCpy((void *) rp0->LMP_Features,(const void *) LMP_Features, 8);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x03;
	buffer_out[5] = 0x10;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read local version info command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadLocalVerInfoCmd(void)
{
	int output_size = 1 + 1 + 2 + 1 + 2 + 2;
	/* Output params */
	hci_read_local_version_information_rp0 *rp0 = (hci_read_local_version_information_rp0 *) (buffer_out + 6);
	uint8_t HCI_Version = 0;
	uint16_t HCI_Revision = 0;
	uint8_t LMP_PAL_Version = 0;
	uint16_t Manufacturer_Name = 0;
	uint16_t LMP_PAL_Subversion = 0;

	rp0->Status = hci_read_local_version_information(&HCI_Version,
			&HCI_Revision,
			&LMP_PAL_Version,
			&Manufacturer_Name,
			&LMP_PAL_Subversion);
	{
		PartInfoType partInfo;
		/* get partInfo */
		HAL_GetPartInfo(&partInfo);
		//  H[15:12]= Hardware cut major version
				//  h[11:8]= Hardware cut minor version
				//  C[7:4]= 0: BlueNRG/BlueNRG-MS, 1: BlueNRG-1; 2: BlueNRG-2;
				//  S[3:0]= Library major version
		/* Set HCI_Revision fields with information coming from HAL_GetPartInfo() */
		HCI_Revision = HCI_Revision & 0xF;
		HCI_Revision |= ((partInfo.die_major & 0xF) <<12) | ((partInfo.die_cut & 0xF) <<8) | ((partInfo.die_id & 0xF) <<4) ;
	}
	rp0->HCI_Version = HCI_Version;
	rp0->HCI_Revision = HCI_Revision;
	rp0->LMP_PAL_Version = LMP_PAL_Version;
	rp0->Manufacturer_Name = Manufacturer_Name;
	rp0->LMP_PAL_Subversion = LMP_PAL_Subversion;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x01;
	buffer_out[5] = 0x10;
	//return (output_size + 6);
	onDataReceived(buffer_out,output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read remote version info command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadRemoteVerInfoCmd(uint16_t handle)
{
	output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 3);

	*status = hci_read_remote_version_information(handle);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0F;
	buffer_out[2] = output_size + 3;
	buffer_out[4] = 0x01;
	buffer_out[5] = 0x1d;
	buffer_out[6] = 0x04;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read RSSI command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadRssiCmd(uint16_t handle)
{
	int output_size = 1 + 2 + 1;
	/* Output params */
	hci_read_rssi_rp0 *rp0 = (hci_read_rssi_rp0 *) (buffer_out + 6);
	uint8_t RSSI = 0;

	rp0->Status = hci_read_rssi(handle,
			&RSSI);
	rp0->Connection_Handle = handle;
	rp0->RSSI = RSSI;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x05;
	buffer_out[5] = 0x14;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read Tx power level command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadTxPwrLvlCmd(uint16_t handle, uint8_t type)
{
	output_size = 1 + 2 + 1;
	/* Output params */
	hci_read_transmit_power_level_rp0 *rp0 = (hci_read_transmit_power_level_rp0 *) (buffer_out + 6);
	uint8_t Transmit_Power_Level = 0;

	rp0->Status = hci_read_transmit_power_level(handle, type, &Transmit_Power_Level);
	rp0->Connection_Handle = handle;
	rp0->Transmit_Power_Level = Transmit_Power_Level;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x2d;
	buffer_out[5] = 0x0c;
	//return (output_size + 6);
	onDataReceived(buffer_out,output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI reset command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciResetCmd(void)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_reset();
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x03;
	buffer_out[5] = 0x0c;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI set event mask command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciSetEventMaskCmd(uint8_t *pEventMask)
{
	output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_set_event_mask(*pEventMask);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x01;
	buffer_out[5] = 0x0c;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI set event mask page 2 command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciSetEventMaskPage2Cmd(uint8_t *pEventMask)
{
	/*
	uint8_t *pBuf;
	uint8_t *p;

	if ((pBuf = hciCmdAlloc(HCI_OPCODE_SET_EVENT_MASK_PAGE2, HCI_LEN_SET_EVENT_MASK_PAGE2)) != NULL)
	{
		p = pBuf + HCI_CMD_HDR_LEN;
		memcpy(p, pEventMask, HCI_EVT_MASK_PAGE_2_LEN);
		hciCmdSend(pBuf);
	}
	*/
	while(1){;}
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read authenticated payload timeout command.
 *
 *  \param  handle    Connection handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadAuthPayloadTimeout(uint16_t handle)
{
	/*
	uint8_t *pBuf;
	uint8_t *p;

	if ((pBuf = hciCmdAlloc(HCI_OPCODE_READ_AUTH_PAYLOAD_TO, HCI_LEN_READ_AUTH_PAYLOAD_TO)) != NULL)
	{
		p = pBuf + HCI_CMD_HDR_LEN;
		UINT16_TO_BSTREAM(p, handle);
		hciCmdSend(pBuf);
	}
	*/
	while(1){;}
}

/*************************************************************************************************/
/*!
 *  \brief  HCI write authenticated payload timeout command.
 *
 *  \param  handle    Connection handle.
 *  \param  timeout   Timeout value.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciWriteAuthPayloadTimeout(uint16_t handle, uint16_t timeout)
{
	/*
	uint8_t *pBuf;
	uint8_t *p;

	if ((pBuf = hciCmdAlloc(HCI_OPCODE_WRITE_AUTH_PAYLOAD_TO, HCI_LEN_WRITE_AUTH_PAYLOAD_TO)) != NULL)
	{
		p = pBuf + HCI_CMD_HDR_LEN;
		UINT16_TO_BSTREAM(p, handle);
		UINT16_TO_BSTREAM(p, timeout);
		hciCmdSend(pBuf);
	}
	*/
	while(1){;}
}

/*************************************************************************************************/
/*!
 *  \brief  HCI add device to resolving list command.
 *
 *  \param  peerAddrType        Peer identity address type.
 *  \param  pPeerIdentityAddr   Peer identity address.
 *  \param  pPeerIrk            Peer IRK.
 *  \param  pLocalIrk           Local IRK.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeAddDeviceToResolvingListCmd(uint8_t peerAddrType, const uint8_t *pPeerIdentityAddr,
		const uint8_t *pPeerIrk, const uint8_t *pLocalIrk)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_add_device_to_resolving_list(peerAddrType /* 1 */,
												  *pPeerIdentityAddr /* 6 */,
												  pPeerIrk /* 16 */,
												  pLocalIrk /* 16 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x27;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI remove device from resolving list command.
 *
 *  \param  peerAddrType        Peer identity address type.
 *  \param  pPeerIdentityAddr   Peer identity address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoveDeviceFromResolvingList(uint8_t peerAddrType, const uint8_t *pPeerIdentityAddr)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_remove_device_from_resolving_list(peerAddrType /* 1 */,
													   *pPeerIdentityAddr /* 6 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x28;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI clear resolving list command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeClearResolvingList(void)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_clear_resolving_list();
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x29;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read resolving list command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadResolvingListSize(void)
{
	int output_size = 1 + 1;
	/* Output params */
	hci_le_read_resolving_list_size_rp0 *rp0 = (hci_le_read_resolving_list_size_rp0 *) (buffer_out + 6);
	uint8_t Resolving_List_Size = 0;

	rp0->Status = hci_le_read_resolving_list_size(&Resolving_List_Size);
	rp0->Resolving_List_Size = Resolving_List_Size;
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x2a;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read peer resolvable address command.
 *
 *  \param  addrType        Peer identity address type.
 *  \param  pIdentityAddr   Peer identity address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadPeerResolvableAddr(uint8_t addrType, const uint8_t *pIdentityAddr)
{
	int output_size = 1 + 6;
	/* Output params */
	hci_le_read_peer_resolvable_address_rp0 *rp0 = (hci_le_read_peer_resolvable_address_rp0 *) (buffer_out + 6);
	uint8_t Peer_Resolvable_Address[6];

	rp0->Status = hci_le_read_peer_resolvable_address(addrType /* 1 */,
													  *pIdentityAddr /* 6 */,
													  Peer_Resolvable_Address);
	Osal_MemCpy((void *) rp0->Peer_Resolvable_Address,(const void *) Peer_Resolvable_Address, 6);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x2b;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI read local resolvable address command.
 *
 *  \param  addrType        Peer identity address type.
 *  \param  pIdentityAddr   Peer identity address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadLocalResolvableAddr(uint8_t addrType, const uint8_t *pIdentityAddr)
{
	int output_size = 1 + 6;
	/* Output params */
	hci_le_read_local_resolvable_address_rp0 *rp0 = (hci_le_read_local_resolvable_address_rp0 *) (buffer_out + 6);
	uint8_t Local_Resolvable_Address[6];

	rp0->Status = hci_le_read_local_resolvable_address(addrType /* 1 */,
													   *pIdentityAddr /* 6 */,
													   Local_Resolvable_Address);
	Osal_MemCpy((void *) rp0->Local_Resolvable_Address,(const void *) Local_Resolvable_Address, 6);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x2c;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI enable or disable address resolution command.
 *
 *  \param  enable      Set to TRUE to enable address resolution or FALSE to disable address
 *                      resolution.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAddrResolutionEnable(uint8_t enable)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_set_address_resolution_enable(enable /* 1 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x2d;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief  HCI set resolvable private address timeout command.
 *
 *  \param  rpaTimeout    Timeout measured in seconds.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetResolvablePrivateAddrTimeout(uint16_t rpaTimeout)
{
	int output_size = 1;
	/* Output params */
	uint8_t *status = (uint8_t *) (buffer_out + 6);

	*status = hci_le_set_resolvable_private_address_timeout(rpaTimeout /* 2 */);
	buffer_out[0] = 0x04;
	buffer_out[1] = 0x0E;
	buffer_out[2] = output_size + 3;
	buffer_out[3] = 0x01;
	buffer_out[4] = 0x2e;
	buffer_out[5] = 0x20;
	//return (output_size + 6);
	onDataReceived(buffer_out, output_size + 6);
}

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set privacy mode command.
 *
 *  \param      peerAddrType    Peer identity address type.
 *  \param      pPeerAddr       Peer identity address.
 *  \param      mode            Privacy mode.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetPrivacyModeCmd(uint8_t addrType, uint8_t *pAddr, uint8_t mode)
{
	/*
	uint8_t *pBuf;
	uint8_t *p;

	if ((pBuf = hciCmdAlloc(HCI_OPCODE_LE_SET_PRIVACY_MODE, HCI_LEN_LE_SET_PRIVACY_MODE)) != NULL)
	{
		p = pBuf + HCI_CMD_HDR_LEN;
		UINT8_TO_BSTREAM(p, addrType);
		BDA_TO_BSTREAM(p, pAddr);
		UINT8_TO_BSTREAM(p, mode);
		hciCmdSend(pBuf);
	}
	*/
	while(1){;}
}

/*************************************************************************************************/
/*!
 *  \brief  HCI vencor specific command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciVendorSpecificCmd(uint16_t opcode, uint8_t len, uint8_t *pData)
{
	/*
	uint8_t *pBuf;
	uint8_t *p;

	if ((pBuf = hciCmdAlloc(opcode, len)) != NULL)
	{
		p = pBuf + HCI_CMD_HDR_LEN;
		memcpy(p, pData, len);
		hciCmdSend(pBuf);
	}
	*/
	while(1){;}
}
