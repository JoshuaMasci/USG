#include "Client/Input/InputManager.hpp"

#include "Client/Client.hpp"
#include "Client/SDL2_Include.hpp"

#include "Client/Input/InputDevice.hpp"
#include "Client/Input/JoystickDevice.hpp"

#include "Common/Logger/Logger.hpp"

InputManager* InputManager::instance = nullptr;

InputManager* InputManager::getInstance()
{
	if (InputManager::instance == nullptr)
	{
		InputManager::instance = new InputManager();
	}
	
	return InputManager::instance;
}

InputManager::InputManager()
{
	InputManager::instance = this;
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

	this->device_map = unordered_map<void*, InputDevice*>();

	keyboardMouse = new KeyboardMouseDevice();

	keyboardMouse->addButton("Flight_Forward", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_W));
	keyboardMouse->addButton("Flight_Backward", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_S));
	keyboardMouse->addButton("Flight_Left", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_A));
	keyboardMouse->addButton("Flight_Right", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_D));
	keyboardMouse->addButton("Flight_Up", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_LSHIFT));
	keyboardMouse->addButton("Flight_Down", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_LCTRL));
	keyboardMouse->addButton("Flight_RollLeft", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_Q));
	keyboardMouse->addButton("Flight_RollRight", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_E));
	keyboardMouse->addAxis("Flight_Pitch", MouseAxis(MouseDirection::Mouse_Y, 0.05, 0.01, false));
	keyboardMouse->addAxis("Flight_Yaw", MouseAxis(MouseDirection::Mouse_X, 0.05, 0.01, false));
	//keyboardMouse->addButton("Flight_Interact", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_F));

	keyboardMouse->addButton("Flight_PitchUp", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_UP));
	keyboardMouse->addButton("Flight_PitchDown", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_DOWN));

	//keyboardMouse->addButton("Left", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_LEFT));
	//keyboardMouse->addButton("Right", KeyboardMouseButton(KEYBOARD, SDL_SCANCODE_RIGHT));
}

InputManager::~InputManager()
{
	if (this->keyboardMouse)
	{
		delete this->keyboardMouse;
	}

	for (std::pair<void*, InputDevice*> it : this->device_map)
	{
		delete it.second;
	}
	this->device_map.clear();
}

void InputManager::update(double deltaTime)
{
	this->centerMouse();
}

void InputManager::resetPreviousValues()
{
	this->keyboardMouse->resetPreviousValues();

	for (std::pair<void*, InputDevice*> it : this->device_map)
	{
		it.second->resetPreviousValues();
	}
}

bool InputManager::hasAxis(string name)
{
	/*if (keyboardMouse->hasAxis(name))
	{
		return true;
	}*/

	for (std::pair<void*, InputDevice*> it : this->device_map)
	{
		if (it.second->hasAxis(name))
		{
			return true;
		}
	}

	return false;
}

//TODO return axis for last changed instead?????
/*
	name: the axis name;
	return: the highest axis value(ignoring sign) of all devices; 
*/
double InputManager::getAxis(string name)
{
	AxisReturn axisValue;
	AxisReturn tempAxisValue;

	if (this->keyboardMouse->hasAxis(name))
	{
		tempAxisValue = this->keyboardMouse->getAxis(name);
		axisValue = tempAxisValue;
	}

	//axisValue = keyboardMouse->getAxis(name);

	for (std::pair<void*, InputDevice*> it : this->device_map)
	{
		InputDevice* device = it.second;
		if (device->hasAxis(name))
		{
			tempAxisValue = device->getAxis(name);

			if (tempAxisValue.timestamp > axisValue.timestamp)
			{
				axisValue = tempAxisValue;
			}

		}
	}

	return axisValue.value;
}

/*
	name: the button name;
	return: true if any devices have the button down;
*/
bool InputManager::getButtonDown(string name)
{
	if (this->keyboardMouse->hasButton(name))
	{
		if (this->keyboardMouse->getButtonDown(name))
		{
			return true;
		}
	}

	for (std::pair<void*, InputDevice*> it : this->device_map)
	{
		InputDevice* device = it.second;
		if (device->hasButton(name) && device->getButtonDown(name))
		{
			return true;
		}
	}

	return false;
}

/*
	name: the button name;
	return: true if any devices have the button pressed;
*/
bool InputManager::getButtonPressed(string name)
{
	if (this->keyboardMouse->hasButton(name))
	{
		if (this->keyboardMouse->getButtonPressed(name))
		{
			return true;
		}
	}

	for (std::pair<void*, InputDevice*> it : this->device_map)
	{
		InputDevice* device = it.second;
		if (device->hasButton(name) && device->getButtonPressed(name))
		{
			return true;
		}
	}

	return false;
}


