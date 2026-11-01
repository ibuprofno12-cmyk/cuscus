// payload.cpp — MinGW64 — triggerbot externo con focus por username (detección visual)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

struct Cfg {
    char   target[32];
    int    range;
    int    delay_ms;
    int    crit_only;
    int    enabled;
    DWORD  last_click;
};
static Cfg g = { "", 6, 550, 0, 0, 0 };
static HWND g_game = NULL;

static void on_command(const char *line) {
    if (strncasecmp(line, "/tb focus ", 10)==0) {
        strncpy(g.target, line+10, sizeof(g.target)-1);
        g.target[sizeof(g.target)-1]=0;
        size_t L=strlen(g.target);
        while (L && (g.target[L-1]=='\r'||g.target[L-1]=='\n'||g.target[L-1]==' ')) g.target[--L]=0;
        g.enabled=1; return;
    }
    if (strncasecmp(line, "/tb range ", 10)==0) { g.range=atoi(line+10); return; }
    if (strncasecmp(line, "/tb delay ", 10)==0) { g.delay_ms=atoi(line+10); return; }
    if (strncasecmp(line, "/tb crit ", 9)==0)   { g.crit_only=(strstr(line,"on")!=NULL); return; }
    if (strncasecmp(line, "/tb off", 7)==0)     { g.enabled=0; return; }
    if (strncasecmp(line, "/tb on", 6)==0)      { g.enabled=1; return; }
}

static DWORD WINAPI console_thread(LPVOID) {
    AllocConsole();
    freopen("CONOUT$","w",stdout);
    freopen("CONIN$","r",stdin);
    SetConsoleTitleA("tb");
    printf("tb commands:\n  /tb focus <username>\n  /tb range <n>\n  /tb delay <ms>\n  /tb crit on|off\n  /tb on | /tb off\n");
    char buf[256];
    while (fgets(buf, sizeof(buf), stdin)) on_command(buf);
    return 0;
}

static HDC g_dc=NULL; static HBITMAP g_bmp=NULL; static int g_w=0,g_h=0;

static void init_capture(HWND game) {
    HDC wdc = GetDC(game);
    RECT r; GetClientRect(game,&r);
    g_w=r.right; g_h=r.bottom;
    g_dc = CreateCompatibleDC(wdc);
    g_bmp = CreateCompatibleBitmap(wdc, g_w, g_h);
    SelectObject(g_dc, g_bmp);
    ReleaseDC(game, wdc);
}

static int crosshair_hits_player() {
    if (!g_dc || !g_game) return 0;
    HDC wdc = GetDC(g_game);
    BitBlt(g_dc,0,0,g_w,g_h,wdc,0,0,SRCCOPY);
    ReleaseDC(g_game,wdc);
    const int cx=g_w/2, cy=g_h/2, R=10;
    int white=0;
    for (int y=cy-R;y<cy+R;++y) for (int x=cx-R;x<cx+R;++x) {
        COLORREF c = GetPixel(g_dc,x,y);
        int r=GetRValue(c), gg=GetGValue(c), b=GetBValue(c);
        if (r>200&&gg>200&&b>200&&abs(r-gg)<20&&abs(gg-b)<20) ++white;
    }
    return white>30;
}

static DWORD WINAPI tb_loop(LPVOID) {
    while (1) {
        Sleep(10);
        if (!g.enabled || !g.target[0]) continue;
        if (!g_game || !IsWindow(g_game)) {
            g_game = FindWindowA(NULL,"Minecraft* 1.21.11");
            if (!g_game) g_game = FindWindowA(NULL,"Minecraft");
            if (g_game) init_capture(g_game);
            continue;
        }
        if (!IsWindowVisible(g_game)) continue;
        DWORD now = GetTickCount();
        if (now - g.last_click < (DWORD)g.delay_ms) continue;
        if (!crosshair_hits_player()) continue;
        INPUT inp[2]; memset(inp,0,sizeof(inp));
        inp[0].type=INPUT_MOUSE; inp[0].mi.dwFlags=MOUSEEVENTF_LEFTDOWN;
        inp[1].type=INPUT_MOUSE; inp[1].mi.dwFlags=MOUSEEVENTF_LEFTUP;
        SendInput(2, inp, sizeof(INPUT));
        g.last_click = now;
    }
    return 0;
}

extern "C" __declspec(dllexport) BOOL WINAPI DllMain(HMODULE,DWORD reason,LPVOID) {
    if (reason==DLL_PROCESS_ATTACH) {
        RegisterHotKey(NULL,1,0,VK_INSERT);
        CreateThread(NULL,0,console_thread,NULL,0,NULL);
        CreateThread(NULL,0,tb_loop,NULL,0,NULL);
    }
    return TRUE;
}