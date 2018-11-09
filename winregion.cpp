#include <windows.h>
#include <string>
#include <stdio.h>
#include "winregion.h"

class WinRegion
{
    public:

        WinRegion();
        WinRegion(std::string path);
        WinRegion(unsigned width, unsigned height);
        ~WinRegion();

        void safeAddPoint(unsigned x, unsigned y);
        void addPoint(unsigned x, unsigned y);
        void safeRemovePoint(unsigned x, unsigned y);
        void removePoint(unsigned x, unsigned y);

        void clear();

        void saveToFile(std::string path);
        void loadFromFile(std::string path);
        void loadFromImageBuffer(unsigned char* buffer, unsigned width, unsigned height);

        HRGN getRegion() {return m_region;}

    private:
        HRGN        m_region;
        unsigned    m_width, m_height;
        bool        loaded;
};

WinRegion::WinRegion(unsigned width, unsigned height)
{
    m_region = CreateRectRgn(0, 0, width, height);
    m_width = width;
    m_height = height;
    loaded = true;
}

WinRegion::WinRegion() : loaded(false) {};

WinRegion::WinRegion(std::string path) : loaded(false)
{
    loadFromFile(path);
}

WinRegion::~WinRegion()
{
    clear();
}

void WinRegion::clear()
{
    if (!loaded)
        return;

    if (m_region)
        DeleteObject(m_region);

    loaded = false;
}

void WinRegion::safeAddPoint(unsigned x, unsigned y)
{
    if (x >= m_width || y >= m_height || !x || !y)
        return;
    addPoint(x, y);
}

void WinRegion::addPoint(unsigned x, unsigned y)
{
    HRGN PointRegion = CreateRectRgn(x, y, x + 1, y + 1);
    CombineRgn(m_region, m_region, PointRegion, RGN_OR);
    DeleteObject(PointRegion);
}

void WinRegion::safeRemovePoint(unsigned x, unsigned y)
{
    if (x >= m_width || y >= m_height || !x || !y)
        return;
    removePoint(x, y);
}

void WinRegion::removePoint(unsigned x, unsigned y)
{
    HRGN PointRegion = CreateRectRgn(x, y, x + 1, y + 1);
    CombineRgn(m_region, m_region, PointRegion, RGN_DIFF);
    DeleteObject(PointRegion);
}

void WinRegion::saveToFile(std::string path)
{
    DWORD bytesRequired = GetRegionData(m_region, 0, NULL);
    RGNDATA* regionData = (RGNDATA*) new char[bytesRequired];
    GetRegionData(m_region, bytesRequired, regionData);
    FILE* file = fopen(path.c_str(), "wb");
    fwrite(&(regionData->rdh), 1, sizeof(RGNDATAHEADER), file);
    fwrite(regionData->Buffer, 1, bytesRequired - sizeof(RGNDATAHEADER), file);
    fclose(file);
    delete [] regionData;
}

void WinRegion::loadFromFile(std::string path)
{
    if (loaded)
        clear();
    FILE* file = fopen(path.c_str(), "rb");
    if (!file)
    {
        printf("File from path %s not found.\n", path.c_str());
        return;
    }
    RGNDATAHEADER dataHeader;
    fread(&dataHeader, 1, sizeof(RGNDATAHEADER), file);
    RGNDATA* regionData = (RGNDATA*)new char[dataHeader.nRgnSize + sizeof(RGNDATAHEADER)];
    regionData->rdh = dataHeader;
    fread(regionData->Buffer, 1, dataHeader.nRgnSize, file);
    m_region = ExtCreateRegion(NULL, dataHeader.nRgnSize + sizeof(RGNDATAHEADER), regionData);
    loaded = m_region != NULL;
    m_width = dataHeader.rcBound.right - dataHeader.rcBound.left;
    m_height = dataHeader.rcBound.bottom - dataHeader.rcBound.top;
    delete [] regionData;
    fclose(file);
}

void WinRegion::loadFromImageBuffer(unsigned char* buffer, unsigned width, unsigned height)
{
    if (!loaded || width != m_width || height != m_height)
    {
        clear();
        m_region = CreateRectRgn(0, 0, width, height);
        m_width = width;
        m_height = height;
    }

    if (!width || !height)
        return;

    unsigned char* bufferend = buffer + width * height * 4;
    unsigned x = 0, y = 1;

    while (buffer < bufferend)
    {
        x++;
        if (x > width)
        {
            x = 1;
            y++;
        }
        unsigned char A = *(buffer + 3);
        buffer += 4;
        if (!A)
            removePoint(x - 1, y - 1);
    }
    loaded = true;
}

#define metaname "LIBWIN_WINREGION"

int WRCreate(lua_State* L)
{
    unsigned width = 0, height = 0;
    std::string path;
    if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
    {
        width = lua_tonumber(L, 1);
        height = lua_tonumber(L, 2);
    }
    else if (lua_isstring(L, 1))
    {
        path = lua_tostring(L, 1);
    }
    WinRegion* region = (WinRegion*)lua_newuserdata(L, sizeof(WinRegion));
    if (width && height)
        new (region) WinRegion(width, height);
    else
        new (region) WinRegion(path);
    luaL_setmetatable(L, metaname);
    return 1;
}

int WRCall(lua_State* L)
{
    lua_remove(L, 1);
    return WRCreate(L);
}

int WRSaveToFile(lua_State* L)
{
    WinRegion* region = (WinRegion*)luaL_checkudata(L, 1, metaname);
    region->saveToFile(std::string(lua_tostring(L, 2)));
    return 0;
}

int WRCollect(lua_State* L)
{
    WinRegion* region = (WinRegion*)luaL_testudata(L, 1, metaname);
    if (region)
        region->clear();
    return 0;
}

int WRCropWindow(lua_State* L)
{
    WinRegion* reg = (WinRegion*)luaL_checkudata(L, 1, metaname);
    HWND handle = (HWND)lua_topointer(L, 2);
    HRGN region = CreateRectRgn(0, 0, 0, 0);
    CombineRgn(region, reg->getRegion(), NULL, RGN_COPY);
    if (!SetWindowRgn(handle, region, true))
        DeleteObject(region);
    return 0;
}

int WRLoadImage(lua_State* L)
{
    WinRegion* region = (WinRegion*)luaL_checkudata(L, 1, metaname);
    if (!lua_islightuserdata(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
        return 0;
    region->loadFromImageBuffer((unsigned char*)lua_topointer(L, 2),
                                lua_tonumber(L, 3), lua_tonumber(L, 4));
    return 0;
}

void lua_register_winreg(lua_State* L)
{
    lua_newtable(L);

        lua_pushcfunction(L, WRCreate);
        lua_setfield(L, -2, "create");

        lua_pushcfunction(L, WRSaveToFile);
        lua_setfield(L, -2, "save");

        lua_pushcfunction(L, WRCropWindow);
        lua_setfield(L, -2, "crop");

        lua_pushcfunction(L, WRLoadImage);
        lua_setfield(L, -2, "loadfromimage");

        luaL_newmetatable(L, metaname);

            lua_pushcfunction(L, WRCollect);
            lua_setfield(L, -2, "__gc");

            lua_pushvalue(L, -2);
            lua_setfield(L, -2, "__index");

            lua_pushvalue(L, -2);
            lua_setfield(L, -2, "__metatable");

        lua_pop(L, 1);

        lua_newtable(L);

            lua_pushcfunction(L, WRCall);
            lua_setfield(L, -2, "__call");

            lua_setmetatable(L, -2);

    lua_setfield(L, -2, "region");
}

