#include "Input/Private/Android/KeyboardImpl.Android.h"

#include "Base/TemplateHelpers.h"
#include "Utils/UTF8Utils.h"

namespace DAVA
{
namespace Private
{
// Taken from https://developer.android.com/reference/android/view/KeyEvent.html
// or https://android.googlesource.com/platform/frameworks/native/+/master/include/android/keycodes.h
const eInputElements nativeVirtualToDavaScancode[] =
{
  eInputElements::NONE, // 0
  eInputElements::NONE, // 1
  eInputElements::NONE, // 2
  eInputElements::NONE, // 3
  eInputElements::NONE, // 4
  eInputElements::NONE, // 5
  eInputElements::NONE, // 6
  eInputElements::KB_0, // 7
  eInputElements::KB_1, // 8
  eInputElements::KB_2, // 9
  eInputElements::KB_3, // 10
  eInputElements::KB_4, // 11
  eInputElements::KB_5, // 12
  eInputElements::KB_6, // 13
  eInputElements::KB_7, // 14
  eInputElements::KB_8, // 15
  eInputElements::KB_9, // 16
  eInputElements::NONE, // 17
  eInputElements::NONE, // 18
  eInputElements::KB_UP, // 19
  eInputElements::KB_DOWN, // 20
  eInputElements::KB_LEFT, // 21
  eInputElements::KB_RIGHT, // 22
  eInputElements::NONE, // 23
  eInputElements::KB_VOLUME_UP, // 24
  eInputElements::KB_VOLUME_DOWN, // 25
  eInputElements::NONE, // 26
  eInputElements::NONE, // 27
  eInputElements::NONE, // 28
  eInputElements::KB_A, // 29
  eInputElements::KB_B, // 30
  eInputElements::KB_C, // 31
  eInputElements::KB_D, // 32
  eInputElements::KB_E, // 33
  eInputElements::KB_F, // 34
  eInputElements::KB_G, // 35
  eInputElements::KB_H, // 36
  eInputElements::KB_I, // 37
  eInputElements::KB_J, // 38
  eInputElements::KB_K, // 39
  eInputElements::KB_L, // 40
  eInputElements::KB_M, // 41
  eInputElements::KB_N, // 42
  eInputElements::KB_O, // 43
  eInputElements::KB_P, // 44
  eInputElements::KB_Q, // 45
  eInputElements::KB_R, // 46
  eInputElements::KB_S, // 47
  eInputElements::KB_T, // 48
  eInputElements::KB_U, // 49
  eInputElements::KB_V, // 50
  eInputElements::KB_W, // 51
  eInputElements::KB_X, // 52
  eInputElements::KB_Y, // 53
  eInputElements::KB_Z, // 54
  eInputElements::KB_COMMA, // 55
  eInputElements::KB_PERIOD, // 56
  eInputElements::KB_LALT, // 57
  eInputElements::KB_RALT, // 58
  eInputElements::KB_LSHIFT, // 59
  eInputElements::KB_RSHIFT, // 60
  eInputElements::KB_TAB, // 61
  eInputElements::KB_SPACE, // 62
  eInputElements::NONE, // 63
  eInputElements::NONE, // 64
  eInputElements::NONE, // 65
  eInputElements::KB_ENTER, // 66
  eInputElements::KB_BACKSPACE, // 67
  eInputElements::KB_GRAVE, // 68
  eInputElements::KB_MINUS, // 69
  eInputElements::KB_EQUALS, // 70
  eInputElements::KB_LBRACKET, // 71
  eInputElements::KB_RBRACKET, // 72
  eInputElements::KB_BACKSLASH, // 73
  eInputElements::KB_SEMICOLON, // 74
  eInputElements::KB_APOSTROPHE, // 75
  eInputElements::KB_SLASH, // 76
  eInputElements::NONE, // 77
  eInputElements::NONE, // 78
  eInputElements::NONE, // 79
  eInputElements::KB_CAMERA_FOCUS, // 80
  eInputElements::NONE, // 81
  eInputElements::KB_MENU, // 82
  eInputElements::NONE, // 83
  eInputElements::NONE, // 84
  eInputElements::KB_MEDIA_PLAY_PAUSE, // 85
  eInputElements::NONE, // 86
  eInputElements::KB_MEDIA_NEXT, // 87
  eInputElements::KB_MEDIA_PREVIOUS, // 88
  eInputElements::NONE, // 89
  eInputElements::NONE, // 90
  eInputElements::NONE, // 91
  eInputElements::KB_PAGEUP, // 92
  eInputElements::KB_PAGEDOWN, // 93
  eInputElements::NONE, // 94
  eInputElements::NONE, // 95
  eInputElements::NONE, // 96
  eInputElements::NONE, // 97
  eInputElements::NONE, // 98
  eInputElements::NONE, // 99
  eInputElements::NONE, // 100
  eInputElements::NONE, // 101
  eInputElements::NONE, // 102
  eInputElements::NONE, // 103
  eInputElements::NONE, // 104
  eInputElements::NONE, // 105
  eInputElements::NONE, // 106
  eInputElements::NONE, // 107
  eInputElements::NONE, // 108
  eInputElements::NONE, // 109
  eInputElements::NONE, // 110
  eInputElements::KB_ESCAPE, // 111
  eInputElements::KB_DELETE, // 112
  eInputElements::KB_LCTRL, // 113
  eInputElements::KB_RCTRL, // 114
  eInputElements::KB_CAPSLOCK, // 115
  eInputElements::KB_SCROLLLOCK, // 116
  eInputElements::KB_LCMD, // 117
  eInputElements::KB_RCMD, // 118
  eInputElements::KB_FUNCTION, // 119
  eInputElements::KB_PRINTSCREEN, // 120
  eInputElements::KB_PAUSE, // 121
  eInputElements::KB_HOME, // 122
  eInputElements::KB_END, // 123
  eInputElements::KB_INSERT, // 124
  eInputElements::NONE, // 125
  eInputElements::NONE, // 126
  eInputElements::NONE, // 127
  eInputElements::NONE, // 128
  eInputElements::KB_MEDIA_EJECT, // 129
  eInputElements::NONE, // 130
  eInputElements::KB_F1, // 131
  eInputElements::KB_F2, // 132
  eInputElements::KB_F3, // 133
  eInputElements::KB_F4, // 134
  eInputElements::KB_F5, // 135
  eInputElements::KB_F6, // 136
  eInputElements::KB_F7, // 137
  eInputElements::KB_F8, // 138
  eInputElements::KB_F9, // 139
  eInputElements::KB_F10, // 140
  eInputElements::KB_F11, // 141
  eInputElements::KB_F12, // 142
  eInputElements::KB_NUMLOCK, // 143
  eInputElements::KB_NUMPAD_0, // 144
  eInputElements::KB_NUMPAD_1, // 145
  eInputElements::KB_NUMPAD_2, // 146
  eInputElements::KB_NUMPAD_3, // 147
  eInputElements::KB_NUMPAD_4, // 148
  eInputElements::KB_NUMPAD_5, // 149
  eInputElements::KB_NUMPAD_6, // 150
  eInputElements::KB_NUMPAD_7, // 151
  eInputElements::KB_NUMPAD_8, // 152
  eInputElements::KB_NUMPAD_9, // 153
  eInputElements::KB_DIVIDE, // 154
  eInputElements::KB_MULTIPLY, // 155
  eInputElements::KB_NUMPAD_MINUS, // 156
  eInputElements::KB_NUMPAD_PLUS, // 157
  eInputElements::KB_NUMPAD_DELETE, // 158
  eInputElements::NONE, // 159
  eInputElements::KB_NUMPAD_ENTER, // 160
  eInputElements::KB_EQUALS, // 161
  eInputElements::NONE, // 162
  eInputElements::NONE, // 163
  eInputElements::KB_VOLUME_MUTE, // 164
  /* There are at least 283 keycodes in android, the ones below are unused right now
    eInputElements::NONE, // 165
    eInputElements::NONE, // 166
    eInputElements::NONE, // 167
    eInputElements::NONE, // 168
    eInputElements::NONE, // 169
    eInputElements::NONE, // 170
    eInputElements::NONE, // 171
    eInputElements::NONE, // 172
    eInputElements::NONE, // 173
    eInputElements::NONE, // 174
    eInputElements::NONE, // 175
    eInputElements::NONE, // 176
    eInputElements::NONE, // 177
    eInputElements::NONE, // 178
    eInputElements::NONE, // 179
    eInputElements::NONE, // 180
    eInputElements::NONE, // 181
    eInputElements::NONE, // 182
    eInputElements::NONE, // 183
    eInputElements::NONE, // 184
    eInputElements::NONE, // 185
    eInputElements::NONE, // 186
    eInputElements::NONE, // 187
    eInputElements::NONE, // 188
    eInputElements::NONE, // 189
    eInputElements::NONE, // 190
    eInputElements::NONE, // 191
    eInputElements::NONE, // 192
    eInputElements::NONE, // 193
    eInputElements::NONE, // 194
    eInputElements::NONE, // 195
    eInputElements::NONE, // 196
    eInputElements::NONE, // 197
    eInputElements::NONE, // 198
    eInputElements::NONE, // 199
    eInputElements::NONE, // 200
    eInputElements::NONE, // 201
    eInputElements::NONE, // 202
    eInputElements::NONE, // 203
    eInputElements::NONE, // 204
    eInputElements::NONE, // 205
    eInputElements::NONE, // 206
    eInputElements::NONE, // 207
    eInputElements::NONE, // 208
    eInputElements::NONE, // 209
    eInputElements::NONE, // 210
    eInputElements::NONE, // 211
    eInputElements::NONE, // 212
    eInputElements::NONE, // 213
    eInputElements::NONE, // 214
    eInputElements::NONE, // 215
    eInputElements::NONE, // 216
    eInputElements::NONE, // 217
    eInputElements::NONE, // 218
    eInputElements::NONE, // 219
    eInputElements::NONE, // 220
    eInputElements::NONE, // 221
    eInputElements::NONE, // 222
    eInputElements::NONE, // 223
    eInputElements::NONE, // 224
    eInputElements::NONE, // 225
    eInputElements::NONE, // 226
    eInputElements::NONE, // 227
    eInputElements::NONE, // 228
    eInputElements::NONE, // 229
    eInputElements::NONE, // 230
    eInputElements::NONE, // 231
    eInputElements::NONE, // 232
    eInputElements::NONE, // 233
    eInputElements::NONE, // 234
    eInputElements::NONE, // 235
    eInputElements::NONE, // 236
    eInputElements::NONE, // 237
    eInputElements::NONE, // 238
    eInputElements::NONE, // 239
    eInputElements::NONE, // 240
    eInputElements::NONE, // 241
    eInputElements::NONE, // 242
    eInputElements::NONE, // 243
    eInputElements::NONE, // 244
    eInputElements::NONE, // 245
    eInputElements::NONE, // 246
    eInputElements::NONE, // 247
    eInputElements::NONE, // 248
    eInputElements::NONE, // 249
    eInputElements::NONE, // 250
    eInputElements::NONE, // 251
    eInputElements::NONE, // 252
    eInputElements::NONE, // 253
    eInputElements::NONE, // 254
    eInputElements::NONE, // 255
    eInputElements::NONE, // 256
    eInputElements::NONE, // 257
    eInputElements::NONE, // 258
    eInputElements::NONE, // 259
    eInputElements::NONE, // 260
    eInputElements::NONE, // 261
    eInputElements::NONE, // 262
    eInputElements::NONE, // 263
    eInputElements::NONE, // 264
    eInputElements::NONE, // 265
    eInputElements::NONE, // 266
    eInputElements::NONE, // 267
    eInputElements::NONE, // 268
    eInputElements::NONE, // 269
    eInputElements::NONE, // 270
    eInputElements::NONE, // 271
    eInputElements::NONE, // 272
    eInputElements::NONE, // 273
    eInputElements::NONE, // 274
    eInputElements::NONE, // 275
    eInputElements::NONE, // 276
    eInputElements::NONE, // 277
    eInputElements::NONE, // 278
    eInputElements::NONE, // 279
    eInputElements::NONE, // 280
    eInputElements::NONE, // 281
    eInputElements::NONE, // 282
    eInputElements::NONE, // 283
    */
};

eInputElements KeyboardImpl::ConvertNativeScancodeToDavaScancode(uint32 /*nativeScancode*/, uint32 nativeVirtual)
{
    // Use virtual key instead of scancode for now

    if (nativeVirtual >= COUNT_OF(nativeVirtualToDavaScancode))
    {
        return eInputElements::NONE;
    }

    return nativeVirtualToDavaScancode[nativeVirtual];
}

uint32 KeyboardImpl::ConvertDavaScancodeToNativeScancode(eInputElements elementId)
{
    int nativeScancode = -1;

    for (size_t i = 0; i < COUNT_OF(nativeVirtualToDavaScancode); ++i)
    {
        if (nativeVirtualToDavaScancode[i] == elementId)
        {
            nativeScancode = static_cast<int>(i);
        }
    }

    DVASSERT(nativeScancode >= 0);

    return static_cast<uint32>(nativeScancode);
}

String KeyboardImpl::TranslateElementToUTF8String(eInputElements elementId)
{
    return GetInputElementInfo(elementId).name;
}
} // namespace Private
} // namespace DAVA
