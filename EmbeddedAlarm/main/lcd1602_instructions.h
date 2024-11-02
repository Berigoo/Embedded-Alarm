#pragma once
namespace LCD{
  enum Intructions{
    CLEAR = 0x01,
    HOME = 0x02,
    CURSOR_DEC = 0x04,
    CURSOR_INC = 0x06,
    CURSOR_X0_Y0 = 0x80,
    CURSOR_X0_Y1 = 0xC0,
    SHIFT_DISPLAY_RIGHT = 0x05,
    SHIFT_DISPLAT_LEFT = 0x07,
    SHIFT_DISPLAY_ALL_RIGHT = 0x1C,
    SHIFT_DISPLAT_ALL__LEFT = 0x18,
    SHIFT_CURSOR_LEFT = 0x10,
    SHIFT_CURSOR_RIGHT = 0x14,
    DISPLAY_OFF_CURSOR_OFF = 0x08,
    DISPLAY_OFF_CURSOR_ON = 0x0A,
    DISPLAY_ON_CURSOR_OFF = 0x0C,
    DISPLAY_ON_CURSOR_BLINK = 0x0E,
    DISPLAY_OFF_CURSOR_BLINK = 0x0F,
  };
}