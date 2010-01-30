#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0501
#include <windows.h>

#include "emu.h"
#include "image.h"
#include "msuiutil.h"

BOOL DriverIsComputer(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_COMPUTER) != 0;
}

BOOL DriverIsModified(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_COMPUTER_MODIFIED) != 0;
}

BOOL DriverHasDevice(const game_driver *gamedrv, iodevice_t type)
{
	BOOL b = FALSE;
	machine_config *config;
	const device_config *device;

	// allocate the machine config
	config = machine_config_alloc(gamedrv->machine_config);

	for (device = config->devicelist.first(); device != NULL;device = device->next)
	{
		if (is_image_device(device))
		{			
			if (image_device_getinfo(config, device).type == type)
			{
				b = TRUE;
				break;
			}
		}
	}

	machine_config_free(config);
	return b;
}


