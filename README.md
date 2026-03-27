# Welcome to Sebus!
## Sebus is a lightweight 2D development environment built on SFML 3. It utilizes a custom interpreted language, SebScript, designed for rapid prototyping through a human-readable, label-based syntax. The engine handles low-level tasks like physics, resolution scaling, and input mapping, allowing developers to focus entirely on game logic.

Discord for the Developers of Sebus:
<iframe src="https://discord.com/widget?id=1453911643506344029&theme=dark" width="350" height="500" allowtransparency="true" frameborder="0" sandbox="allow-popups allow-popups-to-escape-sandbox allow-same-origin allow-scripts"></iframe>

Key Features
SebScript Interpreter: Support for .gs (GameScript) and .ls (LevelScript) files.

Label-Based Flow: Efficient state management using labels and Goto commands.

Modular Architecture: Include headers to manage large projects across multiple script files.

Built-in Physics: Integrated AABB collision detection, gravity, and friction.

Dynamic Letterboxing: High-fidelity rendering that maintains aspect ratio across all window sizes and fullscreen modes.

Input Mapping: Native support for full A-Z keyboard, Arrow keys, and coordinate-aware mouse clicking.

SebScript Example
The following script demonstrates the basic structure of a Sebus project:
```
Include Sebus.Levels

(Start)
window.title("Sebus Project")
window.color(20 20 20)
create box(player)
size(player) 40 40
pos(player) 0 0
physics(player)
goto(Logic)

(Logic)
loop(infinite) {
    if {
        Keyboard.Space = 1
    Then
        Physics.VelocityY(player) = -15
    }
}
