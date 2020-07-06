/*==================================================================================
* COSC 363  Computer Graphics (2020)
* Department of Computer Science and Software Engineering, University of Canterbury.
*
* A basic ray tracer
* See Lab07.pdf, Lab08.pdf for details.
*===================================================================================
*/
#define _USE_MATH_DEFINES

#include <iostream>
#include <cmath>
#include <math.h>
#include <vector>
#include <glm/glm.hpp>
#include "Sphere.h"
#include "Cylinder.h"
#include "SceneObject.h"
#include "Ray.h"
#include <GL/freeglut.h>
#include "Plane.h"
#include "TextureBMP.h"
using namespace std;

const float WIDTH = 20.0;  
const float HEIGHT = 20.0;
const float EDIST = 40.0;
const int NUMDIV = 500;
const int MAX_STEPS = 5;
const float XMIN = -WIDTH * 0.5;
const float XMAX =  WIDTH * 0.5;
const float YMIN = -HEIGHT * 0.5;
const float YMAX =  HEIGHT * 0.5;

TextureBMP textureFootball;
TextureBMP textureTable;
vector<SceneObject*> sceneObjects;


//---The most important function in a ray tracer! ---------------------------------- 
//   Computes the colour value obtained by tracing a ray and finding its 
//   closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
	glm::vec3 backgroundCol(0);						
	glm::vec3 lightPos(20, 40, 20);					
	glm::vec3 lightPos2(0, 60, -65.0);
	glm::vec3 color(0);
	SceneObject* obj;
	float ambientColor = 0.4;
	float transparentAmbientColor = 0.8;
	float alpha = 0.1;
	glm::vec3 spotDir(0, -1, 0);

	ray.closestPt(sceneObjects);					
	if (ray.index == -1) return backgroundCol;		
	obj = sceneObjects[ray.index];					

	if (ray.index == 0) //Floor
	{				   
		//Chequered Pattern
		int stripeWidth = 5;
		int iz = (ray.hit.z) / stripeWidth;
		int ix = (ray.hit.x) / stripeWidth;
		int k = iz % 2; //2 colors
		int l = ix % 2; //2 colors
		if (ray.hit.x <= 0) {
			if (k == l) color = glm::vec3(0, 1, 0);
			else color = glm::vec3(1, 1, 0.5);
		}
		else {
			if ((k != 0 && l == 0) || (l != 0 && k == 0)) color = glm::vec3(0, 1, 0);
			else color = glm::vec3(1, 1, 0.5);
		}
		obj->setColor(color);			
	}

	if (ray.index == 1) //Football
	{	
		glm::vec3 shereOrigin = glm::vec3(0.0, -0.5, -65.0);
		glm::vec3 d = normalize(shereOrigin - ray.hit);

		float texcoords = 0.5 + (atan2(d.x, d.z)) / (2*M_PI);
		float texcoordt = 0.5 - (asin(d.y) / (M_PI));
		if (texcoords > 0 && texcoords < 1 &&
			texcoordt > 0 && texcoordt < 1)
		{
			color = textureFootball.getColorAt(texcoords, texcoordt);
			obj->setColor(color);
		}
	}

	if (ray.index == 2) //Table
	{				    
		float texcoords = (ray.hit.x + 15) / 30;
		float texcoordt = (ray.hit.z + 50) / -30;
		if (texcoords > 0 && texcoords < 1 &&
			texcoordt > 0 && texcoordt < 1)
		{
			color = textureTable.getColorAt(texcoords, texcoordt);
			obj->setColor(color);
		}

	}

	if (ray.index == 5) //Pattern Panel
	{	
		//Sin pattern
		if ((ray.hit.y + 2.5) > sin((ray.hit.x + 13))) {
			color = glm::vec3(0.5, 0, 0);
		}
		else {
			color = glm::vec3(0, 0, 0.5);
		}
		obj->setColor(color);

	}

	color = obj->lighting(lightPos, -ray.dir, ray.hit);		
	color = color + obj->lighting(lightPos2, -ray.dir, ray.hit);


	glm::vec3 lightVec = lightPos - ray.hit;   //To add shadows
	Ray shadowRay(ray.hit, lightVec);
	glm::vec3 lightVec2 = lightPos2 - ray.hit;   //To add shadows
	Ray shadowRay2(ray.hit, lightVec2);

	float lightDist = glm::length(lightVec);
	float lightDist2 = glm::length(lightVec2);

	shadowRay.closestPt(sceneObjects); //Find the closest point of intersection of the shadow ray
	shadowRay2.closestPt(sceneObjects);

	//Shadow for a transparent obj
	if ((shadowRay.index == 3 || shadowRay.index == 4) && shadowRay.dist < lightDist) {
		color = transparentAmbientColor * obj->getColor(); //0.7 = ambient scale factor
	} else if (shadowRay.index > -1 && shadowRay.dist < lightDist) { 
		color = ambientColor * obj->getColor(); //0.2 = ambient scale factor
	}

	//Shadows for spotlight
	double dot = glm::dot(-normalize(shadowRay2.dir), normalize(spotDir));
	double shadowSpotAngle = acos(dot);
	if ((shadowRay2.index == 3 || shadowRay2.index == 4) && shadowRay2.dist < lightDist2) {
		color = color * transparentAmbientColor;
	}
	else if (shadowRay2.index > -1 && shadowRay2.dist < lightDist2) {
		color = color * ambientColor;
	}
	else if (shadowRay2.index <= -1 && shadowSpotAngle > alpha) {
		color = color * ambientColor;

	}

	//Generate transparency
	if (obj->isTransparent() && step < MAX_STEPS)
	{
		float transparencyCoeff = obj->getTransparencyCoeff();
		color = color * (1 - transparencyCoeff);
	}

	//Generate refractions
	if (obj->isRefractive() && step < MAX_STEPS)
	{
		float tho = obj->getRefractionCoeff();
		float refractiveIndex = obj->getRefractiveIndex();
		float eta = 1 / refractiveIndex;
		glm::vec3 normalVec = obj->normal(ray.hit);
		glm::vec3 firstRefract = glm::refract(ray.dir, normalVec, eta);
		Ray firstRefractedRay(ray.hit, firstRefract);
		firstRefractedRay.closestPt(sceneObjects);
		glm::vec3 secondNormal = obj->normal(firstRefractedRay.hit);
		glm::vec3 secondRefract = glm::refract(firstRefract, -secondNormal, 1.0f / eta);
		Ray secondRefractedRay(firstRefractedRay.hit, secondRefract);
		glm::vec3 refractedColor = trace(secondRefractedRay, step + 1);
		color = color + (tho * refractedColor);
	}

	//Generate reflections
	if (obj->isReflective() && step < MAX_STEPS)
	{
		float rho = obj->getReflectionCoeff();
		glm::vec3 normalVec = obj->normal(ray.hit);
		glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
		Ray reflectedRay(ray.hit, reflectedDir);
		glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
		color = color + (rho * reflectedColor);
	}

	

	return color;
}

