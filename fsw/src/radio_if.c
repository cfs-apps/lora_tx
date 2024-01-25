/*
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU Lesser General Public License as
**  published by the Free Software Foundation, either version 3 of the
**  License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Lesser General Public License for more details.
**
**  You should have received a copy of the GNU Lesser General Public License
**  along with this program.  If not, see <https://www.gnu.org/licenses/>.
**
**  Purpose:
**    Implement the Transmit Demo Class methods
**
**  Notes:
**    1. Refactored Stanford's Lora_tx.cpp functions into this object that
**       runs within the context of a cFS Basecamp app. This object provides
**       demo configuration and operations commands and uses a child task
**       to manage a transfer. 
**    2. Bridges SX128X C++ library and the main app and Basecamp's app_c_fw
**       written in C.  
**
*/

/*
** Include Files:
*/

#include <string.h>
#include "app_cfg.h"
#include "radio_if.h"
#include "radio_tx.h"


/**********************/
/** Global File Data **/
/**********************/

static RADIO_IF_Class_t *RadioIf = NULL;


/*******************************/
/** Local Function Prototypes **/
/*******************************/


/******************************************************************************
** Function: RADIO_IF_Constructor
**
** Initialize the Radio Interface object to a known state
**
** Notes:
**   1. This must be called prior to any other function.
**
*/
void RADIO_IF_Constructor(RADIO_IF_Class_t *RadioIfPtr, INITBL_Class_t *IniTbl)
{
   
   RadioIf = RadioIfPtr;
   
   memset(RadioIf, 0, sizeof(RADIO_IF_Class_t));
   
   RadioIf->IniTbl = IniTbl;
   RadioIf->Initialized = false;
   RadioIf->SpiSpeed = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_SPI_SPEED);
   
   RadioIf->RadioConfig.Frequency = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_FREQUENCY);
   
   RadioIf->RadioConfig.LoRa.SpreadingFactor = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_LORA_SF);
   RadioIf->RadioConfig.LoRa.Bandwidth       = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_LORA_BW);
   RadioIf->RadioConfig.LoRa.CodingRate      = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_LORA_CR);
      
   CFE_MSG_Init(CFE_MSG_PTR(RadioIf->RadioTlm.TelemetryHeader), CFE_SB_ValueToMsgId(INITBL_GetIntConfig(RadioIf->IniTbl, CFG_LORA_TX_RADIO_TLM_TOPICID)), sizeof(LORA_TX_RadioTlm_t));

} /* End RADIO_IF_Constructor() */


/******************************************************************************
** Function: RADIO_IF_ChildTask
**
** Notes:
**   1. Returning false causes the child task to terminate.
**   2. Information events are sent because this is instructional code and the
**      events provide feedback. The events are filtered so they won't flood
**      the ground. A reset app command resets the event filter.  
**
*/
bool RADIO_IF_ChildTask(CHILDMGR_Class_t *ChildMgr)
{
   
   bool RetStatus = true;
 
   CFE_EVS_SendEvent (RADIO_IF_CHILD_TASK_EID, CFE_EVS_EventType_INFORMATION, 
                      "RADIO_IF_ChildTask()"); 
   OS_TaskDelay(2000);
       
   return RetStatus;

} /* End RADIO_IF_ChildTask() */


/******************************************************************************
** Function: RADIO_IF_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
** Notes:
**   1. Any counter or variable that is reported in HK telemetry that doesn't
**      change the functional behavior should be reset.
**
*/
void RADIO_IF_ResetStatus(void)
{

   return;

} /* End RADIO_IF_ResetStatus() */


