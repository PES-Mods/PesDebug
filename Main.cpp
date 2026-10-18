#include "plugin.h"
#include <vector>
#include <list>
#include "PesImGui.h"
#include "Config.hpp"

using namespace plugin;
using namespace std;

std::list<std::string> DebugMessages;
std::list<std::wstring> DebugMessagesW;

bool initialized = false;

static CRITICAL_SECTION *cs() {
    static CRITICAL_SECTION cs;
    return &cs;
}

void CopyListToClipboard(const std::list<std::string> &strList) {
    if (strList.empty()) return;
    size_t totalSize = 0;
    for (const auto &str : strList)
        totalSize += str.size() + 2;
    totalSize += 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, totalSize);
    if (!hMem) return;
    char *buffer = static_cast<char *>(GlobalLock(hMem));
    if (!buffer) {
        GlobalFree(hMem);
        return;
    }
    char *ptr = buffer;
    for (const auto &str : strList) {
        memcpy(ptr, str.c_str(), str.size());
        ptr += str.size();
    }
    *ptr = '\0';
    GlobalUnlock(hMem);
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        SetClipboardData(CF_TEXT, hMem);
        CloseClipboard();
    }
    else
        GlobalFree(hMem);
}

void OnCreate() {
    IMGUI_SYNC
    ImGuiCheckVersion();
}

void OnDestroy() {
    IMGUI_SYNC
}

void OnRender() {
    IMGUI_SYNC
        static size_t last_size = 0;
    if (ImGui::Button("Clear Log"))
        DebugMessages.clear();
    ImGui::SameLine();
    if (ImGui::Button("Copy Log"))
        CopyListToClipboard(DebugMessages);
    std::string message;
    for (auto const &s : DebugMessages)
        message += s;
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), true);
    ImGui::TextUnformatted(message.c_str());
    if (message.size() != last_size) {
        ImGui::SetScrollHereY(1.0f);
        last_size = message.size();
    }
    ImGui::EndChild();
}

void MyDebugPrint(std::string const &message) {
    if (initialized) {
        EnterCriticalSection(cs());
        while (DebugMessages.size() >= 1000)
            DebugMessages.pop_front();
        DebugMessages.push_back(message);
        LeaveCriticalSection(cs());
    }
}

void __stdcall MyOutputDebugStringA(LPCSTR lpOutputString) {
    MyDebugPrint(lpOutputString);
}

extern "C" __declspec(dllexport) void DebugPrint(char const *message) {
    MyDebugPrint(message);
}

class PesDebug {
public:
    PesDebug() {
        if (!CheckPluginName(Magic<'P','e','s','D','e','b','u','g','.','a','s','i'>()))
            return;
        auto v = PES::GetAppVersion();
        if (v.id() != ID_PES5 && v.id() != ID_PES6)
            return;
        Config config;
        config.Read(PES::GameDirPath(L"plugins\\PesDebug.ini"));
        if (!config.ENABLED)
            return;
        if (!ImGuiEnsureLoaded())
            return;
        InitializeCriticalSection(cs());
        initialized = true;
        ImGuiOnCreate(OnCreate);
        ImGuiOnRender(OnRender);
        ImGuiOnDestroy(OnDestroy);
        ImGuiEnableMouseCursor();
        switch (v.id()) {
        case ID_PES5:
            patch::SetPointer(0xACE150, MyOutputDebugStringA);
            break;
        case ID_PES6:
            patch::SetPointer(0xB7D1BC, MyOutputDebugStringA);
            break;
        }
    }

    ~PesDebug() {
        if (initialized) {
            ImGuiRemoveOnCreate(OnCreate);
            ImGuiRemoveOnRender(OnRender);
            ImGuiRemoveOnDestroy(OnDestroy);
            DeleteCriticalSection(cs());
        }
    }
} pesDebug;