//---The main display module -----------------------------------------------------------
// In a ray tracing application, it just displays the ray traced image by drawing
// each cell as a quad.
//---------------------------------------------------------------------------------------
void display()
{
	float xp, yp;  //grid point
	float cellX = (XMAX - XMIN) / NUMDIV;  //cell width
	float cellY = (YMAX - YMIN) / NUMDIV;  //cell height
	glm::vec3 eye(0., 0., 0.);
	float squareMatrix[4 * 2] = {
		1.0 / 4.0, 1.0 / 4.0,
		3.0 / 4.0, 1.0 / 4.0,
		3.0 / 4.0, 3.0 / 4.0,
		1.0 / 4.0, 3.0 / 4.0
	};


	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBegin(GL_QUADS);  //Each cell is a tiny quad.

	for (int i = 0; i < NUMDIV; i++)	//Scan every cell of the image plane
	{
		xp = XMIN + i * cellX;
		for (int j = 0; j < NUMDIV; j++)
		{
			yp = YMIN + j * cellY;

			glm::vec3 dir;	//direction of the primary ray

			Ray ray;

			glm::vec3 col; //Trace the primary ray and get the colour value

			for (int sample = 0; sample < 4; sample++) { //Anti-aliassing
				dir = glm::vec3(xp + (squareMatrix[2 * sample]) * cellX, yp + (squareMatrix[2 * sample + 1]) * cellY, -EDIST);
				ray = Ray(eye, dir);
				col += trace(ray, 1);
			}

			col.r = col.r / 4;
			col.g = col.g / 4;
			col.b = col.b / 4;

			glColor3f(col.r, col.g, col.b);
			glVertex2f(xp, yp);				//Draw each cell with its color value
			glVertex2f(xp + cellX, yp);
			glVertex2f(xp + cellX, yp + cellY);
			glVertex2f(xp, yp + cellY);
		}
	}

	glEnd();
	glFlush();
}



