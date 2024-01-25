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
**    Implement the LoRa Transmit application
**
**  Notes:
**    1. See lora_tx_app.h for details.
**
*/

/*
** Includes
*/

#include <string.h>
#include "lora_tx_app.h"
#include "lora_tx_eds_cc.h"

/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ   (&(LoraTx.IniTbl))
#define  CMDMGR_OBJ   (&(LoraTx.CmdMgr))
#define  CHILDMGR_OBJ (&(LoraTx.ChildMgr))
#define  RADIO_IF_OBJ (&(LoraTx.RadioIf))


/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static int32 ProcessCommands(void);
static void SendStatusTlm(void);


/**********************/
/** File Global Data **/
/**********************/

/* 
** Must match DECLARE ENUM() declaration in app_cfg.h
** Defines "static INILIB_CfgEnum IniCfgEnum"
*/
DEFINE_ENUM(Config,APP_CONFIG)  


static CFE_EVS_BinFilter_t  EventFilters[] =
{  
   /* Event ID                  Mask */
   {RADIO_IF_CHILD_TASK_EID,   CFE_EVS_FIRST_4_STOP}  // Use CFE_EVS_NO_FILTER to see all events

};


/*****************/
/** Global Data **/
/*****************/

LORA_TX_Class_t  LoraTx;

/******************************************************************************
** Function: LORA_TX_AppMain
**
*/
void LORA_TX_AppMain(void)
{

   uint32 RunStatus = CFE_ES_RunStatus_APP_ERROR;


   CFE_EVS_Register(EventFilters, sizeof(EventFilters)/sizeof(CFE_EVS_BinFilter_t),
                    CFE_EVS_EventFilter_BINARY);

   if (InitApp() == CFE_SUCCESS) /* Performs initial CFE_ES_PerfLogEntry() call */
   {  
   
      RunStatus = CFE_ES_RunStatus_APP_RUN;
      
   }
   
   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus))
   {

      RunStatus = ProcessCommands(); /* Pends indefinitely & manages CFE_ES_PerfLogEntry() calls */

   } /* End CFE_ES_RunLoop */

   CFE_ES_WriteToSysLog("LORA_TX App terminating, err = 0x%08X\n", RunStatus);   /* Use SysLog, events may not be working */

   CFE_EVS_SendEvent(LORA_TX_EXIT_EID, CFE_EVS_EventType_CRITICAL, "LORA_TX App terminating, err = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */

} /* End of LORA_TX_AppMain() */


/******************************************************************************
** Function: LORA_TX_NoOpCmd
**
*/

bool LORA_TX_NoOpCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_SendEvent (LORA_TX_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for LORA_TX App version %d.%d.%d",
                      LORA_TX_MAJOR_VER, LORA_TX_MINOR_VER, LORA_TX_PLATFORM_REV);

   return true;


} /* End LORA_TX_NoOpCmd() */


/******************************************************************************
** Function: LORA_TX_ResetAppCmd
**
** Notes:
**   1. No need to pass an object reference to contained objects because they
**      already have a reference from when they were constructed
**
*/

