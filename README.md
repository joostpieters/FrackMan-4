FrackMan, a clone of the 80’s classic Dig Dug arcade game.
---------------------------------------------------------------------------
This project was specced by Carey Nachenberg for UCLA's CS32 course in 2016. It is a practice in proper object oriented design principles, inheritence, polymorphism, and efficient breadth-first pathing algorithms for the in-game characters.

A lot of the backend and reference files were already provided by Carey Nachenberg (and freeglut was written by Pawel W. Olszta). I built the following files from the ground up:
- StudentWorld.h
- StudentWorld.cpp
- Actor.h
- Actor.cpp

The StudentWorld class is a derived class of the GameWorld class. It does all the dirty work of keeping track of all the "actors" in the game (ie. player, characters, items) and how they interact with each other. It also includes the breadth-first pathing algorithms for the characters that require pathing to certain locations (eg. hunting for the player, or searching for the exit).

The Actor class is a derived class of the GraphObject class. It acts as the base class of all in-game objects. A visual representation of the class hierarchy can be found on Actor.h.

Copyright (c) 2016 Yen Chen
