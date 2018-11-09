#ifndef WINREGION_H_INCLUDED
#define WINREGION_H_INCLUDED

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

void lua_register_winreg(lua_State* L);

#endif // WINREGION_H_INCLUDED
