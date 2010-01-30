/**********************************************************************

    Commodore IEC Serial Bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __CBM_IEC__
#define __CBM_IEC__

#include "emu.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define CBM_IEC DEVICE_GET_INFO_NAME( cbm_iec )

#define MDRV_CBM_IEC_ADD(_tag, _daisy_chain) \
	MDRV_DEVICE_ADD(_tag, CBM_IEC, 0) \
	MDRV_DEVICE_CONFIG(_daisy_chain)

#define CBM_IEC_DAISY(_name) \
	const cbm_iec_daisy_chain (_name)[] =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cbm_iec_daisy_chain cbm_iec_daisy_chain;
struct _cbm_iec_daisy_chain
{
	const char *tag;	/* device tag */

	devcb_write_line	out_srq_func;
	devcb_write_line	out_atn_func;
	devcb_write_line	out_clk_func;
	devcb_write_line	out_data_func;
	devcb_write_line	out_reset_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( cbm_iec );

/* service request */
void cbm_iec_srq_w(running_device *iec, running_device *device, int state);
READ_LINE_DEVICE_HANDLER( cbm_iec_srq_r );

/* attention */
void cbm_iec_atn_w(running_device *iec, running_device *device, int state);
READ_LINE_DEVICE_HANDLER( cbm_iec_atn_r );

/* clock */
void cbm_iec_clk_w(running_device *iec, running_device *device, int state);
READ_LINE_DEVICE_HANDLER( cbm_iec_clk_r );

/* data */
void cbm_iec_data_w(running_device *iec, running_device *device, int state);
READ_LINE_DEVICE_HANDLER( cbm_iec_data_r );

/* reset */
void cbm_iec_reset_w(running_device *iec, running_device *device, int state);
READ_LINE_DEVICE_HANDLER( cbm_iec_reset_r );

#endif
