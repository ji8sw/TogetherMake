
# Together Make
is a multiplayer 3D modeller using OpenGL for graphics, ENet for networking, and Dear ImGui for user interface.

# Graphics
the graphics are using OpenGL as it is simple yet effective, a more complicated, more efficient API, is not needed for this project as it uses a very basic scene, which usually contains 1 object.

# Networking
while the method that players use to communicate is not yet decided (P2P or dedicated), it currently has basic client to server connections but players are not able to play with each other just yet.
the networking is based on Samurai, which is a networking solution I made to handle packet data from ENet.

# Interface
the interface is using Dear ImGui, it is currently very simple, the main attraction is that users can select vertices on the object, which will later be used for modelling capabilties

# Controls
Hold middle mouse (scrollwheel) and move your mouse to rotate the camera around the origin
Click on corners of the cube to select vertices
Press F1-F3 to change rendering modes
While a vertex is selected, press G to enter Move mode
Mode mode: move the mouse to change the position of the vertex
All modes: press enter to confirm your changes, or escape to cancel changes

# Building

To build simply read `ReadMe-Setup.md` [here](https://github.com/ji8sw/TogetherMake/blob/master/ReadMe-Setup.md)
Then compile and run using Visual Studio 2022
You may need `glew32.dll` and `glfw3.dll`, you can find these in the `\bin` folder next to where you found the `.lib` files.
There is more info in `ReadMe-Setup.md`.

# To-Do
This project has a lot of to-do's as it was literally just created.

- Improve controls: stop camera jumping to center
- Safety (client): ensure vertices exist before accessing with them
- Safety (server): ensure peers are registered players when recieving packets
- Safety (server): password requirements
