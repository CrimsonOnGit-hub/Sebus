#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Main.hpp>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <set>
#include <memory>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
extern int main(int argc, char* argv[]);
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
    return main(__argc, __argv);
}
#endif

struct PhysicsObject {
    bool enabled = false;
    bool isSolid = false;
    sf::Vector2f velocity = { 0.f, 0.f };
    float gravity = 0.8f;
    float friction = 0.92f;
};

class SebusEngine {
private:
    std::map<std::string, sf::RectangleShape> boxes;
    std::map<std::string, PhysicsObject> physicsData;
    std::map<std::string, bool> isUI;
    std::set<std::string> persistentObjects; 
    std::map<std::string, float> disabledInputs; 
    
    std::map<std::string, std::unique_ptr<sf::Texture>> textures;
    std::list<std::unique_ptr<sf::Music>> activeSounds; 

    std::vector<std::string> scriptLines;
    int pc = 0;
    int loopStartPC = -1;
    bool isLS = false;
    std::string currentFile = "";

    sf::Clock engineClock;
    float delayUntil = 0.f;

    std::string clean(std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    void triggerError(std::string msg) {
        MessageBoxA(NULL, msg.c_str(), "Sebus Engine Error", MB_ICONERROR | MB_OK);
        exit(1);
    }

    void executeLevelScript(std::string path) {
        std::ifstream file(path);
        if (!file.is_open()) { triggerError("Could not find level file: " + path); return; }

        for (auto it = boxes.begin(); it != boxes.end();) {
            if (persistentObjects.find(it->first) == persistentObjects.end()) {
                physicsData.erase(it->first); isUI.erase(it->first); it = boxes.erase(it);
            } else { ++it; }
        }

        bool prevLS = isLS;
        std::string prevFile = currentFile;
        isLS = true;
        currentFile = path;

        std::string line;
        int ln = 1;
        while (std::getline(file, line)) {
            if (line.empty() || line.substr(0, 2) == "//") { ln++; continue; }
            processCommand(line, ln); 
            ln++;
        }

        isLS = prevLS;
        currentFile = prevFile;
    }

public:
    sf::Color bgColor = sf::Color(15, 15, 15);
    bool scriptLoaded = false;
    std::map<std::string, sf::Keyboard::Key> keyMap;

    bool consoleOpen = false;
    std::string consoleInput = "";
    std::list<std::string> engineLogs; 
    sf::Font font;
    bool fontLoaded = false;

    SebusEngine() {
        for (int i = 0; i < 26; ++i) keyMap["keyboard." + std::string(1, 'a' + i)] = static_cast<sf::Keyboard::Key>(i);
        keyMap["keyboard.space"] = sf::Keyboard::Key::Space;
        keyMap["keyboard.left"] = sf::Keyboard::Key::Left;
        keyMap["keyboard.right"] = sf::Keyboard::Key::Right;

        // Robust Font Loading
        const char* fontPaths[] = {"C:/Windows/Fonts/consola.ttf", "C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/tahoma.ttf", "C:/Windows/Fonts/segoeui.ttf"};
        for(const auto& p : fontPaths) {
            if(font.openFromFile(p)) { fontLoaded = true; break; }
        }
    }

    void addLog(std::string msg) {
        engineLogs.push_back("> " + msg);
        if (engineLogs.size() > 30) engineLogs.pop_front(); 
    }

    void submitConsole() {
        if (consoleInput.empty()) return;
        addLog("CMD: " + consoleInput);
        processCommand(consoleInput, 0);
        consoleInput = "";
    }

    bool load(std::string path) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        
        currentFile = path;
        isLS = (path.find(".ls") != std::string::npos);

        boxes.clear(); physicsData.clear(); isUI.clear(); persistentObjects.clear(); textures.clear(); activeSounds.clear();
        scriptLines.clear(); pc = 0; loopStartPC = -1;
        disabledInputs.clear(); 
        delayUntil = 0.f; 
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line.substr(0, 2) == "//") continue;
            scriptLines.push_back(line);
        }
        scriptLoaded = true;
        addLog("Loaded file: " + path);
        return true;
    }

    bool evaluateCondition(std::string cond) {
        if (consoleOpen) return false; 

        std::string s = clean(cond);
        if (s.find("colliding") != std::string::npos) {
            size_t start = s.find('(') + 1; size_t mid = s.find(")colliding("); size_t end = s.rfind(')');
            if (mid != std::string::npos) {
                std::string a = s.substr(start, mid - start);
                std::string b = s.substr(mid + 11, end - (mid + 11));
                if (boxes.count(a) && boxes.count(b))
                    return boxes[a].getGlobalBounds().findIntersection(boxes[b].getGlobalBounds()).has_value();
            }
        }
        
        if (s.find("=1") != std::string::npos) {
            for (auto const& [name, code] : keyMap) {
                if (s.find(name) != std::string::npos) {
                    if (disabledInputs.count(name)) {
                        if (engineClock.getElapsedTime().asSeconds() < disabledInputs[name]) {
                            return false; 
                        } else {
                            disabledInputs.erase(name);
                            addLog("Input lock expired for: " + name);
                        }
                    } 
                    return sf::Keyboard::isKeyPressed(code);
                }
            }
        }
        return false;
    }

    void processCommand(std::string line, int lineNum) {
        std::string original = line;
        
        original.erase(0, original.find_first_not_of(" \t\r\n"));
        if (original.empty()) return;
        original.erase(original.find_last_not_of(" \t\r\n") + 1);

        std::string lowerOrig = original;
        std::transform(lowerOrig.begin(), lowerOrig.end(), lowerOrig.begin(), ::tolower);
        
        if (lowerOrig.substr(0, 5) == "sound") {
            std::string filename = original.substr(5);
            filename.erase(0, filename.find_first_not_of(" \t"));
            auto music = std::make_unique<sf::Music>();
            if (music->openFromFile(filename)) { 
                music->play(); 
                activeSounds.push_back(std::move(music)); 
                addLog("Sound " + filename + " played");
            }
            return;
        }

        if (lowerOrig.substr(0, 5) == "delay") {
            std::string timeStr = original.substr(5);
            timeStr.erase(0, timeStr.find_first_not_of(" \t"));
            if (!timeStr.empty()) {
                float secondsToWait = 0.f;
                try { secondsToWait = std::stof(timeStr); } catch (...) {}
                delayUntil = engineClock.getElapsedTime().asSeconds() + secondsToWait;
                addLog("Script delayed for " + timeStr + "s");
            }
            return;
        }

        if (original.find('(') == std::string::npos) return;
        
        std::string cmd = clean(original.substr(0, original.find('(')));
        std::string target = clean(original.substr(original.find('(') + 1, original.find(')') - original.find('(') - 1));
        std::string params = original.substr(original.find(')') + 1);
        
        if (!params.empty()) {
            params.erase(0, params.find_first_not_of(" \t"));
            params.erase(params.find_last_not_of(" \t\r\n") + 1);
        }

        // --- FIXED: FORCE STUN OVERWRITE ---
        if (cmd == "disableinputtime") { 
            float lockTime = 0.f;
            try { lockTime = std::stof(params); } catch (...) {}
            disabledInputs[target] = engineClock.getElapsedTime().asSeconds() + lockTime; 
            addLog("Froze input " + target + " for " + params + "s");
            return; 
        }
        
        if (cmd == "disableinput") { disabledInputs[target] = 999999999.f; addLog("Permanently froze input " + target); return; }
        if (cmd == "enableinput") { disabledInputs.erase(target); addLog("Unfroze input " + target); return; }

        if (cmd == "freeze") {
            if (physicsData.count(target)) {
                physicsData[target].velocity = {0.f, 0.f};
                addLog("Momentum frozen for " + target);
            }
            return;
        }

        if (cmd == "persistent") { persistentObjects.insert(target); return; }
        if (cmd == "destroy") { boxes.erase(target); physicsData.erase(target); isUI.erase(target); persistentObjects.erase(target); addLog("Destroyed " + target); return; }
        if (cmd == "loadlevel") { executeLevelScript(target + ".ls"); return; }
        
        if (cmd == "texture" && boxes.count(target)) {
            params.erase(std::remove(params.begin(), params.end(), ' '), params.end());
            if (textures.find(params) == textures.end()) {
                auto tex = std::make_unique<sf::Texture>();
                if (tex->loadFromFile(params)) textures[params] = std::move(tex);
            }
            if (textures.count(params)) { boxes[target].setTexture(textures[params].get()); boxes[target].setFillColor(sf::Color::White); }
            return;
        }

        if (cmd == "physics" || cmd == "collide") {
            if (isLS && !consoleOpen) triggerError("Error in " + currentFile + "! Logic Lockdown: Cannot put physics in .ls files!");
            if (isUI[target]) triggerError("Sebus Error: UI elements cannot have physics!");
            if (boxes.count(target)) {
                if (cmd == "physics") physicsData[target].enabled = true;
                if (cmd == "collide") physicsData[target].isSolid = true;
            }
        }
        else if (cmd == "createbox" || cmd == "createui") {
            if (boxes.count(target)) return; 
            boxes[target] = sf::RectangleShape({ 50.f, 50.f });
            boxes[target].setOrigin({ 25.f, 25.f });
            isUI[target] = (cmd == "createui");
            addLog("Created box named " + target);
        }
        else if (cmd == "pos" && boxes.count(target)) {
            std::stringstream ss(params); float x, y;
            if (ss >> x >> y) boxes[target].setPosition({ 400.f + x, 300.f - y });
        }
        else if (cmd == "size" && boxes.count(target)) {
            std::stringstream ss(params); float w, h;
            if (ss >> w >> h) { boxes[target].setSize({ w, h }); boxes[target].setOrigin({ w / 2.f, h / 2.f }); }
        }
        else if (cmd == "color" && boxes.count(target)) {
            std::stringstream ss(params); int r, g, b;
            if (ss >> r >> g >> b) boxes[target].setFillColor(sf::Color(r, g, b));
        }
        else if (cmd.find("physics.velocity") != std::string::npos) {
            std::string valStr = params;
            if (valStr.find('=') != std::string::npos) valStr = valStr.substr(valStr.find('=') + 1);
            float val = std::stof(valStr);
            if (cmd.find("velocityx") != std::string::npos) physicsData[target].velocity.x = val;
            if (cmd.find("velocityy") != std::string::npos) physicsData[target].velocity.y = val;
        }
    }

    void runLogicBlock(std::string content) {
        std::string lowerContent = content;
        std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
        size_t thenPos = lowerContent.find("then");
        if (thenPos == std::string::npos) return;

        std::string condition = content.substr(0, thenPos);
        std::string actions = content.substr(thenPos + 4);

        if (evaluateCondition(condition)) {
            std::string formattedActions = actions;
            
            std::vector<std::string> cmds = {
                "physics.", "sound ", "delay ", "freeze",
                "disableinputtime", "disableinput", "enableinput", 
                "destroy", "texture", "color", "size", "pos", 
                "persistent", "loadlevel", "createbox", "createui", "collide"
            };
            
            for (const auto& c : cmds) {
                size_t pos = 0;
                std::string lowerActions = formattedActions;
                std::transform(lowerActions.begin(), lowerActions.end(), lowerActions.begin(), ::tolower);
                while ((pos = lowerActions.find(c, pos)) != std::string::npos) {
                    if (pos > 0 && formattedActions[pos-1] != '\n') {
                        formattedActions.insert(pos, "\n");
                        lowerActions.insert(pos, "\n"); 
                        pos++;
                    }
                    pos += c.length();
                }
            }
            
            std::stringstream ss(formattedActions);
            std::string cmdStr;
            while (std::getline(ss, cmdStr, '\n')) {
                if (!clean(cmdStr).empty()) processCommand(cmdStr, 0);
            }
        }
    }

    void update() {
        activeSounds.remove_if([](const std::unique_ptr<sf::Music>& m) {
            return m->getStatus() == sf::SoundSource::Status::Stopped;
        });

        for (auto& [name, phys] : physicsData) {
            if (!phys.enabled) continue;
            phys.velocity.y += phys.gravity; phys.velocity.x *= phys.friction;
            
            boxes[name].move({ phys.velocity.x, 0.f });
            for (auto& [oN, oB] : boxes) {
                if (name == oN || !physicsData[oN].isSolid) continue;
                if (boxes[name].getGlobalBounds().findIntersection(oB.getGlobalBounds())) {
                    boxes[name].move({ -phys.velocity.x, 0.f }); phys.velocity.x = 0;
                }
            }
            
            boxes[name].move({ 0.f, phys.velocity.y });
            for (auto& [oN, oB] : boxes) {
                if (name == oN || !physicsData[oN].isSolid) continue;
                if (boxes[name].getGlobalBounds().findIntersection(oB.getGlobalBounds())) {
                    boxes[name].move({ 0.f, -phys.velocity.y }); phys.velocity.y = 0;
                }
            }
        }
    }

    void runFrame(sf::RenderWindow& window) {
        if (!scriptLoaded) return;

        if (engineClock.getElapsedTime().asSeconds() < delayUntil) {
            update();
            return; 
        }

        while (pc < (int)scriptLines.size() && loopStartPC == -1) {
            if (clean(scriptLines[pc]).find("loop(infinite){") != std::string::npos) { loopStartPC = pc + 1; break; }
            processCommand(scriptLines[pc], pc + 1); pc++;
            
            if (engineClock.getElapsedTime().asSeconds() < delayUntil) {
                update();
                return;
            }
        }

        if (loopStartPC != -1) {
            for (int i = loopStartPC; i < (int)scriptLines.size(); ++i) {
                std::string line = clean(scriptLines[i]);
                if (line == "}") break;

                if (line.find("if{") != std::string::npos || line.substr(0, 3) == "if ") {
                    if (i + 4 < scriptLines.size() && clean(scriptLines[i+2]) == "}then{") {
                        if (evaluateCondition(scriptLines[i + 1])) processCommand(scriptLines[i + 3], i + 4);
                        i += 4; 
                        continue;
                    }
                    size_t firstBrace = scriptLines[i].find('{');
                    if (firstBrace != std::string::npos) {
                        std::string blockContent = scriptLines[i].substr(firstBrace + 1);
                        int j = i;
                        size_t lastBrace = blockContent.find('}');
                        
                        if (lastBrace == std::string::npos) {
                            j++;
                            while (j < scriptLines.size()) {
                                std::string nextLine = scriptLines[j];
                                size_t closePos = nextLine.find('}');
                                if (closePos != std::string::npos) {
                                    blockContent += "\n" + nextLine.substr(0, closePos);
                                    break;
                                } else {
                                    blockContent += "\n" + nextLine;
                                }
                                j++;
                            }
                        } else {
                            blockContent = blockContent.substr(0, lastBrace); 
                        }
                        
                        runLogicBlock(blockContent);
                        i = j; 
                        continue;
                    }
                } else {
                    processCommand(scriptLines[i], i + 1);
                }
            }
        }
        update();
    }

    void render(sf::RenderWindow& window) {
        window.clear(bgColor);
        for (auto const& [name, shape] : boxes) window.draw(shape);
        
        if (consoleOpen) {
            sf::RectangleShape bg({300.f, 600.f});
            bg.setPosition({500.f, 0.f});
            bg.setFillColor(sf::Color(10, 10, 20, 240)); 
            window.draw(bg);
            
            sf::RectangleShape line({2.f, 600.f});
            line.setPosition({498.f, 0.f});
            line.setFillColor(sf::Color::Cyan);
            window.draw(line);
            
            if (fontLoaded) {
                sf::Text text(font);
                text.setCharacterSize(13);
                text.setFillColor(sf::Color(180, 255, 180)); 
                
                float y = 10.f;
                for (const auto& logStr : engineLogs) {
                    text.setString(logStr);
                    text.setPosition({510.f, y});
                    window.draw(text);
                    y += 18.f;
                }
                
                text.setString("Type Command:\n> " + consoleInput + "_");
                text.setPosition({510.f, 550.f});
                text.setFillColor(sf::Color::Yellow);
                window.draw(text);
            }
        }
        window.display();
    }
};

