/*
 * File:   matlab_bullet_wrapper.h
 * Author: bminortx
 * This wrapper is designed to facilitate the use of Bullet Physics
 * in MATLAB. Graphics are handled by MATLAB, as well as parameterization of
 * objects and environmental variables. Bullet takes care of all calculations
 * and returns those results back to MATLAB.
 */

#pragma once

// Suppress Bullet warnings in GCC and Clang
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic pop

#include "Compound.h"
#include "../Graphics/graphicsWorld.h"
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>

///////////////////////////////////////////////////////
/// The BulletWorld class
/// The Simulator class holds all of the methods that can be called from
/// bullet_interface_mex.cpp. Any method called from there MUST be in this
/// class.

static std::vector<std::unique_ptr<Compound> > compounds_;
static std::vector<std::unique_ptr<bullet_shape> > shapes_;
static std::vector<std::unique_ptr<bullet_vehicle> > vehicles_;
static std::vector<btTypedConstraint*> constraints_;

/// OPENGL STUFF
static int window;
static std::vector<unsigned int> buffers_;
static int view_angle_ = 0;
static int view_elevation_ = 0;
static float fov_ = 55;
static float aspect_ratio_ = 1;
static float world_dim_ = 7.0;
static int light_move_ = 1;
static int light_angle_ = 90;
static float light_elevation_ = 2;
static int shader_program_;
const float CrystalDensity=5.0;
const float CrystalSize=.15;

static unsigned int buffer;

// //
// //  Cube Vertexes
// //
// static void Cube(void)
// {
//   //  Front
//   glColor3f(1,0,0);
//   glBegin(GL_QUADS);
//   glNormal3f( 0, 0,+1);
//   glTexCoord2f(0,0); glVertex3f(-1,-1,+1);
//   glTexCoord2f(1,0); glVertex3f(+1,-1,+1);
//   glTexCoord2f(1,1); glVertex3f(+1,+1,+1);
//   glTexCoord2f(0,1); glVertex3f(-1,+1,+1);
//   glEnd();
//   //  Back
//   glColor3f(0,0,1);
//   glBegin(GL_QUADS);
//   glNormal3f( 0, 0,-1);
//   glTexCoord2f(0,0); glVertex3f(+1,-1,-1);
//   glTexCoord2f(1,0); glVertex3f(-1,-1,-1);
//   glTexCoord2f(1,1); glVertex3f(-1,+1,-1);
//   glTexCoord2f(0,1); glVertex3f(+1,+1,-1);
//   glEnd();
//   //  Right
//   glColor3f(1,1,0);
//   glBegin(GL_QUADS);
//   glNormal3f(+1, 0, 0);
//   glTexCoord2f(0,0); glVertex3f(+1,-1,+1);
//   glTexCoord2f(1,0); glVertex3f(+1,-1,-1);
//   glTexCoord2f(1,1); glVertex3f(+1,+1,-1);
//   glTexCoord2f(0,1); glVertex3f(+1,+1,+1);
//   glEnd();
//   //  Left
//   glColor3f(0,1,0);
//   glBegin(GL_QUADS);
//   glNormal3f(-1, 0, 0);
//   glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
//   glTexCoord2f(1,0); glVertex3f(-1,-1,+1);
//   glTexCoord2f(1,1); glVertex3f(-1,+1,+1);
//   glTexCoord2f(0,1); glVertex3f(-1,+1,-1);
//   glEnd();
//   //  Top
//   glColor3f(0,1,1);
//   glBegin(GL_QUADS);
//   glNormal3f( 0,+1, 0);
//   glTexCoord2f(0,0); glVertex3f(-1,+1,+1);
//   glTexCoord2f(1,0); glVertex3f(+1,+1,+1);
//   glTexCoord2f(1,1); glVertex3f(+1,+1,-1);
//   glTexCoord2f(0,1); glVertex3f(-1,+1,-1);
//   glEnd();
//   //  Bottom
//   glColor3f(1,0,1);
//   glBegin(GL_QUADS);
//   glNormal3f( 0,-1, 0);
//   glTexCoord2f(0,0); glVertex3f(-1,-1,-1);
//   glTexCoord2f(1,0); glVertex3f(+1,-1,-1);
//   glTexCoord2f(1,1); glVertex3f(+1,-1,+1);
//   glTexCoord2f(0,1); glVertex3f(-1,-1,+1);
//   glEnd();
// }


/// OPENGL STUFF

class BulletWorld {
 public:
  BulletWorld();
  ~BulletWorld();

  /*********************************************************************
   *ADDING OBJECTS
   **********************************************************************/
  int AddCube(double x_length, double y_length, double z_length,
                     double dMass, double dRestitution,
                     double* position, double* rotation);
  int AddSphere(double radius, double dMass, double dRestitution,
                double* position, double* rotation);
  int AddCylinder(double radius, double height, double dMass,
                  double dRestitution, double* position, double* rotation);
  int AddTerrain(int row_count, int col_count, double grad,
                 double min_ht, double max_ht,
                 double* X, double *Y, double* Z,
                 double* normal);

