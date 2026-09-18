#pragma once

#include <algorithm>
#include <cmath>
#include <imgui.h>

// Keep a real ImGui button underneath the artwork for keyboard navigation.
inline bool iceButton(const char* label, ImVec2 size, bool primary = false)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 0));
    const bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    const bool highlight = ImGui::IsItemHovered() || ImGui::IsItemFocused();
    const ImGuiID id = ImGui::GetItemID();
    ImGuiStorage* storage = ImGui::GetStateStorage();
    float glow = storage->GetFloat(id, 0.0f);
    glow += ((highlight ? 1.0f : 0.0f) - glow) * (1.0f - std::exp(-12.0f * ImGui::GetIO().DeltaTime));
    storage->SetFloat(id, glow);
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    for (int i = 9; i > 0; --i) {
        draw->AddRect(ImVec2(a.x-i, a.y-i), ImVec2(b.x+i, b.y+i),
            IM_COL32(115, 232, 246, static_cast<int>((primary ? 10 : 4) + glow * 13) * (10-i) / 9), 5+i, 0, 2);
    }
    draw->AddRectFilled(a, b, primary ? IM_COL32(35, 86, 101, 245) : IM_COL32(17, 39, 52, 235), 5);
    draw->AddRectFilledMultiColor(ImVec2(a.x+1,a.y+1), ImVec2(b.x-1,b.y-1),
        IM_COL32(159, 240, 251, static_cast<int>(35+glow*40)), IM_COL32(159, 240, 251, static_cast<int>(35+glow*40)),
        IM_COL32(45, 106, 125, 4), IM_COL32(45, 106, 125, 4));
    draw->AddRect(a, b, IM_COL32(163, 239, 247, static_cast<int>(100+glow*150)), 5);
    draw->AddLine(ImVec2(a.x+18,a.y+1), ImVec2(b.x-18,a.y+1), IM_COL32(224, 255, 255, 180), 1);
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const ImVec2 textPos((a.x+b.x-textSize.x)*0.5f, (a.y+b.y-textSize.y)*0.5f + (ImGui::IsItemActive() ? 1 : 0));
    draw->AddText(ImVec2(textPos.x+1,textPos.y+2), IM_COL32(0, 10, 20, 210), label);
    for (int i : {-1, 1}) draw->AddText(ImVec2(textPos.x+i,textPos.y), IM_COL32(130, 240, 255, static_cast<int>(18+glow*35)), label);
    draw->AddText(textPos, IM_COL32(235, 253, 255, 255), label);
    return clicked;
}
