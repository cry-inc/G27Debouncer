#include <cstdlib>
#include <iostream>
#include <cassert>

#include <windows.h>
#include <SDL.h>
#include <vjoyinterface.h>

#define VIRTUAL_DURATION	300
#define VIRTUAL_GAP			400
#define VIRTUAL_DELAY		5000
#define VIRTUAL_JOYSTICK_ID	1u
#define SDL_POLLING_TIME	10

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

SDL_Joystick* getG27Joystick()
{
	if (SDL_Init(SDL_INIT_JOYSTICK))
		exitWithErrorMsg("Could not initialize SDL Joystick subsystem!");

	// Switch SDL to manual polling
	int switched = SDL_JoystickEventState(SDL_IGNORE);
	if (switched)
		exitWithErrorMsg("SDL could not enable manual joystick polling!");
	
	int joystickNumber = SDL_NumJoysticks();
	cout << "INFO: SDL found " << joystickNumber << " joystick(s)" << endl;
	if (joystickNumber < 1)
		exitWithErrorMsg("Could not find a joystick!");

	for (int i=0; i<joystickNumber; ++i) {
		SDL_Joystick* joystick = SDL_JoystickOpen(0);
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
	SDL_Joystick* g27 = getG27Joystick();
	UINT vJoy = getVirtualJoystick(VIRTUAL_JOYSTICK_ID);

	bool btnRight = false, btnLeft = false;
	bool virtualRightOn = false, virtualLeftOn = false;
	Uint32 virtualRightStart = 0, virtualLeftStart = 0;

	cout << "INFO: Starting capture loop" << endl;
	while (true) {
		SDL_JoystickUpdate();
		Uint32 now = SDL_GetTicks();

		if (checkForKeyIsDown(VK_F6)) {
			SDL_Delay(VIRTUAL_DELAY);
			SetBtn(TRUE, vJoy, 1);
			SDL_Delay(VIRTUAL_DURATION);
			SetBtn(FALSE, vJoy, 1);
		}

		if (checkForKeyIsDown(VK_F5)) {
			SDL_Delay(VIRTUAL_DELAY);
			SetBtn(TRUE, vJoy, 2);
			SDL_Delay(VIRTUAL_DURATION);
			SetBtn(FALSE, vJoy, 2);
		}

		bool right = SDL_JoystickGetButton (g27, 4);
		if (right != btnRight) {
			btnRight = right;
			cout << "INFO: Right hardware button changed to " << btnRight << endl;
			Uint32 sinceLastStart = now - virtualRightStart;
			if (sinceLastStart > VIRTUAL_GAP && btnRight) {
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
			if (sinceLastStart > VIRTUAL_GAP && btnLeft) {
				virtualLeftOn = true;
				virtualLeftStart = now;
				SetBtn(TRUE, vJoy, 2);
			}
		}

		if (virtualRightOn && now - virtualRightStart > VIRTUAL_DURATION) {
			virtualRightOn = false;
			SetBtn(FALSE, vJoy, 1);
		}

		if (virtualLeftOn && now - virtualLeftStart > VIRTUAL_DURATION) {
			virtualLeftOn = false;
			SetBtn(FALSE, vJoy, 2);
		}

		SDL_Delay(SDL_POLLING_TIME);
	}

	SDL_JoystickClose(g27);
	return EXIT_SUCCESS;
}
