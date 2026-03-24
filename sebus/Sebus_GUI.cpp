#include <SFML/Graphics.hpp>
#include <SFML/Main.hpp>
#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>

// --- Windows Entry Point Bridge ---
#ifdef _WIN32
extern int main(int argc, char* argv[]);
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
    return main(__argc, __argv);
}
#endif

// --- Engine Structures ---
struct PhysicsObject {
    bool enabled = false;
    bool isSolid = false;
    sf::Vector2f velocity = { 0.f, 0.f };
    float gravity = 0.6f;
    float friction = 0.90f;
};

// --- The Engine Class ---
class SebusEngine {
private:
    std::map<std::string, sf::RectangleShape> boxes;
    std::map<std::string, PhysicsObject> physicsData;
    std::map<std::string, bool> isUI; 
    std::map<std::string, sf::Keyboard::Key> keyMap;
    std::map<std::string, int> labels; 
    std::vector<std::string> scriptLines;
    int pc = 0; 
    bool firstRun = true;

    std::string clean(std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    void triggerCrash(int lineNum, std::string command, std::string target) {
        std::string msg = "Sebus error!!!\nCrash!!! Error on line " + std::to_string(lineNum) + 
                         ": " + command + "(" + target + ") unable to add physics to object!\n\n" +
                         "Suggestion: UI is not something you can add physics to.";
        MessageBoxA(NULL, msg.c_str(), "Execution Error", MB_ICONERROR | MB_OK);
        exit(1);
    }

public:
    bool scriptLoaded = false;
    sf::Color bgColor = sf::Color(15, 15, 15);

    SebusEngine() {
        for (int i = 0; i < 26; ++i) keyMap["keyboard." + std::string(1, 'a' + i)] = static_cast<sf::Keyboard::Key>(i);
        keyMap["keyboard.space"] = sf::Keyboard::Key::Space;
        keyMap["keyboard.larrow"] = sf::Keyboard::Key::Left;
        keyMap["keyboard.rarrow"] = sf::Keyboard::Key::Right;
        keyMap["keyboard.uarrow"] = sf::Keyboard::Key::Up;
        keyMap["keyboard.darrow"] = sf::Keyboard::Key::Down;
    }

    bool load(std::string path) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        
        // Reset engine state for a fresh load
        scriptLines.clear(); labels.clear(); boxes.clear(); 
        physicsData.clear(); isUI.clear(); pc = 0; firstRun = true;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line.substr(0, 2) == "//") continue;
            if (line[0] == '(' && line.find(')') != std::string::npos) {
                labels[line.substr(1, line.find(')') - 1)] = (int)scriptLines.size();
            }
            scriptLines.push_back(line);
        }
        scriptLoaded = true;
        return true;
    }

    bool checkInput(std::string line, sf::RenderWindow& window) {
        std::string s = clean(line);
        if (s.find("=1") == std::string::npos) return false;
        sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        if (s.find("mouse.click(") != std::string::npos) {
            std::string target = s.substr(s.find('(') + 1, s.find(')') - s.find('(') - 1);
            if (boxes.count(target) && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
                if (boxes[target].getGlobalBounds().contains(worldPos)) return true;
            return false;
        }
        for (auto const& [name, code] : keyMap) if (s.find(name) != std::string::npos) return sf::Keyboard::isKeyPressed(code);
        return false;
    }

    void processCommand(std::string line, sf::RenderWindow& window, int lineNum) {
        if (line.find("window.title") != std::string::npos) {
            size_t s = line.find("\""), e = line.rfind("\"");
            if (s != std::string::npos) window.setTitle(line.substr(s + 1, e - s - 1));
            return;
        }
        if (line.find("window.color") != std::string::npos) {
            std::stringstream ss(line.substr(line.find('(') + 1));
            int r, g, b; if (ss >> r >> g >> b) bgColor = sf::Color(r, g, b);
            return;
        }
        if (line.find('(') == std::string::npos) return;
        std::string cmd = clean(line.substr(0, line.find('(')));
        std::string target = line.substr(line.find('(') + 1, line.find(')') - line.find('(') - 1);
        std::string params = line.substr(line.find(')') + 1);

        if (cmd == "createui") { boxes[target] = sf::RectangleShape({ 100.f, 50.f }); isUI[target] = true; }
        else if (cmd == "createbox") { boxes[target] = sf::RectangleShape({ 50.f, 50.f }); isUI[target] = false; }
        else if (cmd == "collide") { if (isUI[target]) triggerCrash(lineNum, "Collide", target); physicsData[target].isSolid = true; }
        else if (cmd == "physics") { if (isUI[target]) triggerCrash(lineNum, "Physics", target); physicsData[target].enabled = true; }
        else if (cmd == "size" && boxes.count(target)) {
            std::stringstream ss(params); float w, h; if (ss >> w >> h) boxes[target].setSize({ w, h });
        }
        else if (cmd == "pos" && boxes.count(target) && (firstRun || isUI[target])) {
            std::stringstream ss(params); float x, y; if (ss >> x >> y) boxes[target].setPosition({ 400.f + x, 300.f - y });
        }
        else if (cmd == "color" && boxes.count(target)) {
            std::stringstream ss(params); int r, g, b; if (ss >> r >> g >> b) boxes[target].setFillColor(sf::Color(r, g, b));
        }
    }

    void updatePhysics() {
        for (auto& [name, phys] : physicsData) {
            if (phys.enabled && boxes.count(name) && !isUI[name]) {
                phys.velocity.y += phys.gravity; phys.velocity.x *= phys.friction;
                boxes[name].move({ phys.velocity.x, 0.f });
                for (auto& [oN, oB] : boxes) if (name != oN && physicsData[oN].isSolid)
                    if (boxes[name].getGlobalBounds().findIntersection(oB.getGlobalBounds())) { boxes[name].move({ -phys.velocity.x, 0.f }); phys.velocity.x = 0; }
                boxes[name].move({ 0.f, phys.velocity.y });
                for (auto& [oN, oB] : boxes) if (name != oN && physicsData[oN].isSolid)
                    if (boxes[name].getGlobalBounds().findIntersection(oB.getGlobalBounds())) { boxes[name].move({ 0.f, -phys.velocity.y }); phys.velocity.y = 0; }
            }
        }
    }

    void runFrame(sf::RenderWindow& window) {
        if (!scriptLoaded || scriptLines.empty()) return;
        int safety = 0;
        while (pc < (int)scriptLines.size() && safety < 1000) {
            std::string line = scriptLines[pc];
            if (clean(line).find("loop(infinite){") == 0) break;
            processCommand(line, window, pc + 1); pc++; safety++;
        }
        updatePhysics(); firstRun = false;
    }

    void render(sf::RenderWindow& window) { for (auto const& [n, s] : boxes) window.draw(s); }
};

// --- GUI Helper ---
std::string openFileDialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Sebus Game Script (*.gs)\0*.gs\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) return std::string(szFile);
    return "";
}

int main(int argc, char* argv[]) {
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "Sebus Engine + GUI");
    window.setFramerateLimit(60);
    SebusEngine engine;

    // GUI Load Button
    sf::RectangleShape loadBtn({ 200.f, 60.f });
    loadBtn.setFillColor(sf::Color(255, 128, 0));
    loadBtn.setOrigin({ 100.f, 30.f });
    loadBtn.setPosition({ 400.f, 300.f });

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            
            if (const auto* mouseEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (!engine.scriptLoaded && mouseEvent->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    if (loadBtn.getGlobalBounds().contains(mPos)) {
                        std::string file = openFileDialog();
                        if (!file.empty()) engine.load(file);
                    }
                }
            }
        }

        window.clear(engine.scriptLoaded ? engine.bgColor : sf::Color(30, 30, 30));

        if (!engine.scriptLoaded) {
            window.draw(loadBtn);
        } else {
            engine.runFrame(window);
            engine.render(window);
        }

        window.display();
    }
    return 0;
}