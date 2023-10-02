#include <algorithm>
#include <exception>
#include <fstream>
#include <functional>
#include <raylib.h>
#include <iostream>
#include <cstdio>
#include <sstream>
#include <string>
#include <array>
#include <stdexcept>
#include <memory>
#include <tuple>
#include <vector>
#include <math.h>

std::string execCommand(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    try {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

std::vector<std::string> splitStr(const std::string &input, char delimiter){
    std::vector<std::string> res;
    try {
        std::string str = "";
        for(int i = 0; i < input.length(); i++){
            char c = input[i];
            if(c == delimiter) {
                res.push_back(str);
                str.clear();
                continue;
            }
            str += c;
        }
        res.push_back(str);
    } catch (std::exception &ex) {
        std::cout << "Split Error: " << ex.what();
    }
    return res;
}

struct Monitor{
public:
    std::string name;
    int width;
    int height;
    int x;
    int y;
    bool isSelected = false;
    
    void draw() const {
        const auto rect = this->getRenderRect();
        DrawRectangleRounded(rect, 0.15, 50, this->isSelected ? RED : BLACK);
        DrawText(this->name.c_str(), rect.x + 6, rect.y + 6, 20, WHITE);
    }

    bool hasPointer(int x, int y) const {
        const auto rect = this->getRenderRect();
        return  x > rect.x &&
                x < rect.x + rect.width &&
                y > rect.y &&
                y < rect.y + rect.height;
    }

    void setInnerDragAnchor(int mX, int mY) {
        const auto rect = this->getRenderRect();
        this->mpx = mX - rect.x;
        this->mpy = mY - rect.y;
    }

    void slide(int mX, int mY, const std::vector<Monitor> &monitors) {
        auto rect = this->getRenderRect();
        auto deltaX = mX - rect.x - mpx;
        auto deltaY = mY - rect.y - mpy;
    
        for(int i = 0; i < monitors.size(); i++){
            auto pMonitor = &monitors.at(i);
            if(pMonitor == this) continue;
            auto [bounceX, bounceY ] = this->collideAndGetBounce(pMonitor);
            // std::cout << "Should Bound: ( " << bounceX << ", " << bounceY << " )\n";
            deltaX += bounceX;
            deltaY += bounceY;
        }

        rect.x += deltaX;
        rect.y += deltaY;

        this->unpackScreenCoordsToReal(rect.x, rect.y);

        setInnerDragAnchor(mX, mY);
    }

private:
    std::tuple<int, int> collideAndGetBounce(const Monitor *other) {
        int deltaX = 0, deltaY = 0;
        const auto thisRect = this->getRenderRect();
        const auto otherRect = other->getRenderRect();

        if( thisRect.x < otherRect.x + thisRect.width &&
            thisRect.x + thisRect.width > otherRect.x &&
            thisRect.y < otherRect.y + otherRect.height &&
            thisRect.y + thisRect.height > otherRect.y) {

            float xOverlap  = std::min(thisRect.x + thisRect.width, otherRect.x + otherRect.width) 
                            - std::max(thisRect.x, otherRect.x);
            float yOverlap  = std::min(thisRect.y + thisRect.height, otherRect.y + otherRect.height) 
                            - std::max(thisRect.y, otherRect.y);

            if (xOverlap < yOverlap) {
                // Resolve along the X axis
                if (thisRect.x < otherRect.x) {
                    deltaX = -xOverlap;
                } else {
                    deltaX = xOverlap;
                }
            } else {
                // Resolve along the Y axis
                if (thisRect.y < otherRect.y) {
                    deltaY = -yOverlap;
                } else {
                    deltaY = yOverlap;
                }
            }
        }
        return std::tuple<int, int>(deltaX, deltaY);
    }
    void unpackScreenCoordsToReal(int x, int y) {
        this->x = (800.f / 2 - x) * 10;
        this->y = (500.f / 2 - y) * 10;
    }

public:
    Rectangle getRenderRect() const {
        Rectangle rect;
        const auto eWidth = (float)this->width / 10.f;
        const auto eHeight = (float)this->height / 10.f;
        rect.width = eWidth;
        rect.height = eHeight;
        rect.x = 800.f / 2 - (float)this->x / 10.f;
        rect.y = 500.f / 2 - (float)this->y / 10.f;
        return rect;
    }

private:
    
    //mp_ is the mouse position within the render rect, when dragging. (drag anchor, if you will)
    int mpx = 0;
    int mpy = 0;
};

std::ostream& operator<<(std::ostream& os, const Monitor& monitor){
    os  << "Display: " << monitor.name << "\n"
        << "Width: " << monitor.width << "\n"
        << "Height: " << monitor.height << "\n"
        << "X: " << monitor.x << "\n"
        << "Y: " << monitor.y << "\n";
    return os;
}

std::vector<Monitor> getMonitorsData() {
    std::vector<Monitor> monitors;
    try {
        const auto rawRes = execCommand("xrandr | grep \" connected \"");
        const auto lines = splitStr(rawRes, '\n');
        for(auto line : lines){
            Monitor monitor = Monitor();

            // //splitting the line by spaces
            const auto parts = splitStr(line, ' ');
            if(parts.size() < 3) continue;
            const auto display = parts[0];
            const auto props = parts[2];

            //setting monitor name (label/id)
            monitor.name = display;

            //finding delimiter of resolution/position of display
            const auto negIdx = props.find('-');
            const auto pluIdx = props.find('+');
            const auto posSplitIdx = std::min(pluIdx, negIdx);

            //parsing the resolution string
            const auto resRaw = props.substr(0, posSplitIdx);
            const auto resParts = splitStr(resRaw, 'x');
            monitor.width = std::stoi(resParts[0]);
            monitor.height = std::stoi(resParts[1]);

            //parsing the position string
            const auto posRaw = props.substr(posSplitIdx + 1, props.length() - 1);
            const auto xSign = props[posSplitIdx] == '+' ? 1 : -1;
            //height sign index
            const auto negIdx2 = posRaw.find('-');
            const auto pluIdx2 = posRaw.find('+');
            const auto heightSplitIdx = std::min(negIdx2, pluIdx2);
            const auto ySign = posRaw[heightSplitIdx] == '+' ? 1 : -1;

            monitor.x = std::stoi(posRaw.substr(0, heightSplitIdx)) * xSign;
            monitor.y = std::stoi(posRaw.substr(heightSplitIdx + 1, posRaw.length() - 1)) * ySign;

            monitors.push_back(monitor);
        }

    } catch (std::exception &ex) {
        std::cout << "Monitor Info Fetch Error: " << ex.what(); 
    }
    return monitors;
}

void saveStateToJson(const std::vector<Monitor> &monitors){
    std::string json;
    std::ofstream file;
    file.open("output.json");
    file << "[\n";
    for(const auto &monitor : monitors){
        file  << "\t{\n\t\t"
                << "\"display\": " << '"' << monitor.name << '"' << ",\n\t\t"
                << "\"width\": " << '"' << monitor.width << '"' << ",\n\t\t"
                << "\"height\": " << '"' << monitor.height << '"' << ",\n\t\t"
                << "\"x\": " << '"' << monitor.x << '"' << ",\n\t\t"
                << "\"y\": " << '"' << monitor.y << '"' << "\n\t},\n";
    }
    file << "]";
    file.close();
}

int main(void) {
    const auto monitors = getMonitorsData();
    InitWindow(800, 500, "raylibWin");
    SetTargetFPS(165);
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_UNFOCUSED);

    int mX = 0;
    int mY = 0;
    Monitor* selectedMonitor = nullptr;

    while(!WindowShouldClose()) {
        mX = GetMouseX();
        mY = GetMouseY();

        BeginDrawing();
        ClearBackground(WHITE);
        DrawFPS(10, 10);

        DrawText(std::string(std::to_string(mX) + ' ' + std::to_string(mY)).c_str(), 10, 40, 10, BLACK); 
        DrawText(IsMouseButtonDown(0) ? "DOWN" : "UP", 10, 50, 10, BLACK);

        for(int i = 0; i < monitors.size(); i++){
            Monitor &monitor = const_cast<Monitor&>(monitors.at(i));
            monitor.draw();
            if(IsMouseButtonPressed(0)) {
                if(monitor.hasPointer(mX, mY)){
                    if(selectedMonitor != nullptr) selectedMonitor->isSelected = false;
                    selectedMonitor = &monitor;
                    monitor.setInnerDragAnchor(mX, mY);
                    monitor.isSelected = true;
                }
            }
        }

        if(IsMouseButtonReleased(0)) {
            if(selectedMonitor != nullptr) selectedMonitor->isSelected = false;
            selectedMonitor = nullptr;
        } 

        if(selectedMonitor != nullptr) {
            DrawText(selectedMonitor->name.c_str(), 10, 61, 10, BLACK);
            selectedMonitor->slide(mX, mY, monitors);
        }

        if(IsKeyPressed(KEY_S)){
            saveStateToJson(monitors);
        }

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
