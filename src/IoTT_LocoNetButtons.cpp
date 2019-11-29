#include <IoTT_LocoNetButtons.h>

buttonEvent getEventTypeFromName(String eventName)
{
//	Serial.print(eventName);
//	Serial.print(" ");
	if (eventName == "onbtndown") return onbtndown;
	if (eventName == "onbtnup") return onbtnup;
	if (eventName == "onbtnclick") return onbtnclick;
	if (eventName == "onbtndblclick") return onbtndblclick;
	if (eventName == "onbtnhold") return onbtnhold;
	return onbtnup; //default

}

actionType getActionTypeByName(String actionName)
{ //blockdet, dccswitch, dccsignal, svbutton, analoginp, powerstat
  if (actionName == "block") return blockdet; 
  if (actionName == "switch") return dccswitch;
  if (actionName == "signal") return dccsignalnmra;
  if (actionName == "button") return svbutton;
  if (actionName == "analog") return analoginp;
  if (actionName == "power") return powerstat;
  return unknown; 
}

ctrlTypeType getCtrlTypeByName(String typeName)
{
  if (typeName == "closed") return closed;
  if (typeName == "thrown") return thrown;
  if (typeName == "toggle") return toggle;
  if (typeName == "nochange") return nochange;
  if (typeName == "input") return input;
  return (ctrlTypeType)typeName.toInt();
}

ctrlValueType getCtrlValueByName(String valueName)
{
  if (valueName == "on") return onVal;
  if (valueName == "off") return offVal;
  if (valueName == "idle") return idleVal;
  return (ctrlValueType)valueName.toInt();
}


/*----------------------------------------------------------------------------------------------------------------------*/

IoTT_BtnHandlerCmd::IoTT_BtnHandlerCmd()
{
//	Serial.println("Obj BtnHdlCmd");
}

IoTT_BtnHandlerCmd::~IoTT_BtnHandlerCmd()
{
}

void IoTT_BtnHandlerCmd::loadButtonCfgJSON(JsonObject thisObj)
{
	targetType = getActionTypeByName(thisObj["CtrlTarget"]);
	targetAddr = thisObj["CtrlAddr"];
	cmdType = getCtrlTypeByName(thisObj["CtrlType"]);
	cmdValue = getCtrlValueByName(thisObj["CtrlValue"]);
	if (thisObj.containsKey("ExecDelay"))
		execDelay = thisObj["ExecDelay"];
	else
		execDelay = 250;
}

void IoTT_BtnHandlerCmd::executeBtnEvent()
{
//	Serial.printf("Handling Button Command to Addr %i Type %i Value %i\n", targetAddr, cmdType, cmdValue);
    switch (targetType)
    {
      case dccswitch: if (sendSwitchCommand) sendSwitchCommand(targetAddr, cmdType, cmdValue); break; //switch
      case dccsignalnmra: if (sendSignalCommand) sendSignalCommand(targetAddr, cmdValue); break; //signal
      case powerstat: if (sendPowerCommand) sendPowerCommand(cmdType, cmdValue); break; //analog
      case blockdet: if (sendBlockDetectorCommand) sendBlockDetectorCommand(targetAddr, cmdValue); break; //analog
    }
}


/*----------------------------------------------------------------------------------------------------------------------*/

IoTT_BtnHandler::IoTT_BtnHandler()
{
}

IoTT_BtnHandler::~IoTT_BtnHandler()
{
	for (uint16_t i = 0; i < numCmds; i++)
	{
		IoTT_BtnHandlerCmd * thisPointer = &cmdList[i];
		delete thisPointer;
		thisPointer = NULL;
	}
	numCmds = 0;
	free(cmdList);
}

void IoTT_BtnHandler::loadButtonCfgJSON(JsonObject thisObj)
{
	if (thisObj.containsKey("EventType"))
	{
		eventType = getEventTypeFromName(thisObj["EventType"]);
//		Serial.println(eventType);
	}
	if (thisObj.containsKey("CmdList"))
	{
		JsonArray btnCmdList = thisObj["CmdList"];
		numCmds = btnCmdList.size();
		cmdList = (IoTT_BtnHandlerCmd*) realloc (cmdList, numCmds * sizeof(IoTT_BtnHandlerCmd));
		for (uint16_t i = 0; i < numCmds; i++)
		{
			IoTT_BtnHandlerCmd * thisCmd = new(IoTT_BtnHandlerCmd);
			thisCmd->loadButtonCfgJSON(btnCmdList[i]);
			thisCmd->parentObj = this;
			cmdList[i] = *thisCmd;
		}
	}
}

buttonEvent IoTT_BtnHandler::getEventType()
{
	return eventType;
}

