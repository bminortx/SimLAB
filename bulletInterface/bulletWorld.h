#ifndef __MATLAB_BULLET_WRAPPER_
#define __MATLAB_BULLET_WRAPPER_

/*
 * File:   matlab_bullet_wrapper.h
 * Author: bminortx
 * This wrapper is designed to facilitate the use of Bullet Physics
 * in MATLAB. Graphics are handled by MATLAB, as well as parameterization of
 * objects and environmental variables. Bullet takes care of all calculations
 * and returns those results back to MATLAB.
 */

// Suppress Bullet warnings in GCC and Clang
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic pop

#include <map>
#include <vector>
#include <memory>
#include "bulletEntities.h"
// Our shapes
#include "../bulletShapes/bullet_cube.h"
#include "../bulletShapes/bullet_cylinder.h"
#include "../bulletShapes/bullet_heightmap.h"
#include "../bulletShapes/bullet_sphere.h"
#include "../bulletShapes/bullet_vehicle.h"

///////////////////////////////////////////////////////
/// The BulletWorld class
/// The Simulator class holds all of the methods that can be called from
/// bullet_interface_mex.cpp. Any method called from there MUST be in this
/// class.

class BulletWorld {
 public:
  BulletWorld() {
    ///Bullet settings
    timestep_ = 1.0/30.0;
    gravity_ = -9.8;
    max_sub_steps_ = 10; //  bullet -- for stepSimulation
    btDefaultCollisionConfiguration collision_configuration_;
    btCollisionDispatcher dispatcher_ =
        btCollisionDispatcher(&collision_configuration_);
    btDbvtBroadphase broadphase_;
    btSequentialImpulseConstraintSolver constraint_solver_;
    dynamics_world_ = std::make_shared<btDiscreteDynamicsWorld>(
        btDiscreteDynamicsWorld(&dispatcher_,
                                &broadphase_,
                                &constraint_solver_,
                                &collision_configuration_));
    dynamics_world_->setGravity(btVector3(0, 0, gravity_));
  }

  /*********************************************************************
   *ADDING OBJECTS
   **********************************************************************/

  // template <class T>
  inline int AddShapeToWorld(bullet_shape shape) {
    CollisionShapePtr pShape(shape.getBulletShapePtr());
    MotionStatePtr pMotionState(shape.getBulletMotionStatePtr());
    RigidBodyPtr body(shape.getBulletBodyPtr());
    dynamics_world_->addRigidBody(body);
    Shape_Entity pEntity;
    pEntity.rigidbody_ = body;
    pEntity.shape_ = pShape;
    pEntity.motionstate_ = pMotionState;
    int id = shapes_.size();
    shapes_[id] = pEntity;
    return id;
  }

  inline int AddCube(double x_length, double y_length, double z_length,
              double dMass, double dRestitution,
              double* position, double* rotation) {
    bullet_cube btBox(x_length, y_length, z_length, dMass, dRestitution,
                      position, rotation);
    return AddShapeToWorld(btBox);
  }

  int AddSphere(double radius, double dMass, double dRestitution,
                double* position, double* rotation) {
    bullet_sphere btSphere(radius, dMass, dRestitution,
                           position, rotation);
    return AddShapeToWorld(btSphere);
  }

  int AddCylinder(double radius, double height, double dMass,
                  double dRestitution, double* position, double* rotation) {
    bullet_cylinder btCylinder(radius, height, dMass, dRestitution,
                               position, rotation);
    return AddShapeToWorld(btCylinder);
  }

  int AddTerrain(int row_count, int col_count, double grad,
                 double min_ht, double max_ht,
                 double* X, double *Y, double* Z,
                 double* normal) {
    // Let's see if that works... if not, try something else.
    bullet_heightmap btTerrain(row_count, col_count, grad, min_ht,
                               max_ht, X, Y, Z, normal);
    return AddShapeToWorld(btTerrain);
  }