/*
	Mixes the inputs of an axis with 2 buttons emulating an axis;
	axis_name: the name of the axis;
	pos_button_name: the name of the positive button;
	neg_button_name: the name of the negitive button;
	return: a double clampled between 1 and -1;
*/
double InputManager::getButtonAxisCombo(string axis_name, string pos_button_name, string neg_button_name)
{
	double axis_Value = 0.0;

	axis_Value = this->getAxis(axis_name);

	if (this->getButtonDown(pos_button_name))
	{
		axis_Value += 1.0;
	}

	if (this->getButtonDown(neg_button_name))
	{
		axis_Value -= 1.0;
	}

	//Clamp value
	if (axis_Value > 1.0)
	{
		axis_Value = 1.0;
	}
	else if (axis_Value < -1.0)
	{
		axis_Value = -1.0;
	}

	return axis_Value;
}

void InputManager::processEvent(SDL_Event event)
{
	if(event.type == SDL_JOYDEVICEADDED)
	{
		int i = event.jdevice.which;
		SDL_Joystick* joystick = SDL_JoystickOpen(i);
		this->loadJoystick(joystick);
	}
	else if(event.type == SDL_JOYDEVICEREMOVED)
	{
		int i = event.jdevice.which;
		SDL_Joystick* joystick = SDL_JoystickFromInstanceID(i);
		this->unloadJoystick(joystick);
		SDL_JoystickClose(joystick);
	}
	else if(event.type == SDL_CONTROLLERDEVICEADDED)
	{
		int i = event.cdevice.which;
		SDL_GameController* controller = SDL_GameControllerOpen(i);
		this->loadController(controller);
	}
	else if(event.type == SDL_CONTROLLERDEVICEREMOVED)
	{
		int i = event.cdevice.which;
		SDL_GameController* controller = SDL_GameControllerFromInstanceID(i);
		this->unloadController(controller);
		SDL_GameControllerClose(controller);
	}
	else if (event.type == SDL_JOYAXISMOTION)
	{
		SDL_Joystick* joystick = SDL_JoystickFromInstanceID(event.jaxis.which);
		this->device_map[joystick]->processEvent(event);
	}
	else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP)
	{
		SDL_Joystick* joystick = SDL_JoystickFromInstanceID(event.jbutton.which);
		this->device_map[joystick]->processEvent(event);
	}
	else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
	{
		this->keyboardMouse->processEvent(event);
	}
	else if (event.type == SDL_MOUSEMOTION && this->mouseLocked == true)
	{
		this->keyboardMouse->processEvent(event);
	}
}

void InputManager::loadController(SDL_GameController* controller)
{
	//InputDevice* device = new InputDevice(SDL_GameControllerName(controller), InputType::GAMEPAD, controller);

	//Load Config

	//this->devices.push_back(device);
	//this->device_map[controller] = device;
}

void InputManager::unloadController(SDL_GameController* controller)
{
	/*InputDevice* device = nullptr;

	for (InputDevice* dev : this->devices)
	{
		if (dev->m_devicePtr == controller)
		{
			device = dev;
			break;
		}
	}

	if (device != nullptr)
	{
		this->devices.remove(device);
	}*/

	if (this->device_map.find(controller) != this->device_map.end())
	{
		delete this->device_map[controller];
		this->device_map.erase(controller);
	}
}

void InputManager::loadJoystick(SDL_Joystick* joystick)
{
	JoystickDevice* device = new JoystickDevice(SDL_JoystickName(joystick), joystick);

	//Load Config
	Logger::getInstance()->logDebug("Loaded Joystick: %s\n", SDL_JoystickName(joystick));
	if (device->name == "CH PRO THROTTLE USB ")
	{
		JoystickButton axis(0);
		device->addButton("Throttle", axis);
	}
	else if (device->name == "Logitech Extreme 3D")
	{
		JoystickButton axis(0);
		device->addButton("Throttle", axis);
	}
	else if (device->name == "DUALSHOCK®4 USB Wireless Adaptor")
	{
		device->addAxis("DebugForwardBackward", JoystickAxis(1, 0.1, true));
		device->addAxis("DebugLeftRight", JoystickAxis(0, 0.1, true));

		device->addAxis("DebugPitch", JoystickAxis(5, 0.1, false));
		device->addAxis("DebugYaw", JoystickAxis(2, 0.1, true));

		device->addButton("DebugRollRight", JoystickButton(5));
		device->addButton("DebugRollLeft", JoystickButton(4));

		device->addButton("DebugInteract", JoystickButton(0));
	}

	this->device_map[joystick] = device;
}

void InputManager::unloadJoystick(SDL_Joystick* joystick)
{
	if (this->device_map.find(joystick) != this->device_map.end())
	{
		delete this->device_map[joystick];
		this->device_map.erase(joystick);
	}
}

void InputManager::centerMouse()
{
	if (this->mouseLocked == true)
	{
		int width, height;
		if (Client::instance != nullptr)
		{
			Client::instance->window->getWindowSize(width, height);
			Client::instance->window->setMousePos(width / 2, height / 2);
		}
	}
}

void InputManager::setMouseLock(bool locked)
{
	this->mouseLocked = locked;

	if (this->mouseLocked == true)
	{
		this->centerMouse();
		SDL_ShowCursor(SDL_DISABLE);
	}
	else
	{
		SDL_ShowCursor(SDL_ENABLE);
	}
}

bool InputManager::getMouseLock()
{
	return this->mouseLocked;
}