void IoTT_BtnHandler::processBtnEvent()
{
    cmdBuffer * thisOutBuffer = &parentObj->parentObj->outBuffer;
    cmdPtr * lastCmd = &thisOutBuffer->cmdOutBuffer[thisOutBuffer->writePtr];
    uint8_t thisWritePtr = (thisOutBuffer->writePtr + 1) % cmdBufferLen;
	uint32_t nextExecTime = lastCmd->execTime;
	if (lastCmd->nextCommand != NULL)
		nextExecTime += lastCmd->nextCommand->execDelay;
    if (nextExecTime < millis())
		nextExecTime = millis();
	for (uint16_t i = 0; i < numCmds; i++)
	{
		if (thisWritePtr != thisOutBuffer->readPtr) //override protection
		{
			IoTT_BtnHandlerCmd * thisPointer = &cmdList[i];
			thisOutBuffer->cmdOutBuffer[thisWritePtr].nextCommand = thisPointer;
			thisOutBuffer->cmdOutBuffer[thisWritePtr].execTime = nextExecTime;
			nextExecTime = nextExecTime + thisPointer->execDelay;
			thisOutBuffer->writePtr = thisWritePtr;
			thisWritePtr = (thisWritePtr + 1) % cmdBufferLen;
		}
	}
}


/*----------------------------------------------------------------------------------------------------------------------*/

IoTT_LocoNetButtons::IoTT_LocoNetButtons()
{
}

IoTT_LocoNetButtons::~IoTT_LocoNetButtons()
{
	for (uint16_t i = 0; i < numEvents; i++)
	{
		IoTT_BtnHandler * thisPointer = &eventTypeList[i];
		delete thisPointer;
		thisPointer = NULL;
	}
	numEvents = 0;
	free(eventTypeList);
}

void IoTT_LocoNetButtons::loadButtonCfgJSON(JsonObject thisObj)
{
	if (thisObj.containsKey("ButtonNr"))
		btnAddr = thisObj["ButtonNr"];
	if (thisObj.containsKey("CtrlCmd"))
	{
		JsonArray btnCmd = thisObj["CtrlCmd"];
		numEvents = btnCmd.size();
		
		for (uint16_t i = 0; i < numEvents; i++)
		{
			JsonArray btnCmdList;
			eventTypeList = (IoTT_BtnHandler*) realloc (eventTypeList, numEvents * sizeof(IoTT_BtnHandler));
			IoTT_BtnHandler * thisEvent = new(IoTT_BtnHandler);
			thisEvent->loadButtonCfgJSON(btnCmd[i]);
			thisEvent->parentObj = this;
			eventTypeList[i] = *thisEvent;
		}
	}
}

uint16_t IoTT_LocoNetButtons::getBtnAddr()
{
	return btnAddr;
}

void IoTT_LocoNetButtons::processBtnEvent(buttonEvent inputValue)
{
	lastRecButtonEvent = inputValue;
	for (uint16_t i = 0; i < numEvents; i++)
	{
		IoTT_BtnHandler * thisEvent = &eventTypeList[i];
		if (thisEvent->getEventType() == inputValue)
		{
			lastComplButtonEvent = inputValue;
			thisEvent->processBtnEvent();
			break;
		}
	}
}


/*----------------------------------------------------------------------------------------------------------------------*/

IoTT_LocoNetButtonList::IoTT_LocoNetButtonList()
{
}

IoTT_LocoNetButtonList::~IoTT_LocoNetButtonList()
{
	freeObjects();
}

void IoTT_LocoNetButtonList::freeObjects()
{
	for (uint16_t i = 0; i < numBtnHandler; i++)
	{
		IoTT_LocoNetButtons * thisPointer = &btnList[i];
		delete thisPointer;
		thisPointer = NULL;
	}
	numBtnHandler = 0;
	free(btnList);
}

void IoTT_LocoNetButtonList::processBtnEvent(uint16_t btnAddr, buttonEvent inputValue)
{
//	Serial.println("Call Handler 1");
	for (uint16_t i = 0; i < numBtnHandler; i++)
	{
		IoTT_LocoNetButtons * thisButton = &btnList[i];
		if (thisButton->getBtnAddr() == btnAddr)
		{
			thisButton->processBtnEvent(inputValue);
			break;
		}
	}
}

void IoTT_LocoNetButtonList::loadButtonCfgJSON(DynamicJsonDocument doc)
{
	if (numBtnHandler > 0)
		freeObjects();
	if (doc.containsKey("ButtonHandler"))
    {
        JsonArray ButtonHandlers = doc["ButtonHandler"];
        numBtnHandler = ButtonHandlers.size();
        btnList = (IoTT_LocoNetButtons*) realloc (btnList, numBtnHandler * sizeof(IoTT_LocoNetButtons));
		for (uint16_t i = 0; i < numBtnHandler; i++)
		{
			IoTT_LocoNetButtons * thisButton = new(IoTT_LocoNetButtons);
			thisButton->loadButtonCfgJSON(ButtonHandlers[i]);
			thisButton->parentObj = this;
			btnList[i] = *thisButton;
		}
	}
}

void IoTT_LocoNetButtonList::processButtonHandler()
{
//	return;
	if (outBuffer.readPtr != outBuffer.writePtr)
	{
		uint8_t thisCmdPtr = (outBuffer.readPtr + 1) % cmdBufferLen;
		if (outBuffer.cmdOutBuffer[thisCmdPtr].execTime < millis())
		{
			outBuffer.cmdOutBuffer[thisCmdPtr].nextCommand->executeBtnEvent();
			outBuffer.readPtr = thisCmdPtr;
		}
	}
}

