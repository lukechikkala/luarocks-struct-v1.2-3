//==================================================================================================
//  Original Author : Roberto Ierusalimschy [ https://www.inf.puc-rio.br/~roberto/ ]
//  Modified by     : Luke Chikkala         [ https://github.com/lukechikkala ]
//==================================================================================================
//  Adapted it to work with Lua v5.4.8
//==================================================================================================
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
//==================================================================================================
//  Library for packing/unpacking structures.
//  $Id: struct.c,v 1.2 2008/04/18 20:06:01 roberto Exp $
//==================================================================================================
//  Valid formats:
//      >      : Big endian
//      <      : Little endian
//      ![num] : Alignment
//      x      : Pading
//      b/B    : Signed/unsigned byte
//      h/H    : Signed/unsigned short
//      l/L    : Signed/unsigned long
//      i/In   : Signed/unsigned integer with size `n' (default is size of int)
//      cn     : Sequence of `n' chars (from/to a string); when packing, n==0 means the whole string;
//               when unpacking, n==0 means use the previous read number as the string length
//      s      : Zero-terminated string
//      f      : Float
//      d      : Doulbe
//      ' '    : Ignored
//==================================================================================================

/* is 'x' a power of 2? */
#define isp2(x) ((x) > 0 && ((x) & ((x) - 1)) == 0)

/* dummy structure to get alignment requirements */
struct cD
{
    char c;
    double d;
};

#define PADDING (sizeof(struct cD) - sizeof(double))
#define MAXALIGN (PADDING > sizeof(int) ? PADDING : sizeof(int))

/* endian options */
#define BIG 0
#define LITTLE 1

static union
{
    int dummy;
    char endian;
} const native = {1};

typedef struct Header
{
    int endian;
    int align;
} Header;

static size_t getnum(const char **fmt, size_t df)
{
    if (!isdigit(**fmt)) /* no number? */
        return df;       /* return default value */
    else
    {
        size_t a = 0;
        do
        {
            a = a * 10 + *((*fmt)++) - '0';
        } while (isdigit(**fmt));
        return a;
    }
}

#define defaultoptions(h) ((h)->endian = native.endian, (h)->align = 1)

static size_t optsize(lua_State *L, char opt, const char **fmt)
{
    switch (opt)
    {
    case 'B':
    case 'b':
        return sizeof(char);
    case 'H':
    case 'h':
        return sizeof(short);
    case 'L':
    case 'l':
        return sizeof(long);
    case 'f':
        return sizeof(float);
    case 'd':
        return sizeof(double);
    case 'x':
        return 1;
    case 'c':
        return getnum(fmt, 1);
    case 's':
    case ' ':
    case '<':
    case '>':
    case '!':
        return 0;
    case 'i':
    case 'I':
    {
        int sz = getnum(fmt, sizeof(int));
        if (!isp2(sz))
            luaL_error(L, "integral size %d is not a power of 2", sz);
        return sz;
    }
    default:
    {
        const char *msg = lua_pushfstring(L, "invalid format option [%c]", opt);
        return luaL_argerror(L, 1, msg);
    }
    }
}

static int gettoalign(size_t len, Header *h, int opt, size_t size)
{
    if (size == 0 || opt == 'c')
        return 0;
    if (size > (size_t)h->align)
        size = h->align; /* respect max. alignment */
    return (size - (len & (size - 1))) & (size - 1);
}

static void commoncases(lua_State *L, int opt, const char **fmt, Header *h)
{
    switch (opt)
    {
    case ' ':
        return; /* ignore white spaces */
    case '>':
        h->endian = BIG;
        return;
    case '<':
        h->endian = LITTLE;
        return;
    case '!':
    {
        int a = getnum(fmt, MAXALIGN);
        if (!isp2(a))
            luaL_error(L, "alignment %d is not a power of 2", a);
        h->align = a;
        return;
    }
    default:
        assert(0);
    }
}

static void putinteger(lua_State *L, luaL_Buffer *b, int arg, int endian,
                       int size)
{
    lua_Number n = luaL_checknumber(L, arg);
    unsigned long value;
    if (n < (lua_Number)LONG_MAX)
        value = (long)n;
    else
        value = (unsigned long)n;
    if (endian == LITTLE)
    {
        int i;
        for (i = 0; i < size; i++)
            luaL_addchar(b, (value >> 8 * i) & 0xff);
    }
    else
    {
        int i;
        for (i = size - 1; i >= 0; i--)
            luaL_addchar(b, (value >> 8 * i) & 0xff);
    }
}

static void correctbytes(char *b, int size, int endian)
{
    if (endian != native.endian)
    {
        int i = 0;
        while (i < --size)
        {
            char temp = b[i];
            b[i++] = b[size];
            b[size] = temp;
        }
    }
}