  int AddCompound(double* Shape_ids, double* Con_ids,
                  const char* CompoundType) {
    Compound_Entity pCompound;
    pCompound.shapeid_ = Shape_ids;
    pCompound.constraintid_ = Con_ids;
    if (!std::strcmp(CompoundType, "Vehicle")) {
      pCompound.type_ = VEHICLE;
    }
    int id = compounds_.size();
    compounds_[id] = pCompound;
    return id;
  }

  int AddRaycastVehicle(double* parameters, double* position,
                        double* rotation) {
    bullet_vehicle btRayVehicle(parameters, position, rotation,
                                dynamics_world_.get());
    CollisionShapePtr pShape(btRayVehicle.getBulletShapePtr());
    MotionStatePtr pMotionState(btRayVehicle.getBulletMotionStatePtr());
    RigidBodyPtr body(btRayVehicle.getBulletBodyPtr());
    VehiclePtr vehicle(btRayVehicle.getBulletRaycastVehicle());
    Vehicle_Entity pEntity;
    pEntity.rigidbody_ = body;
    pEntity.shape_ = pShape;
    pEntity.motionstate_ = pMotionState;
    pEntity.vehicle_ = vehicle;
    int id = vehicles_.size();
    vehicles_[id] = pEntity;
    return id;
  }

  /*********************************************************************
   *RUNNING THE SIMULATION
   **********************************************************************/

  void StepSimulation() {
    dynamics_world_->stepSimulation(timestep_,  max_sub_steps_);
  }

  /*********************************************************************
   *COMPOUND METHODS
   **********************************************************************/

  void CommandVehicle(double id, double steering_angle, double force) {
    Compound_Entity Vehicle = compounds_[id];
    double* Shape_ids = Vehicle.shapeid_;
    double* Con_ids = Vehicle.constraintid_;
    btHinge2Constraint* wheel_fl = static_cast<btHinge2Constraint*>(
        constraints_.at(int(Con_ids[0])));
    btHinge2Constraint* wheel_fr = static_cast<btHinge2Constraint*>(
        constraints_.at(int(Con_ids[1])));
    Shape_Entity wheel_bl =
        shapes_.at(int(Shape_ids[3]));
    Shape_Entity wheel_br =
        shapes_.at(int(Shape_ids[4]));
    // Turn the front wheels. This requires manipulation of the constraints.
    wheel_fl->setUpperLimit(steering_angle);
    wheel_fl->setLowerLimit(steering_angle);
    wheel_fr->setUpperLimit(steering_angle);
    wheel_fr->setLowerLimit(steering_angle);
    // Power to the back wheels. Requires torque on the back tires. They're
    // rotated the same way, so torque should be applied in the same direction
    btVector3 torque(0, 0, force);
    wheel_bl.rigidbody_->applyTorque(torque);
    wheel_br.rigidbody_->applyTorque(torque);
  }

  /*********************************************************************
   *RAYCAST VEHICLE METHODS
   **********************************************************************/

  void CommandRaycastVehicle(double id, double steering_angle, double force) {
    VehiclePtr Vehicle = vehicles_[id].vehicle_;
    Vehicle->setSteeringValue(steering_angle, 0);
    Vehicle->setSteeringValue(steering_angle, 1);
    Vehicle->applyEngineForce(force, 2);
    Vehicle->applyEngineForce(force, 3);
  }

  // Holds the steering, engine force, and current velocity
  double* GetRaycastMotionState(double id) {
    double* pose = new double[9];
    VehiclePtr Vehicle = vehicles_[id].vehicle_;
    RigidBodyPtr VehicleBody = vehicles_[id].rigidbody_;
    pose[0] = Vehicle->getSteeringValue(0);
    btWheelInfo wheel = Vehicle->getWheelInfo(2);
    pose[1] = wheel.m_engineForce;
    pose[2] = VehicleBody->getLinearVelocity()[0];
    pose[3] = VehicleBody->getLinearVelocity()[1];
    pose[4] = VehicleBody->getLinearVelocity()[2];
    pose[5] = VehicleBody->getAngularVelocity()[0];
    pose[6] = VehicleBody->getAngularVelocity()[1];
    pose[7] = VehicleBody->getAngularVelocity()[2];
    pose[8] = OnTheGround(id);
    return pose;
  }

