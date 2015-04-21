#ifndef AUGMENTED_SOLVER_H
#define AUGMENTED_SOLVER_H

#include <math.h>
#include <kdl/chainiksolver.hpp>
#include <kdl/chainjnttojacsolver.hpp>
#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/SVD>
#include <iostream>

#include "cob_twist_controller/augmented_solver_data_types.h"

/**
* Implementation of a inverse velocity kinematics algorithm based
* on the generalize pseudo inverse to calculate the velocity
* transformation from Cartesian to joint space of a general
* KDL::Chain. It uses a svd-calculation based on householders
* rotations.
*
* @ingroup KinematicFamily
*/





class AugmentedSolver
{
public:
    /**
     * Constructor of the solver
     *
     * @param chain the chain to calculate the inverse velocity
     * kinematics for
     * @param eps if a singular value is below this value, its
     * inverse is set to zero, default: 0.00001
     * @param maxiter maximum iterations for the svd calculation,
     * default: 150
     *
     */
    AugmentedSolver(const KDL::Chain& chain, double eps=0.001, int maxiter=5) :
        chain_(chain),
        jac_(chain_.getNrOfJoints()),
        jnt2jac_(chain_),
        maxiter_(maxiter),
        x_(0.0),
        y_(0.0),
        r_(0.0),
        z_(0.0)
    {}

    virtual ~AugmentedSolver() {};
    
    /** CartToJnt for chain using SVD including base and various DampingMethods **/
    virtual int CartToJnt(const KDL::JntArray& q_in, const KDL::JntArray& last_q_dot, KDL::Twist& v_in, KDL::JntArray& qdot_out, KDL::Frame &base_position, KDL::Frame &chain_base);
    virtual int OldCartToJnt(const KDL::JntArray& q_in, const KDL::JntArray& last_q_dot, KDL::Twist& v_in, KDL::JntArray& qdot_out, KDL::Frame &base_position, KDL::Frame &chain_base);

    inline virtual int CartToJnt(const KDL::JntArray& q_in, const KDL::JntArray& last_q_dot, KDL::Twist& v_in, KDL::JntArray& qdot_out)
    {
        KDL::Frame dummy;
        dummy.p = KDL::Vector(0,0,0);
        dummy.M = KDL::Rotation::Quaternion(0,0,0,0);
        return CartToJnt(q_in, last_q_dot, v_in, qdot_out, dummy, dummy);
    }

    inline virtual int OldCartToJnt(const KDL::JntArray& q_in, const KDL::JntArray& last_q_dot, KDL::Twist& v_in, KDL::JntArray& qdot_out)
    {
        KDL::Frame dummy;
        dummy.p = KDL::Vector(0,0,0);
        dummy.M = KDL::Rotation::Quaternion(0,0,0,0);
        return OldCartToJnt(q_in, last_q_dot, v_in, qdot_out, dummy, dummy);
    }

    /** not (yet) implemented. */
    virtual int CartToJnt(const KDL::JntArray& q_init, const KDL::JntArray& last_q_dot, const KDL::FrameVel& v_in, KDL::JntArrayVel& q_out){return -1;};
    
    inline void SetAugmentedSolverParams(AugmentedSolverParams params)
    {
        params_ = params;
    }

    inline AugmentedSolverParams GetAugmentedSolverParams()
    {
        return params_;
    }

private:
    const KDL::Chain chain_;
    KDL::Jacobian jac_, jac_base_;
    KDL::ChainJntToJacSolver jnt2jac_;
    int maxiter_;
    double x_,y_,r_,z_;
    AugmentedSolverParams params_;
    Eigen::VectorXd calculate_weighting(const KDL::JntArray& q, const KDL::JntArray& last_q_dout);
};
#endif
