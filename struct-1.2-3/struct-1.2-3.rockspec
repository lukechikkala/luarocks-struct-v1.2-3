----------------------------------------------------------------------------------------------------
package = "struct"
version = "1.2-3"
source  = {
    url = "file://.",  -- Use local directory
}
----------------------------------------------------------------------------------------------------
description = {
     summary    = "Library for packing/unpacking binary data (Lua 5.4 compatible)"
    ,detailed   = [[
        The original struct ( https://luarocks.org/modules/luarocks/struct ) was built using 5.1.
        This rock is modified to make it work with Lua v5.4.8.
        It might with versions between v5.2 and v5.4 too, but I haven't tested this.
    ]]
    ,homepage   = "http://www.inf.puc-rio.br/~roberto/struct/"
    ,license    = "MIT"
}
----------------------------------------------------------------------------------------------------
dependencies = { "lua >= 5.1" }
----------------------------------------------------------------------------------------------------
build = {
    type    = "builtin",
    modules = {
        struct = "struct.c"
    }
}
----------------------------------------------------------------------------------------------------