  double* RaycastToGround(double id, double x, double y) {
    double* pose = new double[3];
    VehiclePtr Vehicle = vehicles_[id].vehicle_;
    //  Move our vehicle out of the way...
    btVector3 point(x+50, y+50, -100);
    btMatrix3x3 rot = Vehicle->getChassisWorldTransform().getBasis();
    btTransform bullet_trans(rot, point);
    vehicles_[id].rigidbody_->setCenterOfMassTransform(bullet_trans);
    //  Now shoot our ray...
    btVector3 ray_start(x, y, 100);
    btVector3 ray_end(x, y, -100);
    btCollisionWorld::ClosestRayResultCallback ray_callback(ray_start,
                                                            ray_end);
    dynamics_world_->rayTest(ray_start, ray_end, ray_callback);
    btVector3 hitpoint = Vehicle->getChassisWorldTransform().getOrigin();
    if (ray_callback.hasHit()) {
      hitpoint = ray_callback.m_hitPointWorld;
      btWheelInfo wheel = Vehicle->getWheelInfo(2);
      double radius = wheel.m_wheelsRadius;
      //  Find a way to access the height of the car.
      hitpoint.setZ(hitpoint[2]+(3*radius));
    }
    //  Now move our car!
    btTransform bullet_move(rot, hitpoint);
    vehicles_[id].rigidbody_->setCenterOfMassTransform(bullet_move);

    // Now make sure none of our wheels are in the ground.
    // Kind of a nasty oop, but keep it for now.
    double hit = -1;
    double count = 0;
    while (hit == -1 && count<20) {
      for (int i = 0; i<4; i++) {
        hit = Vehicle->rayCast(Vehicle->getWheelInfo(i));
        if (hit!= -1) {
          break;
        }
      }
      // If we're still in the ground, lift us up!
      hitpoint.setZ(hitpoint[2]+.1);
      btTransform bullet_move(rot, hitpoint);
      vehicles_[id].
          rigidbody_->setCenterOfMassTransform(bullet_move);
      if (hit!= -1) {
        break;
      }
      count++;
      if (count == 20) {
        break;
      }
    }
    int on = false;
    btVector3 VehiclePose = Vehicle->getChassisWorldTransform().getOrigin();
    while (on == 0) {
      on = OnTheGround(id);
      StepSimulation();
      VehiclePose = Vehicle->getChassisWorldTransform().getOrigin();
    }
    pose[0] = VehiclePose[0];
    pose[1] = VehiclePose[1];
    pose[2] = VehiclePose[2];
    return pose;
  }

  //  This just drops us off on the surface...
  int OnTheGround(double id) {
    VehiclePtr Vehicle = vehicles_[id].vehicle_;
    int OnGround = 0;
    int hit = 0;
    for (int i = 0; i<4; i++) {
      hit = hit + Vehicle->rayCast(Vehicle->getWheelInfo(i));
    }
    if (hit == 0) {
      OnGround = 1;
    }
    return OnGround;
  }

  void SetVehicleVels(double id, double* lin_vel, double* ang_vel) {
    RigidBodyPtr VehicleBody = vehicles_[id].rigidbody_;
    VehiclePtr Vehicle = vehicles_[id].vehicle_;
    btVector3 Lin(lin_vel[0], lin_vel[1], lin_vel[2]);
    btVector3 Ang(ang_vel[0], ang_vel[1], ang_vel[2]);
    VehicleBody->setLinearVelocity(Lin);
    VehicleBody->setAngularVelocity(Ang);
    Vehicle->resetSuspension();
  }

  void ResetVehicle(double id, double* start_pose, double* start_rot) {
    //  Move the vehicle into start position
    btMatrix3x3 rot(start_rot[0], start_rot[3], start_rot[6],
                    start_rot[1], start_rot[4], start_rot[7],
                    start_rot[2], start_rot[5], start_rot[8]);
    btVector3 pose(start_pose[0], start_pose[1], start_pose[2]);
    btTransform bullet_trans(rot, pose);
    //  Reset our car to its initial state.
    vehicles_[id].rigidbody_->setCenterOfMassTransform(bullet_trans);
  }

