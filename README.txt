a.) Launch program using Release Mode. To load models, put them into the Assets folder and use scale of 0.0001f. (See Line 999 in main)
    Move around using WASD and Right Click drag to move the camera.
    I have included powerplant4-powerplant6 in the submission.
    You are able to pick topdown or bottomup construction using the imgui window.
    For Top Down, you have the choice to restrict height to 7 or let it construct until the leaf.
    For both topdown and bottom up, you can change the bounding volume type.
    You can choose to display all the levels of the Tree or you can use the slider to draw specific level.

b.) Interacting with the imgui window is all you need to do to test the program. Run in release.

c.) All parts of the assignment have been completed except extra credit.

d.) Not applicable.

e.) All files for the program can be found inside the Graphics folder.
	-main.cpp is where the application is setup and main render loop is running..

f.) OS - Windows 10 Home 10.0.19045 Build 19045
    GPU - RTX 3060 (Laptop)
    OpenGL - Core GL Version - 4.6
	   - Shading Language Version - 4.60
	   - Compatibility GL ES Version - 3.2

g.) 10 hours a week

h.) Not applicable.

Choosing a heuristic for bottom-up BVH construction that combines nearest neighbor, 
minimum combined volume, and minimum relative increase in volume optimizes both 
spatial locality and bounding volume tightness. This approach enhances traversal efficiency 
by reducing unnecessary intersection tests, maintains a balanced tree structure, 
and preserves cache performance. While it may increase construction time, the overall performance 
improvement in ray tracing and collision detection justifies the trade-off.