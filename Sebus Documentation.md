# Sebus Documentation

## Welcome to the Sebus Documentation.

##### This will explain how to use base GameScript and write your first Sebus Game.



1. ### Basics



Sebus is like a game developer's dream. It's really simple to use, but also complicated and looks cool at the same time.

In this section, we will learn about Create, Pos, and loops.



What you need and what to do.

1. Download Sebus
2. Get your program editor of your chose. (eg. Visual Studio or Visual Studio Code)
3. Create a file named "Game.gs"



Now that you are in there, type these lines:
```
(start)
Create box()
```
Create- Creates whatever. box- Defines that you are making a box. ()- Defines what the compiler/system calls the box. (start)- The header, like "start:" in assembly or C#.

Whatever you type in those brackets are very important. I'll go with box(Test)

Next- type Pos(). You see? Now you type what you put in the brackets in the create command so Sebus Compiler knows that the box is its own thing.
```
Pos(Test)
```
Now type "0 0" after the pos command. This is the position that the box will be at. Now save the .gs file, and drag it into your sebus program. If you see a square in the middle, congrats, you made your first GameScript. This is what your .gs should look like:
```
(start)
Create box(Test)

pos(test) 0 0
```