static int b_pack(lua_State *L)
{
    luaL_Buffer b;
    const char *fmt = luaL_checkstring(L, 1);
    Header h;
    int arg = 2;
    size_t totalsize = 0;
    defaultoptions(&h);
    lua_pushnil(L); /* mark to separate arguments from string buffer */
    luaL_buffinit(L, &b);
    while (*fmt != '\0')
    {
        int opt = *fmt++;
        size_t size = optsize(L, opt, &fmt);
        int toalign = gettoalign(totalsize, &h, opt, size);
        totalsize += toalign;
        while (toalign-- > 0)
            luaL_addchar(&b, '\0');
        switch (opt)
        {
        case 'b':
        case 'B':
        case 'h':
        case 'H':
        case 'l':
        case 'L':
        case 'i':
        case 'I':
        { /* integer types */
            putinteger(L, &b, arg++, h.endian, size);
            break;
        }
        case 'x':
        {
            luaL_addchar(&b, '\0');
            break;
        }
        case 'f':
        {
            float f = (float)luaL_checknumber(L, arg++);
            correctbytes((char *)&f, size, h.endian);
            luaL_addlstring(&b, (char *)&f, size);
            break;
        }
        case 'd':
        {
            double d = luaL_checknumber(L, arg++);
            correctbytes((char *)&d, size, h.endian);
            luaL_addlstring(&b, (char *)&d, size);
            break;
        }
        case 'c':
        case 's':
        {
            size_t l;
            const char *s = luaL_checklstring(L, arg++, &l);
            if (size == 0)
                size = l;
            luaL_argcheck(L, l >= (size_t)size, arg, "string too short");
            luaL_addlstring(&b, s, size);
            if (opt == 's')
            {
                luaL_addchar(&b, '\0'); /* add zero at the end */
                size++;
            }
            break;
        }
        default:
            commoncases(L, opt, &fmt, &h);
        }
        totalsize += size;
    }
    luaL_pushresult(&b);
    return 1;
}

static lua_Number getinteger(const char *buff, int endian,
                             int issigned, int size)
{
    unsigned long l = 0;
    if (endian == BIG)
    {
        int i;
        for (i = 0; i < size; i++)
            l |= (unsigned long)(unsigned char)buff[size - i - 1] << (i * 8);
    }
    else
    {
        int i;
        for (i = 0; i < size; i++)
            l |= (unsigned long)(unsigned char)buff[i] << (i * 8);
    }
    if (!issigned)
        return (lua_Number)l;
    else
    { /* signed format */
        unsigned long mask = ~(0UL) << (size * 8 - 1);
        if (l & mask)  /* negative value? */
            l |= mask; /* signal extension */
        return (lua_Number)(long)l;
    }
}

static int b_unpack(lua_State *L)
{
    Header h;
    const char *fmt = luaL_checkstring(L, 1);
    size_t ld;
    const char *data = luaL_checklstring(L, 2, &ld);
    size_t pos = luaL_optinteger(L, 3, 1) - 1;
    defaultoptions(&h);
    lua_settop(L, 2);
    while (*fmt)
    {
        int opt = *fmt++;
        size_t size = optsize(L, opt, &fmt);
        pos += gettoalign(pos, &h, opt, size);
        luaL_argcheck(L, pos + size <= ld, 2, "data string too short");
        switch (opt)
        {
        case 'b':
        case 'B':
        case 'h':
        case 'H':
        case 'l':
        case 'L':
        case 'i':
        case 'I':
        { /* integer types */
            int issigned = islower(opt);
            lua_Number res = getinteger(data + pos, h.endian, issigned, size);
            lua_pushnumber(L, res);
            break;
        }
        case 'x':
        {
            break;
        }
        case 'f':
        {
            float f;
            memcpy(&f, data + pos, size);
            correctbytes((char *)&f, sizeof(f), h.endian);
            lua_pushnumber(L, f);
            break;
        }
        case 'd':
        {
            double d;
            memcpy(&d, data + pos, size);
            correctbytes((char *)&d, sizeof(d), h.endian);
            lua_pushnumber(L, d);
            break;
        }
        case 'c':
        {
            if (size == 0)
            {
                if (!lua_isnumber(L, -1))
                    luaL_error(L, "format `c0' needs a previous size");
                size = lua_tonumber(L, -1);
                lua_pop(L, 1);
                luaL_argcheck(L, pos + size <= ld, 2, "data string too short");
            }
            lua_pushlstring(L, data + pos, size);
            break;
        }
        case 's':
        {
            const char *e = (const char *)memchr(data + pos, '\0', ld - pos);
            if (e == NULL)
                luaL_error(L, "unfinished string in data");
            size = (e - (data + pos)) + 1;
            lua_pushlstring(L, data + pos, size - 1);
            break;
        }
        default:
            commoncases(L, opt, &fmt, &h);
        }
        pos += size;
    }
    lua_pushinteger(L, pos + 1);
    return lua_gettop(L) - 2;
}

/* }====================================================== */

static const luaL_Reg thislib[] = {
    {"pack", b_pack},
    {"unpack", b_unpack},
    {NULL, NULL}};

LUALIB_API int luaopen_struct(lua_State *L)
{
    luaL_newlib(L, thislib);
    lua_setglobal(L, "struct");
    return 1;
}