bool LORA_TX_ResetAppCmd(void* ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_ResetAllFilters();
   
   CMDMGR_ResetStatus(CMDMGR_OBJ);
   CHILDMGR_ResetStatus(CHILDMGR_OBJ);
   
   RADIO_IF_ResetStatus();
	  
   return true;

} /* End LORA_TX_ResetAppCmd() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 Status = APP_C_FW_CFS_ERROR;
   
   CHILDMGR_TaskInit_t ChildTaskInit;
   
   /*
   ** Initialize objects 
   */

   if (INITBL_Constructor(&LoraTx.IniTbl, LORA_TX_INI_FILENAME, &IniCfgEnum))
   {

      LoraTx.PerfId   = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_PERF_ID);
      LoraTx.CmdMid   = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_LORA_TX_CMD_TOPICID));
      LoraTx.OneHzMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_BC_SCH_1_HZ_TOPICID));
      
      CFE_ES_PerfLogEntry(LoraTx.PerfId);

      /* Constructor sends error events */
      ChildTaskInit.TaskName  = INITBL_GetStrConfig(INITBL_OBJ, CFG_CHILD_NAME);
      ChildTaskInit.PerfId    = INITBL_GetIntConfig(INITBL_OBJ, CFG_CHILD_PERF_ID);
      ChildTaskInit.StackSize = INITBL_GetIntConfig(INITBL_OBJ, CFG_CHILD_STACK_SIZE);
      ChildTaskInit.Priority  = INITBL_GetIntConfig(INITBL_OBJ, CFG_CHILD_PRIORITY);
      Status = CHILDMGR_Constructor(CHILDMGR_OBJ, 
                                    ChildMgr_TaskMainCallback,
                                    RADIO_IF_ChildTask, 
                                    &ChildTaskInit); 

   } /* End if INITBL Constructed */
  
   if (Status == CFE_SUCCESS)
   {

      RADIO_IF_Constructor(RADIO_IF_OBJ, &LoraTx.IniTbl);

      /*
      ** Initialize app level interfaces
      */
      
      CFE_SB_CreatePipe(&LoraTx.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_CMD_PIPE_NAME));  
      CFE_SB_Subscribe(LoraTx.CmdMid,   LoraTx.CmdPipe);
      CFE_SB_Subscribe(LoraTx.OneHzMid, LoraTx.CmdPipe);

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_NOOP_CMD_FC,   NULL, LORA_TX_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, CMDMGR_RESET_CMD_FC,  NULL, LORA_TX_ResetAppCmd, 0);

      CMDMGR_RegisterFunc(CMDMGR_OBJ, LORA_TX_INIT_RADIO_CC,     RADIO_IF_OBJ, RADIO_IF_InitRadioCmd,    0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, LORA_TX_SEND_RADIO_TLM_CC, RADIO_IF_OBJ, RADIO_IF_SendRadioTlmCmd, 0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, LORA_TX_SET_SPI_SPEED_CC,  RADIO_IF_OBJ, RADIO_IF_SetSpiSpeedCmd,  sizeof(LORA_TX_SetSpiSpeed_CmdPayload_t));

      CMDMGR_RegisterFunc(CMDMGR_OBJ, LORA_TX_SET_LO_RA_PARAMS_CC, RADIO_IF_OBJ, RADIO_IF_SetLoRaParamsCmd,  sizeof(LORA_TX_SetLoRaParams_CmdPayload_t));

      CFE_MSG_Init(CFE_MSG_PTR(LoraTx.StatusTlm.TelemetryHeader), CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_LORA_TX_STATUS_TLM_TOPICID)), sizeof(LORA_TX_StatusTlm_t));
   
      /*
      ** Application startup event message
      */
      CFE_EVS_SendEvent(LORA_TX_INIT_APP_EID, CFE_EVS_EventType_INFORMATION,
                        "LORA_TX App Initialized. Version %d.%d.%d",
                        LORA_TX_MAJOR_VER, LORA_TX_MINOR_VER, LORA_TX_PLATFORM_REV);
                        
   } /* End if CHILDMGR constructed */
   
   return(Status);

} /* End of InitApp() */


/******************************************************************************
** Function: ProcessCommands
**
*/
static int32 ProcessCommands(void)
{

   int32  RetStatus = CFE_ES_RunStatus_APP_RUN;
   int32  SysStatus;

   CFE_SB_Buffer_t* SbBufPtr;
   CFE_SB_MsgId_t   MsgId = CFE_SB_INVALID_MSG_ID;
   

   CFE_ES_PerfLogExit(LoraTx.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, LoraTx.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(LoraTx.PerfId);

   if (SysStatus == CFE_SUCCESS)
   {
      
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);
   
      if (SysStatus == CFE_SUCCESS)
      {
  
         if (CFE_SB_MsgId_Equal(MsgId, LoraTx.CmdMid)) 
         {
            
            CMDMGR_DispatchFunc(CMDMGR_OBJ, &SbBufPtr->Msg);
         
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, LoraTx.OneHzMid))
         {

            SendStatusTlm();
            
         }
         else
         {
            
            CFE_EVS_SendEvent(LORA_TX_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                              "Received invalid command packet, MID = 0x%04X",
                              CFE_SB_MsgIdToValue(MsgId));
         } 

      }
      else
      {
         
         CFE_EVS_SendEvent(LORA_TX_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                           "CFE couldn't retrieve message ID from the message, Status = %d", SysStatus);
      }
      
   } /* Valid SB receive */ 
   else 
   {
   
         CFE_ES_WriteToSysLog("LORA_TX software bus error. Status = 0x%08X\n", SysStatus);   /* Use SysLog, events may not be working */
         RetStatus = CFE_ES_RunStatus_APP_ERROR;
   }  
      
   return RetStatus;

} /* End ProcessCommands() */


/******************************************************************************
** Function: SendStatusTlm
**
*/
static void SendStatusTlm(void)
{
   
   LORA_TX_StatusTlm_Payload_t *StatusTlmPayload = &LoraTx.StatusTlm.Payload;
   
   StatusTlmPayload->ValidCmdCnt   = LoraTx.CmdMgr.ValidCmdCnt;
   StatusTlmPayload->InvalidCmdCnt = LoraTx.CmdMgr.InvalidCmdCnt;

   /*
   ** Tx Demo Object
   */ 
      
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(LoraTx.StatusTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(LoraTx.StatusTlm.TelemetryHeader), true);
   
} /* End SendStatusTlm() */
