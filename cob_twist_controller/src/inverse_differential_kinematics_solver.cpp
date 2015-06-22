/*!
 *****************************************************************
 * \file
 *
 * \note
 *   Copyright (c) 2014 \n
 *   Fraunhofer Institute for Manufacturing Engineering
 *   and Automation (IPA) \n\n
 *
 *****************************************************************
 *
 * \note
 *   Project name: care-o-bot
 * \note
 *   ROS stack name: cob_control
 * \note
 *   ROS package name: cob_twist_controller
 *
 * \author
 *   Author: Felix Messmer, email: Felix.Messmer@ipa.fraunhofer.de
 *
 * \date Date of creation: April, 2014
 *
 * \brief
 *   This package provides the implementation of an inverse kinematics solver.
 *
 ****************************************************************/
#include "cob_twist_controller/inverse_differential_kinematics_solver.h"

#include <ros/ros.h>
#include <eigen_conversions/eigen_kdl.h>
#include <kdl/chainfksolverpos_recursive.hpp>

/**
 * Solve the inverse kinematics problem at the first order differential level.
 */
int InverseDifferentialKinematicsSolver::CartToJnt(const KDL::JntArray& q_in,
                                                   const KDL::JntArray& last_q_dot,
                                                   const KDL::Twist& v_in,
                                                   const KDL::Frame &base_position,
                                                   const KDL::Frame &chain_base,
                                                   KDL::JntArray& qdot_out)
{
    int8_t retStat = -1;
    this->adjustJac(q_in, base_position, chain_base);

    t_Vector6d v_in_vec;
    tf::twistKDLToEigen(v_in, v_in_vec);

    KDL::ChainFkSolverPos_recursive fk_solver(this->chain_);
    KDL::Frame cartpos;
    fk_solver.JntToCart(q_in, cartpos);

    double rot_x, rot_y, rot_z, rot_w;
    cartpos.M.GetQuaternion(rot_x, rot_y, rot_z, rot_w); // Similar to frametracker
    //cartpos.p.x()
    t_Vector6d p_in_vec;
    p_in_vec << cartpos.p.x(), cartpos.p.y(), cartpos.p.z(), rot_x, rot_y, rot_z;

    this->params_.delta_p_vec = p_in_vec - last_p_in_vec_;

    Eigen::MatrixXd qdot_out_vec;
    retStat = constraint_solver_factory_.calculateJointVelocities(this->params_,
                                                                  this->jac_.data,
                                                                  v_in_vec,
                                                                  q_in,
                                                                  last_q_dot,
                                                                  qdot_out_vec);

    ///convert output
    for(int i = 0; i < jac_.columns(); i++)
    {
        qdot_out(i) = qdot_out_vec(i);
    }

    last_p_in_vec_ = p_in_vec;

    return retStat;
}

/**
 * Adjustment for the Jacobian in case of base gets active or not.
 * If base_active new columns will be added here.
 */
void InverseDifferentialKinematicsSolver::adjustJac(const KDL::JntArray& q_in,
                                                    const KDL::Frame &base_position,
                                                    const KDL::Frame &chain_base)
{
    ///Let the ChainJntToJacSolver calculate the jacobian "jac_chain" for the current joint positions "q_in"
    KDL::Jacobian jac_chain(chain_.getNrOfJoints());
    Eigen::Matrix<double,6,3> jac_b;
    jnt2jac_.JntToJac(q_in, jac_chain);
    KDL::Frame pos;

    if(params_.base_active)
    {
        Eigen::Matrix3d base_rot, tip_base_rot;
        Eigen::Vector3d w_chain_base;
        Eigen::Vector3d r_chain_base;
        Eigen::Vector3d tangential_vel;

        double base_ratio = params_.base_ratio;

        //Create standard platform jacobian
        jac_b.setZero();

        // Get current x and y position from EE and chain_base with respect to base_footprint
        Eigen::Vector3d r_base_link(base_position.p.x(),
                                    base_position.p.y(),
                                    base_position.p.z());

        Eigen::Quaterniond chain_base_quat;
        tf::quaternionKDLToEigen(chain_base.M, chain_base_quat);
        Eigen::Matrix3d chain_base_rot = chain_base_quat.toRotationMatrix();

        // Transform from base_link to chain_base
        Eigen::Vector3d w_base_link(0,0,base_ratio);
        w_chain_base = chain_base_quat*w_base_link;
        r_chain_base = chain_base_quat*r_base_link;

        //Calculate tangential velocity
        tangential_vel = w_chain_base.cross(r_chain_base);

         //Vx-Base <==> q8 effects a change in the following chain_base Vx velocities
        jac_b(0,0) = base_ratio*chain_base_rot(0,0);
        jac_b(0,1) = base_ratio*chain_base_rot(0,1);
        jac_b(0,2) = tangential_vel(0);

        // Vy-Base <==> q9 effects a change in the following chain_base Vy velocities
        jac_b(1,0) = base_ratio*chain_base_rot(1,0);
        jac_b(1,1) = base_ratio*chain_base_rot(1,1);
        jac_b(1,2) = tangential_vel(1);

        // Vz-Base <==>  effects a change in the following chain_base Vz velocities
        jac_b(2,0) = base_ratio*chain_base_rot(2,0);
        jac_b(2,1) = base_ratio*chain_base_rot(2,1);
        jac_b(2,2) = tangential_vel(2);

        //Phi <==> Wz with respect to base_link
        jac_b(3,2) = w_chain_base(0);
        jac_b(4,2) = w_chain_base(1);
        jac_b(5,2) = w_chain_base(2);

        //combine chain Jacobian and platform Jacobian
        t_Matrix6Xd jac_full;
        jac_full.resize(6,chain_.getNrOfJoints() + jac_b.cols());
        jac_full << jac_chain.data, jac_b;
        jac_.resize(chain_.getNrOfJoints() + jac_b.cols());
        jac_.data << jac_full;
    }
    else
    {
        jac_.resize(chain_.getNrOfJoints());
        jac_.data << jac_chain.data;
    }
}
