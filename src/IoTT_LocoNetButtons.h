#ifndef IoTT_LocoNetButtons_h
#define IoTT_LocoNetButtons_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <IoTT_ButtonTypeDef.h>

class IoTT_BtnHandlerCmd
{
	public:
		IoTT_BtnHandlerCmd();
		~IoTT_BtnHandlerCmd();
		void loadButtonCfgJSON(JsonObject thisObj);
		void executeBtnEvent();
		
	private:
		uint8_t targetType; //switch, input, button, signal, analog, power
		uint16_t targetAddr; //address for target type
		uint8_t cmdType; //value parameter
		uint8_t cmdValue; //value parameter
		   
};

class IoTT_BtnHandler
{
	public:
		IoTT_BtnHandler();
		~IoTT_BtnHandler();
		void loadButtonCfgJSON(JsonObject thisObj);
		buttonEvent getEventType();
		void processBtnEvent();
	private:
		buttonEvent eventType = noevent;
		uint8_t numCmds = 0;
		IoTT_BtnHandlerCmd * cmdList = NULL;
};

class IoTT_LocoNetButtons
{
	public:
		IoTT_LocoNetButtons();
		~IoTT_LocoNetButtons();
		void loadButtonCfgJSON(JsonObject thisObj);
		void processBtnEvent(buttonEvent inputValue);
		uint16_t getBtnAddr();
		
	private:
		buttonEvent lastRecButtonEvent; //last event received
		buttonEvent lastComplButtonEvent; //last event that was executed. If successful, those two are identical
		uint16_t btnAddr = 0xFFFF;
		uint8_t numEvents = 0;
		IoTT_BtnHandler * eventTypeList = NULL;
};

class IoTT_LocoNetButtonList
{
	public:
		IoTT_LocoNetButtonList();
		~IoTT_LocoNetButtonList();
		void processBtnEvent(uint16_t btnAddr, buttonEvent inputValue);
		void loadButtonCfgJSON(DynamicJsonDocument doc);
	private:
		void freeObjects();
		IoTT_LocoNetButtons * btnList = NULL;
		uint16_t numBtnHandler = 0;
};

//these are the execute functions. Provide a function with this name and parameter in your application and it will be called when a command must be sent to LocoNet
extern void sendSwitchCommand(uint16_t swiNr, uint8_t swiTargetPos, uint8_t coilStatus) __attribute__ ((weak)); //switch
extern void sendSignalCommand(uint16_t signalNr, uint8_t signalAspect) __attribute__ ((weak)); //signal
extern void sendPowerCommand(uint8_t cmdType, uint8_t pwrStatus) __attribute__ ((weak)); //power
extern void sendBlockDetectorCommand(uint16_t bdNr, uint8_t bdStatus) __attribute__ ((weak)); //block detector

#endif