  int AddCompound(double* Shape_ids, double* Con_ids,
                  const char* CompoundType);

  int AddRaycastVehicle(double* parameters, double* position,
                        double* rotation);

  /*********************************************************************
   *RUNNING THE SIMULATION
   **********************************************************************/

  void StepSimulation();

  /*********************************************************************
   *COMPOUND METHODS
   **********************************************************************/

  void CommandVehicle(double id, double steering_angle, double force);

  /*********************************************************************
   *RAYCAST VEHICLE METHODS
   **********************************************************************/
  void CommandRaycastVehicle(double id, double steering_angle, double force);
  // Holds the steering, engine force, and current velocity
  double* GetRaycastMotionState(double id);

  double* RaycastToGround(double id, double x, double y);
  //  This just drops us off on the surface...
  int OnTheGround(double id);
  void SetVehicleVels(double id, double* lin_vel, double* ang_vel);
  void ResetVehicle(double id, double* start_pose, double* start_rot);

  /*********************************************************************
   *CONSTRAINT METHODS
   *All of the constructors for our constraints.
   **********************************************************************/
  int AddConstraintToWorld(btTypedConstraint* constraint);
  int PointToPoint_one(double id_A, double* pivot_in_A);
  int PointToPoint_two(double id_A, double id_B,
                       double* pivot_in_A, double* pivot_in_B);
  int Hinge_one_transform(double id_A, double* transform_A, double* limits);
  int Hinge_two_transform(double id_A, double id_B,
                          double* transform_A, double* transform_B,
                          double* limits);
  int Hinge_one_pivot(double id_A, double* pivot_in_A,
                      double* axis_in_A, double* limits);
  int Hinge_two_pivot(double id_A, double id_B,
                      double* pivot_in_A, double* pivot_in_B,
                      double* axis_in_A, double* axis_in_B,
                      double* limits);
  int Hinge2(double id_A, double id_B, double* Anchor, double* Axis_1,
             double* Axis_2, double damping, double stiffness,
             double steering_angle);
  int SixDOF_one(double id_A, double* transform_A, double* limits);

    /*********************************************************************
   *GETTERS FOR OBJECT POSES
   **********************************************************************/

  std::vector<double> GetShapeTransform(double id);
  std::vector<double> GetConstraintTransform(double id);
  std::vector< btTransform > GetVehiclePoses(bullet_vehicle& Vehicle);
  double* GetVehicleTransform(double id);

 private:

  // Physics Engine setup
  double timestep_;
  double gravity_;
  int max_sub_steps_;
  btDefaultCollisionConfiguration  collision_configuration_;
  std::unique_ptr<btCollisionDispatcher> bt_dispatcher_;
  std::unique_ptr<btDbvtBroadphase> bt_broadphase_;
  std::unique_ptr<btSequentialImpulseConstraintSolver> bt_solver_;

  // Physics and Graphics worlds
  std::shared_ptr<btDiscreteDynamicsWorld> dynamics_world_;
  std::shared_ptr<GraphicsWorld> graphics_world_;
};

/////////////////////////
// OPENGL STUFF
/////////////////////////

///////////////
////// Most of these are used to Init OpenGL