//---This function initializes the scene ------------------------------------------- 
//   Specifically, it creates scene objects (spheres, planes, cones, cylinders etc)
//     and add them to the list of scene objects.
//   It also initializes the OpenGL orthographc projection matrix for drawing the
//     the ray traced image.
//----------------------------------------------------------------------------------
void initialize()
{
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);
	textureFootball = TextureBMP("Football.bmp");
	textureTable = TextureBMP("Table.bmp");

    glClearColor(0, 0, 0, 1);

	//Floor
	Plane* floor = new Plane(glm::vec3(-80., -15, -40), //Point A
		glm::vec3(80., -15, -40), //Point B
		glm::vec3(80., -15, -200), //Point C
		glm::vec3(-80., -15, -200)); //Point D
	sceneObjects.push_back(floor); //Add this to the list of sceneObjects:

	//Football
	Sphere* football = new Sphere(glm::vec3(0.0, -0.5, -65.0), 3.0);
	football->setColor(glm::vec3(0, 1, 0));
	sceneObjects.push_back(football);		 //Add sphere to scene objects	

	//Table
	Plane* table = new Plane(glm::vec3(-15., -7, -50), //Point A
		glm::vec3(15., -7, -50), //Point B
		glm::vec3(15., -7, -80), //Point C
		glm::vec3(-15., -7, -80)); //Point D
	table->setColor(glm::vec3(1, 1, 1));
	sceneObjects.push_back(table); //Add this to the list of sceneObjects:

	// Left Glass Ball
	Sphere* transparentSphereLeft = new Sphere(glm::vec3(-10.0, -4.0, -65.0), 3.0);
	transparentSphereLeft->setColor(glm::vec3(0, 0, 1));  
	transparentSphereLeft->setTransparency(true, 1);
	transparentSphereLeft->setRefractivity(true, 1, 1.01);
	sceneObjects.push_back(transparentSphereLeft);

	// Right Glass Ball
	Sphere* transparentSphereRight = new Sphere(glm::vec3(10.0, -4.0, -65.0), 3.0);
	transparentSphereRight->setColor(glm::vec3(1, 0, 0));  
	transparentSphereRight->setTransparency(true, 1);
	transparentSphereRight->setRefractivity(true, 1, 1.01);
	sceneObjects.push_back(transparentSphereRight);

	//Pattern Panel
	Plane* patternPanel = new Plane(glm::vec3(-13., -4, -68), //Point A
		glm::vec3(13., -4, -68), //Point B
		glm::vec3(13, -1, -68), //Point C
		glm::vec3(-13, -1, -68)); //Point D
	sceneObjects.push_back(patternPanel); //Add this to the list of sceneObjects:

	//Pyramid Front
	Plane* pyramidFront = new Plane(glm::vec3(-1., -7, -61), //Point A
		glm::vec3(4., -7, -64), //Point B
		glm::vec3(0, -3.5, -65)); //Point C
	pyramidFront->setColor(glm::vec3(1, 0.7, 0));
	sceneObjects.push_back(pyramidFront); //Add this to the list of sceneObjects:

	//Pyramid Right
	Plane* pyramidRight = new Plane(glm::vec3(4., -7, -64), //Point A
		glm::vec3(1., -7, -69), //Point B
		glm::vec3(0, -3.5, -65)); //Point C
	pyramidRight->setColor(glm::vec3(1, 0.7, 0));
	sceneObjects.push_back(pyramidRight); //Add this to the list of sceneObjects:

	//Pyramid Back
	Plane* pyramidBack = new Plane(glm::vec3(1., -7, -69), //Point A
		glm::vec3(-4., -7, -66), //Point B
		glm::vec3(0, -3.5, -65)); //Point C
	pyramidBack->setColor(glm::vec3(1, 0.7, 0));
	sceneObjects.push_back(pyramidBack); //Add this to the list of sceneObjects:

	//Pyramid Left
	Plane* pyramidLeft = new Plane(glm::vec3(-4., -7, -66), //Point A
		glm::vec3(-1., -7, -61), //Point B
		glm::vec3(0, -3.5, -65)); //Point C
	pyramidLeft->setColor(glm::vec3(1, 0.7, 0));
	sceneObjects.push_back(pyramidLeft); //Add this to the list of sceneObjects:

	//Cylinder Back Left
	Cylinder* cylinderBackLeft = new Cylinder(glm::vec3(-14.0, -7.0, -68.0), 1.0, 6.0);
	cylinderBackLeft->setColor(glm::vec3(0, 0, 1));  
	sceneObjects.push_back(cylinderBackLeft);		 

	//Cylinder Back Right
	Cylinder* cylinderBackRight = new Cylinder(glm::vec3(14.0, -7.0, -68.0), 1.0, 6.0);
	cylinderBackRight->setColor(glm::vec3(1, 0, 0));
	sceneObjects.push_back(cylinderBackRight);		
	
	//Bottom Ball
	Sphere* bottomBall = new Sphere(glm::vec3(0, -11.0, -65.0), 4.0); 
	bottomBall->setColor(glm::vec3(0, 0.5, 0));  
	sceneObjects.push_back(bottomBall);
}


int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracing");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