  double* SpeedSim(double id, double* start_pose, double* start_rot,
                   double* start_lin_vel, double* start_ang_vel,
                   double* forces, double* steering_angles,
                   double command_length) {
    int state_size = (command_length*3)+22;
    double* states = new double[state_size];
    VehiclePtr Vehicle = vehicles_[id].vehicle_;
    ResetVehicle(id, start_pose, start_rot);
    SetVehicleVels(id, start_lin_vel, start_ang_vel);

    //  Run our commands through
    for (int i = 0; i < command_length; i++) {
      CommandRaycastVehicle(id, steering_angles[i], forces[i]);
      StepSimulation();
      btVector3 VehiclePose =
          Vehicle->getChassisWorldTransform().getOrigin();
      states[3*i] = VehiclePose[0];
      states[3*i+1] = VehiclePose[1];
      states[3*i+2] = VehiclePose[2];

      //  Get our whole state on the last step.

      if (i == command_length-1) {
        btVector3 VehiclePose =
            Vehicle->getChassisWorldTransform().getOrigin();
        btMatrix3x3 VehicleRot =
            Vehicle->getChassisWorldTransform().getBasis();
        states[3*i+3] = VehiclePose[0];
        states[3*i+4] = VehiclePose[1];
        states[3*i+5] = VehiclePose[2];
        states[3*i+6] = VehicleRot[0][0];
        states[3*i+7] = VehicleRot[1][0];
        states[3*i+8] = VehicleRot[2][0];
        states[3*i+9] = VehicleRot[0][1];
        states[3*i+10] = VehicleRot[1][1];
        states[3*i+11] = VehicleRot[2][1];
        states[3*i+12] = VehicleRot[0][2];
        states[3*i+13] = VehicleRot[1][2];
        states[3*i+14] = VehicleRot[2][2];
        double* motionstate = GetRaycastMotionState(id);
        states[3*i+15] = motionstate[2];
        states[3*i+16] = motionstate[3];
        states[3*i+17] = motionstate[4];
        states[3*i+18] = motionstate[5];
        states[3*i+19] = motionstate[6];
        states[3*i+20] = motionstate[7];
        states[3*i+21] = motionstate[8];
      }
    }

    // Reset our vehicle again (just in case this is our last iteration)
    ResetVehicle(id, start_pose, start_rot);
    SetVehicleVels(id, start_lin_vel, start_ang_vel);
    CommandRaycastVehicle(id, 0, 0);
    return states;
  }

  /*********************************************************************
   *CONSTRAINT METHODS
   *All of the constructors for our constraints.
   **********************************************************************/

  inline int AddConstraintToWorld(btTypedConstraint& constraint) {
    int id = constraints_.size();
    constraints_[id] = &constraint;
    dynamics_world_->addConstraint(constraints_[id]);
    return id;
  }

  ///////
  // Point-to-Point
  int PointToPoint_one(double id_A, double* pivot_in_A) {
    Shape_Entity Shape_A = shapes_.at(id_A);
    btVector3 pivot_A(pivot_in_A[0], pivot_in_A[1], pivot_in_A[2]);
    btPoint2PointConstraint constraint(*Shape_A.rigidbody_, pivot_A);
    return AddConstraintToWorld(constraint);
  }

  int PointToPoint_two(double id_A, double id_B,
                       double* pivot_in_A, double* pivot_in_B) {
    Shape_Entity Shape_A = shapes_.at(id_A);
    Shape_Entity Shape_B = shapes_.at(id_B);
    btVector3 pivot_A(pivot_in_A[0], pivot_in_A[1], pivot_in_A[2]);
    btVector3 pivot_B(pivot_in_B[0], pivot_in_B[1], pivot_in_B[2]);
    btPoint2PointConstraint constraint(*Shape_A.rigidbody_,
                                 *Shape_B.rigidbody_,
                                 pivot_A, pivot_B);
    return AddConstraintToWorld(constraint);
  }

