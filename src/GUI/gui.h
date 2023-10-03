#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <imgui/imgui.h>
#include "../Net/NetClient.h"

class CalcResponse;
class GuiRender {
    friend CalcResponse;
    
    ImVec4 m_clearClr = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    HWND m_hWnd = nullptr;

    NetClient m_tcp;
    std::string m_text;
    std::vector<std::string> m_operations;
    bool m_isSend = false;
    
    [[noreturn]] void aliveRunner();
    void body();
public:
    GuiRender(): m_tcp("127.0.0.1", 1000) {
        m_tcp.OnDisconnect = [this] {
            m_isSend = false;
        };
        std::thread(&GuiRender::aliveRunner, this).detach();
    }
    
    int loop();
    void frame();

    void refresh();
    void clear();
    void del();
    void calculate();
    
    void number(char number);
    void op(char op);

    void onKeyDown(WPARAM wParam, bool isShift);
    
    ImVec2 GetResolution() const;

    static GuiRender* instance;
};