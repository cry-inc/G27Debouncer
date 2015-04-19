#include <cstdlib>
#include <iostream>
#include <cassert>

#include <windows.h>
#include <SDL.h>
#include <vjoyinterface.h>

#define VIRTUAL_DURATION	300
#define VIRTUAL_GAP			400
#define VIRTUAL_DELAY		5000
#define SDL_POLLING_TIME	10

using namespace std;

bool checkForKeyIsDown(int key)
{
	// Fetch key state.
	SHORT tabKeyState = GetAsyncKeyState(key);
	// Test high bit - if set, key was down when GetAsyncKeyState was called.
	return (( 1 << 16 ) & tabKeyState);
}

void exitWithErrorMsg(const char* errorMsg)
{
	cout << "FATAL ERROR: " << errorMsg << endl;
	exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
	if (SDL_Init(SDL_INIT_JOYSTICK))
		exitWithErrorMsg("Could not initialize SDL Joystick subsystem!");
	
	int joysticks = SDL_NumJoysticks();
	cout << "INFO: SDL found " << joysticks << " joysticks!" << endl;
	if (joysticks < 1)
		exitWithErrorMsg("Could not find a joystick!");

	SDL_Joystick* j = SDL_JoystickOpen(0);
	if (!j)
		exitWithErrorMsg("SDL could not the input joystick!");

	int buttons = SDL_JoystickNumButtons(j);
	cout << "INFO: SDL found " << buttons << " buttons on the joystick!" << endl;

	// Switch SDL to manual polling
	int switched = SDL_JoystickEventState(SDL_IGNORE);
	if (switched)
		exitWithErrorMsg("SDL could not enable manual joystick polling!");

	if (!vJoyEnabled())
		exitWithErrorMsg("The vJoy driver is not running, install and start it first!");

	WORD VerDll, VerDrv;
	if (!DriverMatch(&VerDll, &VerDrv))
		exitWithErrorMsg("Version mismatch between the vJoy driver and interface DLL!");

	UINT id = 1;
	VjdStat status = GetVJDStatus(id);
	if (status != VJD_STAT_FREE)
		exitWithErrorMsg("The virtual joystick device is not free!");

	int virtualButtons  = GetVJDButtonNumber(id);
	cout << "INFO: The virtual joystick has " << virtualButtons << " buttons!" << endl;
	if (virtualButtons < 2)
		exitWithErrorMsg("The virtual joystick has not enough buttons!");

	BOOL acquired = AcquireVJD(id);
	if (!acquired)
		exitWithErrorMsg("The virtual joystick could not be acquired!");

	BOOL reset = ResetVJD(id);
	if (!reset)
		exitWithErrorMsg("The virtual joystick failed to reset!");

	bool btnRight = false, btnLeft = false;
	bool virtualRightOn = false, virtualLeftOn = false;
	Uint32 virtualRightStart = 0, virtualLeftStart = 0;

	while (true) {
		SDL_JoystickUpdate();
		Uint32 now = SDL_GetTicks();

		if (checkForKeyIsDown(VK_F6)) {
			SDL_Delay(VIRTUAL_DELAY);
			SetBtn(TRUE, id, 1);
			SDL_Delay(VIRTUAL_DURATION);
			SetBtn(FALSE, id, 1);
		}

		if (checkForKeyIsDown(VK_F5)) {
			SDL_Delay(VIRTUAL_DELAY);
			SetBtn(TRUE, id, 2);
			SDL_Delay(VIRTUAL_DURATION);
			SetBtn(FALSE, id, 2);
		}

		bool right = SDL_JoystickGetButton (j, 4);
		if (right != btnRight) {
			btnRight = right;
			cout << "INFO: Right hardware button changed to " << btnRight << endl;
			Uint32 sinceLastStart = now - virtualRightStart;
			if (sinceLastStart > VIRTUAL_GAP && btnRight) {
				virtualRightOn = true;
				virtualRightStart = now;
				SetBtn(TRUE, id, 1);
			}
		}

		bool left = SDL_JoystickGetButton (j, 5) || checkForKeyIsDown(VK_F5);
		if (left != btnLeft) {
			btnLeft = left;
			cout << "INFO: Left hardware button changed to " << btnLeft << endl;
			Uint32 sinceLastStart = now - virtualLeftStart;
			if (sinceLastStart > VIRTUAL_GAP && btnLeft) {
				virtualLeftOn = true;
				virtualLeftStart = now;
				SetBtn(TRUE, id, 2);
			}
		}

		if (virtualRightOn && now - virtualRightStart > VIRTUAL_DURATION) {
			virtualRightOn = false;
			SetBtn(FALSE, id, 1);
		}

		if (virtualLeftOn && now - virtualLeftStart > VIRTUAL_DURATION) {
			virtualLeftOn = false;
			SetBtn(FALSE, id, 2);
		}

		SDL_Delay(SDL_POLLING_TIME);
	}

	SDL_JoystickClose(j);
	return EXIT_SUCCESS;
}