  ///////
  // Hinge
  int Hinge_one_transform(double id_A, double* transform_A, double* limits) {
    Shape_Entity Shape_A = shapes_.at(id_A);
    int id = 0;
    return id;
  }

  int Hinge_two_transform(double id_A, double id_B,
                          double* transform_A, double* transform_B,
                          double* limits) {
    Shape_Entity Shape_A = shapes_.at(id_A);
    Shape_Entity Shape_B = shapes_.at(id_B);
    int id = 0;
    return id;

  }

  int Hinge_one_pivot(double id_A, double* pivot_in_A,
                      double* axis_in_A, double* limits) {
    Shape_Entity Shape_A = shapes_.at(id_A);
    btVector3 pivot_A(pivot_in_A[0], pivot_in_A[1], pivot_in_A[2]);
    btVector3 axis_A(axis_in_A[0], axis_in_A[1], axis_in_A[2]);
    btHingeConstraint Hinge(*Shape_A.rigidbody_, pivot_A,
                            axis_A, true);
    Hinge.setLimit(limits[0], limits[1], limits[2], limits[3], limits[4]);
    return AddConstraintToWorld(Hinge);
  }

  int Hinge_two_pivot(double id_A, double id_B,
                      double* pivot_in_A, double* pivot_in_B,
                      double* axis_in_A, double* axis_in_B,
                      double* limits) {
    Shape_Entity Shape_A = shapes_.at(id_A);
    Shape_Entity Shape_B = shapes_.at(id_B);
    btVector3 pivot_A(pivot_in_A[0], pivot_in_A[1], pivot_in_A[2]);
    btVector3 axis_A(axis_in_A[0], axis_in_A[1], axis_in_A[2]);
    btVector3 pivot_B(pivot_in_B[0], pivot_in_B[1], pivot_in_B[2]);
    btVector3 axis_B(axis_in_B[0], axis_in_B[1], axis_in_B[2]);
    btHingeConstraint Hinge(*Shape_A.rigidbody_,
                            *Shape_B.rigidbody_,
                            pivot_A, pivot_B,
                            axis_A, axis_B,
                            true);
    Hinge.setLimit(limits[0], limits[1], limits[2], limits[3], limits[4]);
    return AddConstraintToWorld(Hinge);
  }

  ///////
  // Hinge2
  int Hinge2(double id_A, double id_B, double* Anchor, double* Axis_1,
             double* Axis_2, double damping, double stiffness,
             double steering_angle) {
    Shape_Entity Shape_A = shapes_.at(id_A);
    Shape_Entity Shape_B = shapes_.at(id_B);
    btVector3 btAnchor(Anchor[0], Anchor[1], Anchor[2]);
    btVector3 btAxis_1(Axis_1[0], Axis_1[1], Axis_1[2]);
    btVector3 btAxis_2(Axis_2[0], Axis_2[1], Axis_2[2]);
    btHinge2Constraint Hinge2(*Shape_A.rigidbody_,
                              *Shape_B.rigidbody_,
                              btAnchor, btAxis_1, btAxis_2);
    Hinge2.setUpperLimit(steering_angle);
    Hinge2.setLowerLimit(steering_angle);
    Hinge2.enableSpring(3, true);
    Hinge2.setStiffness(3, stiffness);
    Hinge2.setDamping(3, damping);
    return AddConstraintToWorld(Hinge2);
  }

  ///////
  // Six DOF
  int SixDOF_one(double id_A, double* transform_A, double* limits) {
    Shape_Entity Shape_A = shapes_.at(id_A);
    btQuaternion quat_A(transform_A[3], transform_A[4],
                        transform_A[5], transform_A[6]);
    btVector3 pos_A(transform_A[0], transform_A[1], transform_A[2]);
    btTransform trans_A(quat_A, pos_A);
    btGeneric6DofConstraint SixDOF(*Shape_A.rigidbody_,
                                   trans_A,
                                   true);
    btVector3 max_lin_limits(limits[0], limits[1],  limits[2]);
    btVector3 min_lin_limits(limits[3], limits[4],  limits[5]);
    btVector3 max_ang_limits(limits[6], limits[7],  limits[8]);
    btVector3 min_ang_limits(limits[9], limits[10], limits[11]);
    SixDOF.setLinearLowerLimit(min_lin_limits);
    SixDOF.setLinearUpperLimit(max_lin_limits);
    SixDOF.setAngularLowerLimit(min_ang_limits);
    SixDOF.setAngularUpperLimit(max_ang_limits);
    return AddConstraintToWorld(SixDOF);
  }

