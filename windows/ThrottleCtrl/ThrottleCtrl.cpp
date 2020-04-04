#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <stdio.h>
#include <strsafe.h>
#include "SimConnect.h"


static HANDLE       hSerial, hSerialReadEvent;
static DCB          dcb;
static OVERLAPPED   ovWrite, ovRead;
static COMMTIMEOUTS cto = { 0, 0, 0, 0, 0 };

void SerialBegin(char *port_name, unsigned long baud)
{
	if (hSerial) {
		dcb.BaudRate = baud;
		if (!SetCommState(hSerial, &dcb)) {}
		return;
	}

	hSerial = CreateFile(port_name,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (hSerial != INVALID_HANDLE_VALUE) {

		dcb.DCBlength = sizeof(dcb);
		dcb.BaudRate = baud;
		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;
		dcb.ByteSize = 8;

		if (!SetCommState(hSerial, &dcb)) {
			fprintf("Communication failed!");
		}

		hSerialReadEvent = CreateEvent(NULL,
			FALSE,
			FALSE,
			"RxEvent");	

		ovRead.hEvent = hSerialReadEvent;

	}
}

void SerialEnd()
{
	CloseHandle(hSerial);
	CloseHandle(hSerialReadEvent);
}




int     quit = 0;
HANDLE  hSimConnect = NULL;

enum GROUP_ID {
	GROUP_KEYS,
};
enum INPUT_ID {
	INPUT_KEYS,
};

enum EVENT_ID {
	EVENT_SIM_START,
	EVENT_1,
	EVENT_2
};

enum DATA_DEFINE_ID {
	DEFINITION_THROTTLE,
};

enum DATA_REQUEST_ID {
	REQUEST_THROTTLE,
};

struct structThrottleControl
{
	double throttlePercent;
};

structThrottleControl        tc;


void SerialRead()
{
	unsigned long bytesRead = 0;
	unsigned char c[2];

	ReadFile(hSerial,
		c,
		2,
		&bytesRead,
		&ovRead);


	if (bytesRead == 2)
	{
		int val = (c[1] << 8) | c[0];
		tc.throttlePercent = val / 10.24f;
		printf("pot: %d\n", val);
		printf("throttle: %1f\n", tc.throttlePercent);
		SimConnect_SetDataOnSimObject(hSimConnect, DEFINITION_THROTTLE, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(tc), &tc);
	}

}
void CALLBACK MyDispatchProcTC(SIMCONNECT_RECV* pData, DWORD cbData, void *pContext)
{
	HRESULT hr;

	switch (pData->dwID)
	{
	case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
	{
		SIMCONNECT_RECV_SIMOBJECT_DATA *pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;

		switch (pObjData->dwRequestID)
		{
		case REQUEST_THROTTLE:
		{
			// Read and set the initial throttle control value
			structThrottleControl *pS = (structThrottleControl*)&pObjData->dwData;

			tc.throttlePercent = pS->throttlePercent;

			printf("\nREQUEST_USERID received, throttle = %2.1f", pS->throttlePercent);

			// Now turn the input events on
			hr = SimConnect_SetInputGroupState(hSimConnect, INPUT_KEYS, SIMCONNECT_STATE_ON);
		}

		default:
			break;
		}
		break;
	}

	case SIMCONNECT_RECV_ID_EVENT:
	{
		SIMCONNECT_RECV_EVENT *evt = (SIMCONNECT_RECV_EVENT*)pData;

		switch (evt->uEventID)
		{

		case EVENT_SIM_START:
		{
			// Send this request to get the user aircraft id
			hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_THROTTLE, DEFINITION_THROTTLE, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
		}
		break;

		case EVENT_1:
		{
			// Increase the throttle
			if (tc.throttlePercent <= 95.0f)
				tc.throttlePercent += 5.0f;

			hr = SimConnect_SetDataOnSimObject(hSimConnect, DEFINITION_THROTTLE, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(tc), &tc);
		}
		break;

		case EVENT_2:
		{
			// Decrease the throttle
			if (tc.throttlePercent >= 5.0f)
				tc.throttlePercent -= 5.0f;

			hr = SimConnect_SetDataOnSimObject(hSimConnect, DEFINITION_THROTTLE, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(tc), &tc);
		}
		break;

		default:
			break;
		}
		break;
	}

	case SIMCONNECT_RECV_ID_QUIT:
	{
		quit = 1;
		break;
	}

	default:
		printf("\nReceived:%d", pData->dwID);
		break;
	}
}

void testThrottleControl()
{
	HRESULT hr;

	if (SUCCEEDED(SimConnect_Open(&hSimConnect, "Throttle Control", NULL, 0, 0, 0)))
	{
		printf("\nConnected to Prepar3D!");

		// Set up a data definition for the throttle control
		hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_THROTTLE,
			"GENERAL ENG THROTTLE LEVER POSITION:1", "percent");

		// Request a simulation started event
		hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_START, "SimStart");

		// Create two private key events to control the throttle
		hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_1);
		hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_2);

		// Link the events to some keyboard keys
		hr = SimConnect_MapInputEventToClientEvent(hSimConnect, INPUT_KEYS, "1", EVENT_1);
		hr = SimConnect_MapInputEventToClientEvent(hSimConnect, INPUT_KEYS, "2", EVENT_2);

		// Ensure the input events are off until the sim is up and running
		hr = SimConnect_SetInputGroupState(hSimConnect, INPUT_KEYS, SIMCONNECT_STATE_OFF);

		// Sign up for notifications
		hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_KEYS, EVENT_1);
		hr = SimConnect_AddClientEventToNotificationGroup(hSimConnect, GROUP_KEYS, EVENT_2);

		// Set a high priority for the group
		hr = SimConnect_SetNotificationGroupPriority(hSimConnect, GROUP_KEYS, SIMCONNECT_GROUP_PRIORITY_HIGHEST);

		while (0 == quit)
		{
			SimConnect_CallDispatch(hSimConnect, MyDispatchProcTC, NULL);
			SerialRead();
			Sleep(1);
		}

		hr = SimConnect_Close(hSimConnect);
	}
}

int __cdecl _tmain(int argc, _TCHAR* argv[])
{
	SerialBegin("COM4", CBR_9600);
	testThrottleControl();

	return 0;
}
