#ifndef __MICRONIC__
#define __MICRONIC__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define MC146818_TAG	"mc146818"

typedef struct _micronic_state micronic_state;
struct _micronic_state
{
	running_device *hd61830;
};

#endif
