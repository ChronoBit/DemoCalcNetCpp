#include "gui.h"
#include "utils.h"
#include "../Packets.h"

GuiRender* GuiRender::instance;

#define NUMBER_BTN(num) if (ImGui::Button(num, {50, 50})) number((num)[0])
#define OPERATION_BTN(oper) if (ImGui::Button(oper, {50, 50})) op((oper)[0])

void GuiRender::aliveRunner() {
    while (true) {
        Sleep(5000);
        if (m_tcp.IsConnected()) {
            AliveTickP tick;
            m_tcp.Send(tick);
        }
    }
}

void GuiRender::body() {
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##input", m_text.data(), m_text.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::Spacing();

    NUMBER_BTN("7");
    ImGui::SameLine();
    NUMBER_BTN("8");
    ImGui::SameLine();
    NUMBER_BTN("9");
    ImGui::SameLine();
    OPERATION_BTN("/");
    ImGui::Spacing();
    
    NUMBER_BTN("4");
    ImGui::SameLine();
    NUMBER_BTN("5");
    ImGui::SameLine();
    NUMBER_BTN("6");
    ImGui::SameLine();
    OPERATION_BTN("*");
    ImGui::Spacing();
    
    NUMBER_BTN("1");
    ImGui::SameLine();
    NUMBER_BTN("2");
    ImGui::SameLine();
    NUMBER_BTN("3");
    ImGui::SameLine();
    OPERATION_BTN("-");
    ImGui::Spacing();
    
    NUMBER_BTN("0");
    ImGui::SameLine();
    NUMBER_BTN(".");
    ImGui::SameLine();
    if (m_tcp.IsConnected()) {
        if (ImGui::Button("=", {50, 50})) calculate();
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("=", {50, 50});
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    OPERATION_BTN("+");
    ImGui::Spacing();
    
    if (ImGui::Button("C", {108, 50})) clear();
    ImGui::SameLine();
    if (ImGui::Button("Del", {108, 50})) del();
}

void GuiRender::frame() {
    constexpr auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;
    const auto sz = GetResolution();
    ImGui::SetNextWindowSize(sz);
    ImGui::SetNextWindowPos({0, 0});
    
    ImGui::Begin("Calculator", nullptr, flags);
    body();
    ImGui::End();
}

void GuiRender::refresh() {
    if (m_operations.empty()) {
        m_text = "0";
        return;
    }
    m_text = join(m_operations);
}

void GuiRender::clear() {
    m_operations.clear();
    refresh();
}

void GuiRender::del() {
    if (m_isSend) return;
    if (m_operations.empty()) return;
    auto& last = m_operations.back();
    last = last.substr(0, last.size() - 1);
    *m_operations.rbegin() = last;
    if (last.empty()) {
        m_operations.pop_back();
    }
    refresh();
}

void GuiRender::calculate() {
    if (m_isSend || !m_tcp.IsConnected() || m_operations.size() < 2) return;
    
    if (const auto& last = m_operations.back(); last.back() == '.') {
        *m_operations.rbegin() = last.substr(0, last.size() - 1);
        refresh();
    }
    
    m_isSend = true;
    CalcRequest req;
    req.operations = m_operations;
    if (m_operations.front()[0] == '-') {
        req.operations.insert(req.operations.begin(), "0");
    }
    m_tcp.Send(req);
}

void CalcResponse::Parse() {
    auto& gui = GuiRender::instance;
    if (auto res = toString(result); res.front() == '-') {
        res = res.substr(1);
        gui->m_operations.assign({"-", res});
    } else {
        gui->m_operations.assign({res});
    }
    gui->m_isSend = false;
    gui->refresh();
}

void GuiRender::number(char number) {
    if (m_isSend) return;
    switch (number) {
        default:
            return;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '0':
        case '.':
            break;
    }
    if (m_operations.empty()) {
        if (number == '0') return;
        if (number == '.') {
            m_operations.emplace_back("0.");
        } else {
            m_operations.emplace_back(&number, 1);
        }
    } else {
        switch (m_operations.back()[0]) {
            case '+':
            case '-':
            case '*':
            case '/':
                if (number == '0') return;
                if (number == '.') {
                    m_operations.emplace_back("0.");
                } else {
                    m_operations.emplace_back(&number, 1);
                }
                break;
            default:
                auto& last = m_operations.back();
                if (number == '.' && contains(last, '.')) return;
                if (number == '0' && last[0] == '0') return;
                last += number;
                break;
        }
    }
    refresh();
}

void GuiRender::op(char op) {
    if (m_isSend) return;
    if (m_operations.empty()) {
        if (op == '-') goto print;
        return;
    }
    if (const auto& last = m_operations.back(); last.back() == '.') {
        *m_operations.rbegin() = last.substr(0, last.size() - 1);
    } else {
        switch (m_operations.back()[0]) {
            case '+':
            case '-':
            case '*':
            case '/':
                return;
        }
    }
    print:
    m_operations.emplace_back(&op, 1);
    refresh();
}

#define ON_NUMBER(key, num) case key: number(num); break
#define ON_OPERATOR(key, num) case key: op(num); break

enum {
    VK_KEY_0 = 0x30,
    VK_KEY_1 = 0x31,
    VK_KEY_2 = 0x32,
    VK_KEY_3 = 0x33,
    VK_KEY_4 = 0x34,
    VK_KEY_5 = 0x35,
    VK_KEY_6 = 0x36,
    VK_KEY_7 = 0x37,
    VK_KEY_8 = 0x38,
    VK_KEY_9 = 0x39,
};

void GuiRender::onKeyDown(WPARAM wParam, bool isShift) {
    switch (wParam) {
        ON_NUMBER(VK_KEY_0, '0');
        ON_NUMBER(VK_KEY_1, '1');
        ON_NUMBER(VK_KEY_2, '2');
        ON_NUMBER(VK_KEY_3, '3');
        ON_NUMBER(VK_KEY_4, '4');
        ON_NUMBER(VK_KEY_5, '5');
        ON_NUMBER(VK_KEY_6, '6');
        ON_NUMBER(VK_KEY_7, '7');
        case VK_KEY_8:
            if (isShift) {
                op('*');
            } else {
                number('8');
            }
            break;
        ON_NUMBER(VK_KEY_9, '9');
        
        ON_NUMBER(VK_NUMPAD0, '0');
        ON_NUMBER(VK_NUMPAD1, '1');
        ON_NUMBER(VK_NUMPAD2, '2');
        ON_NUMBER(VK_NUMPAD3, '3');
        ON_NUMBER(VK_NUMPAD4, '4');
        ON_NUMBER(VK_NUMPAD5, '5');
        ON_NUMBER(VK_NUMPAD6, '6');
        ON_NUMBER(VK_NUMPAD7, '7');
        ON_NUMBER(VK_NUMPAD8, '8');
        ON_NUMBER(VK_NUMPAD9, '9');

        case VK_OEM_PLUS:
            if (isShift) {
                op('+');
            } else {
                calculate();
            }
            break;
        ON_OPERATOR(VK_OEM_MINUS, '-');
        ON_OPERATOR(VK_OEM_2, '/');
        
        ON_OPERATOR(VK_ADD, '+');
        ON_OPERATOR(VK_SUBTRACT, '-');
        ON_OPERATOR(VK_MULTIPLY, '*');
        ON_OPERATOR(VK_DIVIDE, '/');
        case VK_RETURN:
            calculate();
            break;
        case VK_BACK:
            del();
            break;
        case VK_DELETE:
            clear();
            break;
    }
}

ImVec2 GuiRender::GetResolution() const {
    RECT rect;
    GetClientRect(m_hWnd, &rect);
    return {static_cast<float>(rect.right), static_cast<float>(rect.bottom)};
}