/******************************************************************************
** Function: RADIO_IF_InitRadio
**
*/
bool RADIO_IF_InitRadioCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   bool RetStatus = false;
   RADIO_TX_Pin_t RadioPin;

   RadioPin.Busy = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_BUSY);
   RadioPin.Nrst = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_NRST);
   RadioPin.Nss  = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_NSS);
   RadioPin.Dio1 = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_DIO1);
   RadioPin.Dio2 = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_DIO2);
   RadioPin.Dio3 = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_DIO3);
   RadioPin.TxEn = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_TX_EN);
   RadioPin.RxEn = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_RX_EN);
   
   RetStatus = RADIO_TX_InitRadio(INITBL_GetStrConfig(RadioIf->IniTbl, CFG_RADIO_SPI_DEV_STR),
                                  INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_SPI_DEV_NUM),
                                  &RadioPin);
   
   if (RetStatus)
   {
      RadioIf->Initialized = true;
      CFE_EVS_SendEvent(RADIO_TX_INIT_RADIO_CMD_EID, CFE_EVS_EventType_INFORMATION,
                        "Sucessfully initialized the Radio");
   }
   else
   {
      RadioIf->Initialized = false;
      CFE_EVS_SendEvent(RADIO_TX_INIT_RADIO_CMD_EID, CFE_EVS_EventType_ERROR,
                        "Failed to initialize the Radio");
   }

   return RetStatus;
   
} /* RADIO_IF_InitRadioCmd() */


/******************************************************************************
** Function: RADIO_IF_SendRadioTlmCmd
**
** Notes:
**   1. See radio_if.h file prologue for data source details.
*/
bool RADIO_IF_SendRadioTlmCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   LORA_TX_RadioTlm_Payload_t *RadioTlmPayload = &RadioIf->RadioTlm.Payload;
   
   strncpy(RadioTlmPayload->SpiDevStr, INITBL_GetStrConfig(RadioIf->IniTbl, CFG_RADIO_SPI_DEV_STR), OS_MAX_PATH_LEN - 1);   
   RadioTlmPayload->SpiDevNum      = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_SPI_DEV_NUM);
   RadioTlmPayload->SpiSpeed       = RadioIf->SpiSpeed;
   RadioTlmPayload->RadioPinBusy   = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_BUSY);
   RadioTlmPayload->RadioPinNrst   = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_NRST);
   RadioTlmPayload->RadioPinNss    = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_NSS);
   RadioTlmPayload->RadioPinDio1   = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_DIO1);
   RadioTlmPayload->RadioPinDio2   = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_DIO2);
   RadioTlmPayload->RadioPinDio3   = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_DIO3);
   RadioTlmPayload->RadioPinTxEn   = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_TX_EN);
   RadioTlmPayload->RadioPinRxEn   = INITBL_GetIntConfig(RadioIf->IniTbl, CFG_RADIO_PIN_RX_EN);

   RadioTlmPayload->RadioFrequency      = RadioIf->RadioConfig.Frequency;
   RadioTlmPayload->LoRaSpreadingFactor = RadioIf->RadioConfig.LoRa.SpreadingFactor;
   RadioTlmPayload->LoRaBandwidth       = RadioIf->RadioConfig.LoRa.Bandwidth;
   RadioTlmPayload->LoRaCodingRate      = RadioIf->RadioConfig.LoRa.CodingRate;
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(RadioIf->RadioTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(RadioIf->RadioTlm.TelemetryHeader), true);

   CFE_EVS_SendEvent(RADIO_TX_SEND_RADIO_TLM_CMD_EID, CFE_EVS_EventType_INFORMATION,
                     "Sent Radio configuration telemetry message");
   return true;
   
} /* RADIO_IF_SendRadioTlmCmd() */