inline void gwDisplay(){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glLoadIdentity();

  // TAKEN FROM HW 4
  const double len=2.0;  //  Length of axes
  //  Light position and colors
  float Emission[]  = {0.0,0.0,0.0,1.0};
  float Ambient[]   = {0.3,0.3,0.3,1.0};
  float Diffuse[]   = {1.0,1.0,1.0,1.0};
  float Specular[]  = {1.0,1.0,1.0,1.0};
  float Shinyness[] = {16};
  float Position[]  = {(float)(2 * std::cos(light_angle_)),
                       light_elevation_,
                       (float)(2*std::sin(light_angle_)),
                       1.0};
  //  Perspective - set eye position
  float Ex = -2 * world_dim_ * std::sin(view_angle_) * std::cos(view_elevation_);
  float Ey = +2 * world_dim_ * std::sin(view_elevation_);
  float Ez = +2 * world_dim_ * std::cos(view_angle_) * std::cos(view_elevation_);
  gluLookAt(Ex,Ey,Ez , 0,0,0 , 0, std::cos(view_elevation_),0);
  glColor3f(1,1,1);
  glPushMatrix();
  glTranslated(Position[0],Position[1],Position[2]);
  glutSolidSphere(0.03,10,10);
  glPopMatrix();

  //  OpenGL should normalize normal vectors
  glEnable(GL_NORMALIZE);
  //  Enable lighting
  glEnable(GL_LIGHTING);
  //  glColor sets ambient and diffuse color materials
  glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  //  Enable light 0
  glEnable(GL_LIGHT0);
  //  Set ambient, diffuse, specular components and position of light 0
  glLightfv(GL_LIGHT0,GL_AMBIENT ,Ambient);
  glLightfv(GL_LIGHT0,GL_DIFFUSE ,Diffuse);
  glLightfv(GL_LIGHT0,GL_SPECULAR,Specular);
  glLightfv(GL_LIGHT0,GL_POSITION,Position);
  //  Set materials
  glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,Shinyness);
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,Specular);
  glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,Emission);

  // Use our Shader
  glUseProgram(shader_program_);
  int id;
  id = glGetUniformLocation(shader_program_,"CrystalDensity");
  if (id>=0) glUniform1f(id,CrystalDensity);
  id = glGetUniformLocation(shader_program_,"CrystalSize");
  if (id>=0) glUniform1f(id,CrystalSize);

  std::unique_ptr<bullet_shape>& currentShape = shapes_[1];
  btTransform world_transform =
      currentShape->rigidBodyPtr()->getCenterOfMassTransform();
  btMatrix3x3 rotation = world_transform.getBasis();
  btVector3 position = world_transform.getOrigin();
  float pose[] = {
    (float)rotation[0][0], (float)rotation[0][1], (float)rotation[0][2], 0,
    (float)rotation[1][0], (float)rotation[1][1], (float)rotation[1][2], 0,
    (float)rotation[2][0], (float)rotation[2][1], (float)rotation[2][2], 0,
    (float)position[0], (float)position[1], (float)position[2], 1
  };
  glMultMatrixf(pose);
  currentShape->getDrawData();
  std::this_thread::sleep_for (std::chrono::milliseconds(50));

  // FOR ALL OF OUR OBJECTS
  // Hand-calculate View Matrices FOR EACH OBJECT
  // Get transformation matrix from bullet
  // Set to model matrix

  /////////
  // DRAWING OUR SHAPES

  // Back to fixed pipeline
  glUseProgram(0);
  glutPostRedisplay();

  //  Display parameters
  glWindowPos2i(5,5);
  //  Render the scene and make it visible
  // glClearColor(.2, .2, 1, 1);
  // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glFlush();
  glutSwapBuffers();
  // glutPostWindowRedisplay(window);
}

inline void gwReshape(int width, int height){
  //  Ratio of the width to the height of the window
  aspect_ratio_ = (height > 0) ? (double) width / height : 1;
  glViewport(0,0, width, height);
  //  Set projection
  Project(fov_, aspect_ratio_, world_dim_);
}

inline void gwSpecial(int key, int x, int y){
  //  Right arrow key - increase angle by 5 degrees
  if (key == GLUT_KEY_RIGHT)
    view_angle_ += 1;
  //  Left arrow key - decrease angle by 5 degrees
  else if (key == GLUT_KEY_LEFT)
    view_angle_ -= 1;
  //  Up arrow key - increase elevation by 5 degrees
  else if (key == GLUT_KEY_UP)
    view_elevation_ += 1;
  //  Down arrow key - decrease elevation by 5 degrees
  else if (key == GLUT_KEY_DOWN)
    view_elevation_ -= 1;
  //  PageUp key - increase dim
  else if (key == GLUT_KEY_PAGE_DOWN)
    world_dim_ += 0.1;
  //  PageDown key - decrease dim
  else if (key == GLUT_KEY_PAGE_UP && world_dim_ > 1)
    world_dim_ -= 0.1;
  //  Keep angles to +/-360 degrees
  view_angle_ %= 360;
  view_elevation_ %= 360;
  //  Update projection
  Project(fov_, aspect_ratio_, world_dim_);
  glutPostRedisplay();
}

inline void gwKeyboard(unsigned char ch,int x,int y){
  //  Tell GLUT it is necessary to redisplay the scene
  glutPostRedisplay();
}

inline void gwIdle(){
  double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
  if (light_move_) light_angle_ = fmod(90 * t, 360.0);
  glutPostRedisplay();
}

inline void Init() {
  char *argv [1];
  int argc = 1;
  argv[0] = strdup ("Buckshot");
  glutInit(&argc,argv);
  //  Request double buffered, true color window with Z buffering at 600x600
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(600,600);
  window = glutCreateWindow("Buckshot GUI");
#ifdef USEGLEW
  //  Initialize GLEW
  if (glewInit()!=GLEW_OK) Fatal("Error initializing GLEW\n");
  if (!GLEW_VERSION_4_3) Fatal("OpenGL 4.3 not supported\n");
#endif

  //  Set callbacks
  glutDisplayFunc(gwDisplay);
  glutReshapeFunc(gwReshape);
  glutSpecialFunc(gwSpecial);
  glutKeyboardFunc(gwKeyboard);
  glutIdleFunc(gwIdle);

  // Load our shader programs
  shader_program_ = CreateShaderProg(
      "/home/replica/GitMisc/personal_repos/Buckshot/bulletComponents/Graphics/gl430.vert",
      "/home/replica/GitMisc/personal_repos/Buckshot/bulletComponents/Graphics/gl430.frag");
}