    /*********************************************************************
   *GETTERS FOR OBJECT POSES
   **********************************************************************/

  std::vector<double> GetShapeTransform(double id) {
    // Returns:
    // 1. The positon of the object in the world
    // 2. The rotation matrix that's used in motion.
    int n_id = (int)id;
    // double* pose = new double[12];
    Shape_Entity entity = shapes_.at(n_id);
    btTransform world_transform =
        entity.rigidbody_->getCenterOfMassTransform();
    btMatrix3x3 rotation = world_transform.getBasis();
    btVector3 position = world_transform.getOrigin();
    std::vector<double> pose {
      position[0], position[1], position[2],
          rotation[0][0], rotation[1][0], rotation[2][0],
          rotation[0][1], rotation[1][1], rotation[2][1],
          rotation[0][2], rotation[1][2], rotation[2][2]
    };
    return pose;
  }

  std::vector<double> GetConstraintTransform(double id) {
    int n_id = (int)id;
    btHinge2Constraint* constraint =
        static_cast<btHinge2Constraint*>(constraints_.at(n_id));
    btVector3 position = constraint->getAnchor();
    std::vector<double> pose {
      position[0], position[1], position[2] };
    return pose;
  }

  //////////
  // For RaycastVehicles
  // Returns:
  // 1. The position and rotation of the body of the vehicle.
  // 2. The position and rotations for each of the wheels.
  //////////

  std::vector< btTransform > GetVehiclePoses(Vehicle_Entity& Vehicle) {
    std::vector<btTransform> VehiclePoses;
    btTransform VehiclePose;
    VehiclePose.setIdentity();
    VehiclePose = Vehicle.vehicle_->getChassisWorldTransform();
    VehiclePoses.push_back(VehiclePose);
    for (int i = 0; i<Vehicle.vehicle_->getNumWheels(); i++) {
      Vehicle.vehicle_->updateWheelTransform(i, false);
      btTransform WheelPose;
      WheelPose.setIdentity();
      WheelPose = Vehicle.vehicle_->getWheelTransformWS(i);
      VehiclePoses.push_back(WheelPose);
    }
    return VehiclePoses;
  }

  double* GetVehicleTransform(double id) {
    int n_id = (int)id;
    Vehicle_Entity Vehicle = vehicles_.at(n_id);
    std::vector<btTransform> Transforms = GetVehiclePoses(Vehicle);
    double* pose = new double[12*5];
    for (unsigned int i = 0; i<5; i++) {
      btMatrix3x3 rotation = Transforms.at(i).getBasis();
      btVector3 position = Transforms.at(i).getOrigin();
      pose[i*12+0] = position[0];
      pose[i*12+1] = position[1];
      pose[i*12+2] = position[2];
      pose[i*12+3] =  rotation[0][0];
      pose[i*12+4] =  rotation[1][0];
      pose[i*12+5] =  rotation[2][0];
      pose[i*12+6] =  rotation[0][1];
      pose[i*12+7] =  rotation[1][1];
      pose[i*12+8] =  rotation[2][1];
      pose[i*12+9] =  rotation[0][2];
      pose[i*12+10] = rotation[1][2];
      pose[i*12+11] = rotation[2][2];
    }
    return pose;
  }

  //////////////////////////////////////////////////////////////////

  std::map<int, Compound_Entity> compounds_;
  std::map<int, btTypedConstraint*> constraints_;
  std::map<int, Shape_Entity> shapes_;
  std::map<int, Vehicle_Entity> vehicles_;

 private:
  std::shared_ptr<btDiscreteDynamicsWorld> dynamics_world_;
  double timestep_;
  double gravity_;
  int max_sub_steps_;
};

#endif
