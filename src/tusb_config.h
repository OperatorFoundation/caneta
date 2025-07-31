#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------+
// COMMON CONFIGURATION
//--------------------------------------------------------------------+

// defined by board.cmake
#ifndef CFG_TUSB_MCU
  #error CFG_TUSB_MCU must be defined
#endif

// RHPort number used for device can be defined by board.mk, default to port 0
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0
#endif

// RHPort max operational speed can defined by board.mk
// Default to Highspeed for MCU with internal HighSpeed PHY (can be port specific), otherwise FullSpeed
#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------
#ifndef CFG_TUD_ENABLED
#define CFG_TUD_ENABLED       0
#endif

//--------------------------------------------------------------------
// HOST CONFIGURATION
//--------------------------------------------------------------------
#ifndef CFG_TUH_ENABLED
#define CFG_TUH_ENABLED       1
#endif

// Size of buffer to hold descriptors and other data used for enumeration
#ifndef CFG_TUH_ENUMERATION_BUFSIZE
#define CFG_TUH_ENUMERATION_BUFSIZE 256
#endif

// Default is max speed that hardware controller could support with on-chip PHY
#ifndef CFG_TUH_MAX_SPEED
#define CFG_TUH_MAX_SPEED     OPT_MODE_DEFAULT_SPEED
#endif

// Maximum number of device that can be enumerated
#ifndef CFG_TUH_DEVICE_MAX
#define CFG_TUH_DEVICE_MAX    (CFG_TUH_HUB ? 4 : 1)
#endif

// Enable Host HUB Class
#ifndef CFG_TUH_HUB
#define CFG_TUH_HUB           0
#endif

/* USB DMA on some MCUs can only access a specific SRAM region with restriction on alignment.
 * Tinyusb use follows macros to declare transferring memory so that they can be put
 * into those specific section.
 * e.g
 * - CFG_TUSB_MEM SECTION : __attribute__ (( section(".usb_ram") ))
 * - CFG_TUSB_MEM_ALIGN   : __attribute__ ((aligned(4)))
 */
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN          __attribute__ ((aligned(4)))
#endif

//--------------------------------------------------------------------
// HOST CLASS DRIVER CONFIGURATION
//--------------------------------------------------------------------

// Enable Host HID Class
#ifndef CFG_TUH_HID
#define CFG_TUH_HID             2
#endif

// Enable Host CDC Class
#ifndef CFG_TUH_CDC
#define CFG_TUH_CDC             0
#endif

// Enable Host MSC Class
#ifndef CFG_TUH_MSC
#define CFG_TUH_MSC             0
#endif

// Enable Host MIDI Class
#ifndef CFG_TUH_MIDI
#define CFG_TUH_MIDI            0
#endif

// Enable Host Vendor Class
#ifndef CFG_TUH_VENDOR
#define CFG_TUH_VENDOR          0
#endif

//--------------------------------------------------------------------
// COMMON CLASS CONFIGURATION
//--------------------------------------------------------------------

// Debug level for TinyUSB
// 0: no debug
// 1: error
// 2: warning
// 3: info
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG          2
#endif

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */