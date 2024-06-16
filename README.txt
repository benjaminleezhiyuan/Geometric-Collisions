a.) When launching the application, you can use the imgui window to select the tests and change the values of the objects.
    There is a seperate imgui window that shows a top down view of the scene by default, you can change the position of this
    camera by using the sliders.
    When doing a test with only 1 sphere or aabb box, to change the values you need to change sphere1 and aabb1 respectively.
    When an object turns red, that means that intersection has occured. 
    For rays, there will also be a point of intersection shown on the object as a green point.

b.) Interacting with the imgui window is all you need to do to test the interactions.

c.) All parts of the assignment have been completed, all 14 tests and the dynamic PiP view.

d.) Not applicable.

e.) All files for the program can be found inside the Graphics folder.
	-main.cpp is where the application is setup and main render loop is running.
	-classes.h is where the struct for the different geometry data is.
	-helper.h is where all the intersection tests are declared.
		-the function names are all named Geometry1VsGeometry2 where Geometry is replaced with
		 the respective geometry that is to be tested.
		-the checkIntersection function is overloaded to take in 2 different geometry types and
		 returns true/false.
	-helper.cpp is where all the intersection tests are defined.

f.) OS - Windows 10 Home 10.0.19045 Build 19045
    GPU - RTX 3060 (Laptop)
    OpenGL - Core GL Version - 4.6
	   - Shading Language Version - 4.60
	   - Compatibility GL ES Version - 3.2

g.) 4 hours a week

h.) Not applicable.