/******************************************************************************
** Function: RADIO_IF_SetLoRaParamsCmd
**
** Notes:
**   1. TODO: Add parameter validity checks
*/
bool RADIO_IF_SetLoRaParamsCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const LORA_TX_SetLoRaParams_CmdPayload_t *Cmd = CMDMGR_PAYLOAD_PTR(MsgPtr, LORA_TX_SetLoRaParams_t);
   bool RetStatus = false;

   if (RadioIf->Initialized)
   {
      RadioIf->RadioConfig.LoRa.SpreadingFactor = Cmd->SpreadingFactor;
      RadioIf->RadioConfig.LoRa.Bandwidth       = Cmd->Bandwidth;
      RadioIf->RadioConfig.LoRa.CodingRate      = Cmd->CodingRate;
      RADIO_TX_SetLoraParams(RadioIf->RadioConfig.LoRa.SpreadingFactor,
                             RadioIf->RadioConfig.LoRa.Bandwidth,
                             RadioIf->RadioConfig.LoRa.CodingRate);
      CFE_EVS_SendEvent(RADIO_TX_SET_LORA_PARAMS_CMD_EID, CFE_EVS_EventType_INFORMATION,
                        "Set LoRa paramaters: SF=%d, BW=%d, RC=%d", Cmd->SpreadingFactor,
                        Cmd->Bandwidth, Cmd->CodingRate);
                        
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(RADIO_TX_SET_LORA_PARAMS_CMD_EID, CFE_EVS_EventType_ERROR,
                        "Set LoRa parameters failed, Radio not initialized");
   }

   return RetStatus;
   
} /* RADIO_IF_SetLoRaParamsCmd() */


/******************************************************************************
** Function: RADIO_IF_SetRadioFrequencyCmd
**
** Notes:
**   1. TODO: What is a valid frequency range
*/
bool RADIO_IF_SetRadioFrequencyCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const LORA_TX_SetRadioFrequency_CmdPayload_t *Cmd = CMDMGR_PAYLOAD_PTR(MsgPtr, LORA_TX_SetRadioFrequency_t);
   bool RetStatus = false;

   if (Cmd->Frequency >= 0 && Cmd->Frequency <= 48000)
   {
      if (RadioIf->Initialized)
      {
         RadioIf->RadioConfig.Frequency = Cmd->Frequency;
         RADIO_TX_SetRadioFrequency(Cmd->Frequency*1000000UL);
         CFE_EVS_SendEvent(RADIO_TX_SET_RADIO_FREQUENCY_CMD_EID, CFE_EVS_EventType_INFORMATION,
                           "Set radio frequency to %d Mhz", Cmd->Frequency);
         RetStatus = true;
      }
      else
      {
         CFE_EVS_SendEvent(RADIO_TX_SET_RADIO_FREQUENCY_CMD_EID, CFE_EVS_EventType_ERROR,
                           "Set radio frequency failed, Radio not initialized");
      }

   }
   else
   {
      CFE_EVS_SendEvent(RADIO_TX_SET_RADIO_FREQUENCY_CMD_EID, CFE_EVS_EventType_ERROR,
                           "Set radio frequency failed, invalid frequency %d.",
                           Cmd->Frequency);
   }

   return RetStatus;
   
} /* RADIO_IF_SetRadioFrequencyCmd() */


/******************************************************************************
** Function: RADIO_IF_SetSpiSpeedCmd
**
** Notes:
**   1. TODO: What is a valid speed range
*/
bool RADIO_IF_SetSpiSpeedCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const LORA_TX_SetSpiSpeed_CmdPayload_t *Cmd = CMDMGR_PAYLOAD_PTR(MsgPtr, LORA_TX_SetSpiSpeed_t);
   bool RetStatus = false;

   if (Cmd->Speed >= 0 && Cmd->Speed <= 8000000)
   {
      if (RadioIf->Initialized)
      {
         RadioIf->SpiSpeed = Cmd->Speed;
         RADIO_TX_SetSpiSpeed(Cmd->Speed);
         CFE_EVS_SendEvent(RADIO_TX_SET_SPI_SPEED_CMD_EID, CFE_EVS_EventType_INFORMATION,
                           "Set radio SPI speed to %d", Cmd->Speed);
         RetStatus = true;
      }
      else
      {
         CFE_EVS_SendEvent(RADIO_TX_SET_SPI_SPEED_CMD_EID, CFE_EVS_EventType_ERROR,
                           "Set radio SPI speed failed, Radio not initialized");
      }

   }
   else
   {
      CFE_EVS_SendEvent(RADIO_TX_SET_SPI_SPEED_CMD_EID, CFE_EVS_EventType_ERROR,
                           "Set radio SPI speed failed, invalid speed %d.",
                           Cmd->Speed);
   }

   return RetStatus;
   
} /* RADIO_IF_SetSpiSpeedCmd() */
