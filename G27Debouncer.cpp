#include <cstdlib>
#include <iostream>
#include <cassert>

#include <windows.h>

#include <SDL.h>

#include <public.h>
#include <vjoyinterface.h>

Uint32 buttonPressDuration = 300;
Uint32 buttonPressGap = 400;
Uint32 fakePressDelay = 5000;
Uint32 sdlPollingDelay = 10;
Uint32 virtualJoystickId = 1;

using namespace std;

bool checkForKeyIsDown(int key)
{
	// Fetch key state.
	SHORT tabKeyState = GetAsyncKeyState(key);
	// Test high bit - if set, key was down when GetAsyncKeyState was called.
	return (1 << 16) & tabKeyState;
}

void exitWithErrorMsg(const char* errorMsg)
{
	cout << "FATAL ERROR: " << errorMsg << endl;
	exit(EXIT_FAILURE);
}

void readSettings()
{
	// Get full path to this executable
	char path[MAX_PATH];
	if (GetModuleFileName(NULL, path, sizeof(path)) <= 0)
		exitWithErrorMsg("Could not read .exe path!");

	// Replace .exe with .ini
	size_t pathLen = strlen(path);
	strcpy_s(path + pathLen - 3, MAX_PATH - pathLen + 3, "ini");

	buttonPressDuration = GetPrivateProfileInt("timing", "button_press_duration", buttonPressDuration, path);
	cout << "SETTING: button_press_duration = " << buttonPressDuration << endl;
	buttonPressGap = GetPrivateProfileInt("timing", "button_press_gap", buttonPressGap, path);
	cout << "SETTING: button_press_gap = " << buttonPressGap << endl;
	fakePressDelay = GetPrivateProfileInt("timing", "fake_press_delay", fakePressDelay, path);
	cout << "SETTING: fake_press_delay = " << fakePressDelay << endl;
	sdlPollingDelay = GetPrivateProfileInt("timing", "sdl_polling_delay", sdlPollingDelay, path);
	cout << "SETTING: sdl_polling_delay = " << sdlPollingDelay << endl;
	virtualJoystickId = GetPrivateProfileInt("vjoy", "virtual_joystick_id", virtualJoystickId, path);
	cout << "SETTING: virtual_joystick_id = " << virtualJoystickId << endl;
}

SDL_Joystick* getG27Joystick()
{
	if (SDL_Init(SDL_INIT_JOYSTICK))
		exitWithErrorMsg("Could not initialize SDL Joystick subsystem!");

	// Switch SDL to manual polling
	int switched = SDL_JoystickEventState(SDL_IGNORE);
	if (switched)
		exitWithErrorMsg("SDL could not enable manual joystick polling!");
	
	int joystickCount = SDL_NumJoysticks();
	cout << "INFO: SDL found " << joystickCount << " joystick(s)" << endl;
	if (joystickCount < 1)
		exitWithErrorMsg("Could not find a joystick!");

	for (int i=0; i<joystickCount; ++i) {
		SDL_Joystick* joystick = SDL_JoystickOpen(i);
		if (!joystick)
			continue;
		int buttonNumber = SDL_JoystickNumButtons(joystick);
		cout << "INFO: SDL found " << buttonNumber << " button(s) on joystick number " << i << endl;
		if (buttonNumber == 23) { // The G27 reports exactly 23 buttons
			cout << "INFO: Found G27 wheel at joystick number " << i << endl;
			return joystick;
		}
		SDL_JoystickClose(joystick);
	}

	exitWithErrorMsg("Could not find the G27 joystick!");
	return 0;
}

UINT getVirtualJoystick(UINT id)
{
	cout << "INFO: Trying to load virtual joystick at ID " << id << endl;

	if (!vJoyEnabled())
		exitWithErrorMsg("The vJoy driver is not running, install and start it first!");

	WORD VerDll, VerDrv;
	if (!DriverMatch(&VerDll, &VerDrv))
		exitWithErrorMsg("Version mismatch between the vJoy driver and interface DLL!");

	VjdStat status = GetVJDStatus(id);
	if (status != VJD_STAT_FREE)
		exitWithErrorMsg("The virtual joystick device is not free!");

	int buttonNumber  = GetVJDButtonNumber(id);
	cout << "INFO: The virtual joystick has " << buttonNumber << " buttons" << endl;
	if (buttonNumber < 2)
		exitWithErrorMsg("The virtual joystick has not enough buttons!");

	BOOL acquired = AcquireVJD(id);
	if (!acquired)
		exitWithErrorMsg("The virtual joystick could not be acquired!");

	BOOL reset = ResetVJD(id);
	if (!reset)
		exitWithErrorMsg("The virtual joystick failed to reset!");

	cout << "INFO: Virtual joystick found and initialized with ID " << id << endl;
	return id;
}

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	cout << "INFO: Starting Logitech G27 Software Debouncer" << endl;
	readSettings();

	SDL_Joystick* g27 = getG27Joystick();
	UINT vJoy = getVirtualJoystick(virtualJoystickId);

	bool btnRight = false, btnLeft = false;
	bool virtualRightOn = false, virtualLeftOn = false;
	Uint32 virtualRightStart = 0, virtualLeftStart = 0;

	cout << "INFO: Starting capture loop" << endl;
	while (true) {
		SDL_JoystickUpdate();
		Uint32 now = SDL_GetTicks();

		if (checkForKeyIsDown(VK_F6)) {
			SDL_Delay(fakePressDelay);
			SetBtn(TRUE, vJoy, 1);
			SDL_Delay(buttonPressDuration);
			SetBtn(FALSE, vJoy, 1);
		}

		if (checkForKeyIsDown(VK_F5)) {
			SDL_Delay(fakePressDelay);
			SetBtn(TRUE, vJoy, 2);
			SDL_Delay(buttonPressDuration);
			SetBtn(FALSE, vJoy, 2);
		}

		bool right = SDL_JoystickGetButton(g27, 4);
		if (right != btnRight) {
			btnRight = right;
			cout << "INFO: Right hardware button changed to " << btnRight << endl;
			Uint32 sinceLastStart = now - virtualRightStart;
			if (sinceLastStart > buttonPressGap && btnRight) {
				virtualRightOn = true;
				virtualRightStart = now;
				SetBtn(TRUE, vJoy, 1);
			}
		}

		bool left = SDL_JoystickGetButton (g27, 5);
		if (left != btnLeft) {
			btnLeft = left;
			cout << "INFO: Left hardware button changed to " << btnLeft << endl;
			Uint32 sinceLastStart = now - virtualLeftStart;
			if (sinceLastStart > buttonPressGap && btnLeft) {
				virtualLeftOn = true;
				virtualLeftStart = now;
				SetBtn(TRUE, vJoy, 2);
			}
		}

		if (virtualRightOn && now - virtualRightStart > buttonPressDuration) {
			virtualRightOn = false;
			SetBtn(FALSE, vJoy, 1);
		}

		if (virtualLeftOn && now - virtualLeftStart > buttonPressDuration) {
			virtualLeftOn = false;
			SetBtn(FALSE, vJoy, 2);
		}

		SDL_Delay(sdlPollingDelay);
	}

	SDL_JoystickClose(g27);
	return EXIT_SUCCESS;
}
