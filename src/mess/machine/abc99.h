#pragma once

#ifndef __ABC99__
#define __ABC99__

#include "emu.h"



//**************************************************************************
//	MACROS / CONSTANTS
//**************************************************************************

#define ABC99_TAG	"abc99"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_ABC99_ADD() \
    MCFG_DEVICE_ADD(ABC99_TAG, ABC99, 0)



//**************************************************************************
//	TYPE DEFINITIONS
//**************************************************************************

// ======================> abc99_interface

struct abc99_interface
{
};


// ======================> abc99_device_config

class abc99_device_config :   public device_config,
                                public abc99_interface
{
    friend class abc99_device;

    // construction/destruction
    abc99_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;
	
	// optional information overrides
	virtual const rom_entry *rom_region() const;
	virtual machine_config_constructor machine_config_additions() const;

protected:
    // device_config overrides
    virtual void device_config_complete();
};


// ======================> abc99_device

class abc99_device :  public device_t
{
    friend class abc99_device_config;

    // construction/destruction
    abc99_device(running_machine &_machine, const abc99_device_config &_config);

public:
	DECLARE_WRITE8_MEMBER( z2_p1_w );
	DECLARE_READ8_MEMBER( z2_t1_r );
	DECLARE_READ8_MEMBER( z5_p1_r );
	DECLARE_WRITE8_MEMBER( z5_p2_w );
	DECLARE_WRITE8_MEMBER( z5_t0_w );
	DECLARE_READ8_MEMBER( z5_t1_r );

protected:
    // device-level overrides
    virtual void device_start();
    virtual void device_reset();

private:
    const abc99_device_config &m_config;
};


// device type definition
extern const device_type ABC99;


// keyboard matrix
INPUT_PORTS_EXTERN( abc99 );



#endif
