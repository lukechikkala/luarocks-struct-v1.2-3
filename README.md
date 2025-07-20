# luarocks struct v1.2-3
Adapted the original luarocks rock, [struct](https://luarocks.org/modules/luarocks/struct) to work with Lua `v5.4.8`

## How to use
Place the `struct-1.2-3.rockspec` in:
```
%APPDATA%\luarocks\lib\luarocks\rocks-5.4\struct\1.2-3
```

If that does not work, recompile it using:
```
luarocks make struct-1.2-3.rockspec
```

## Changes
### Replaced `luaL_putchar` to `luaL_addchar`.
![alt](/rsc/0.png)

### Replaced `struct luaL_reg` to `luaL_Reg`.
![alt](/rsc/1.png)

### Replaced `luaL_register` to `luaL_newlib` and added `lua_setglobal`
![alt](/rsc/2.png)

## Credits
Original Author: [Roberto Ierusalimschy](https://www.inf.puc-rio.br/~roberto/)