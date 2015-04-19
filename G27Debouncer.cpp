#include <cstdlib>
#include <iostream>
#include <cassert>

#include <SDL.h>
#include <windows.h>
#include <vjoyinterface.h>

using namespace std;


bool checkForKeyIsDown(int key)
{
	// Fetch key state.
	SHORT tabKeyState = GetAsyncKeyState(key);
	// Test high bit - if set, key was down when GetAsyncKeyState was called.
	return (( 1 << 16 ) & tabKeyState);
}

int main(int argc, char* argv[])
{
	int initialized = SDL_Init(SDL_INIT_JOYSTICK);
	if (initialized)
		return EXIT_FAILURE;
	
	int joysticks = SDL_NumJoysticks();
	cout << "Joysticks found: " << joysticks << endl;
	if (joysticks < 1)
		return EXIT_FAILURE;

	SDL_Joystick* j = SDL_JoystickOpen(0);
	if (!j)
		return EXIT_FAILURE;

	int buttons = SDL_JoystickNumButtons(j);
	cout << "Joystick buttons: " << buttons << endl;

	// Switch to manual polling
	int switched = SDL_JoystickEventState(SDL_IGNORE);
	if (switched)
		return EXIT_FAILURE;

	if (!vJoyEnabled())
		return EXIT_FAILURE;

	WORD VerDll, VerDrv;
	if (!DriverMatch(&VerDll, &VerDrv))
		return EXIT_FAILURE;

	UINT id = 1;
	VjdStat status = GetVJDStatus(id);
	if (status != VJD_STAT_FREE)
		return EXIT_FAILURE;

	int virtualButtons  = GetVJDButtonNumber(id);
	cout << "Virtual joystick buttons: " << virtualButtons << endl;
	if (virtualButtons < 2)
		return EXIT_FAILURE;

	BOOL acquired = AcquireVJD(id);
	if (!acquired)
		return EXIT_FAILURE;

	// Reset the virtual joystick
	BOOL reset = ResetVJD(id);
	if (!reset)
		return EXIT_FAILURE;

	#define VIRTUAL_DURATION 300
	#define VIRTUAL_GAP 400

	bool btnRight = false, btnLeft = false;
	bool virtualRightOn = false, virtualLeftOn = false;
	Uint32 virtualRightStart = 0, virtualLeftStart = 0;

	bool enabled = true;
	while (enabled) {
		SDL_JoystickUpdate();
		Uint32 now = SDL_GetTicks();

		if (checkForKeyIsDown(VK_F6)) {
			SDL_Delay(5000);
			SetBtn(TRUE, id, 1);
			SDL_Delay(100);
			SetBtn(FALSE, id, 1);
		}

		if (checkForKeyIsDown(VK_F5)) {
			SDL_Delay(5000);
			SetBtn(TRUE, id, 2);
			SDL_Delay(100);
			SetBtn(FALSE, id, 2);
		}

		bool right = SDL_JoystickGetButton (j, 4);
		if (right != btnRight) {
			btnRight = right;
			cout << "Right button changed to " << btnRight << endl;
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
			cout << "Left button changed to " << btnLeft << endl;
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

		SDL_Delay(10);
	}

	SDL_JoystickClose(j);
	return EXIT_SUCCESS;
}