int main(int argc, char* argv[]) {
    SebusEngine engine;
    std::string path = (argc > 1) ? argv[1] : "main.gs";
    if (!engine.load(path)) { MessageBoxA(NULL, "Drag a .gs file onto Sebus!", "Fatal Error", MB_ICONERROR | MB_OK); return 1; }
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "Sebus Engine Pro");
    window.setFramerateLimit(60);
    
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) { 
            if (event->is<sf::Event::Closed>()) {
                window.close(); 
            }
            else if (const auto* mouseEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseEvent->button == sf::Mouse::Button::Left) engine.addLog("Mouse Left clicked");
                else if (mouseEvent->button == sf::Mouse::Button::Right) engine.addLog("Mouse Right clicked");
            }
            else if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                // --- FIXED: NOW MAPPED EXACTLY TO THE SLASH KEY ---
                if (keyEvent->code == sf::Keyboard::Key::Slash) {
                    engine.consoleOpen = !engine.consoleOpen; 
                }
                else if (engine.consoleOpen) {
                    if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        engine.submitConsole();
                    } else if (keyEvent->code == sf::Keyboard::Key::Backspace) {
                        if (!engine.consoleInput.empty()) engine.consoleInput.pop_back();
                    }
                }
                else {
                    for (const auto& [name, code] : engine.keyMap) {
                        if (code == keyEvent->code) {
                            engine.addLog(name + " clicked");
                            break;
                        }
                    }
                }
            }
            else if (const auto* textEvent = event->getIf<sf::Event::TextEntered>()) {
                if (engine.consoleOpen) {
                    char32_t c = textEvent->unicode;
                    // Ignore the slash key so it doesn't type into the console when you toggle it
                    if (c >= 32 && c != 127 && c != '/') {
                        engine.consoleInput += static_cast<char>(c);
                    }
                }
            }
        }
        
        engine.runFrame(window); 
        engine.render(window);
    }
    return 0;
}