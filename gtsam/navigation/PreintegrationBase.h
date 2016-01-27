/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 *  @file  PreintegrationBase.h
 *  @author Luca Carlone
 *  @author Stephen Williams
 *  @author Richard Roberts
 *  @author Vadim Indelman
 *  @author David Jensen
 *  @author Frank Dellaert
 **/

#pragma once

#include <gtsam/navigation/PreintegrationParams.h>
#include <gtsam/navigation/NavState.h>
#include <gtsam/navigation/ImuBias.h>
#include <gtsam/linear/NoiseModel.h>

#include <iosfwd>

namespace gtsam {

#define ALLOW_DEPRECATED_IN_GTSAM4
#ifdef ALLOW_DEPRECATED_IN_GTSAM4
/// @deprecated
struct PoseVelocityBias {
  Pose3 pose;
  Vector3 velocity;
  imuBias::ConstantBias bias;
  PoseVelocityBias(const Pose3& _pose, const Vector3& _velocity,
      const imuBias::ConstantBias _bias) :
      pose(_pose), velocity(_velocity), bias(_bias) {
  }
  PoseVelocityBias(const NavState& navState, const imuBias::ConstantBias _bias) :
      pose(navState.pose()), velocity(navState.velocity()), bias(_bias) {
  }
  NavState navState() const {
    return NavState(pose, velocity);
  }
};
#endif

/**
 * PreintegrationBase is the base class for PreintegratedMeasurements
 * (in ImuFactor) and CombinedPreintegratedMeasurements (in CombinedImuFactor).
 * It includes the definitions of the preintegrated variables and the methods
 * to access, print, and compare them.
 */
class PreintegrationBase {
 public:
  typedef imuBias::ConstantBias Bias;
  typedef PreintegrationParams Params;

  /// The IMU is integrated in the tangent space, represented by a Vector9
  /// This small inner class provides some convenient constructors and efficient
  /// access to the orientation, position, and velocity components
  class TangentVector {
    Vector9 v_;
    typedef const Vector9 constV9;

   public:
    TangentVector() { v_.setZero(); }
    TangentVector(const Vector9& v) : v_(v) {}
    TangentVector(const Vector3& theta, const Vector3& pos,
                  const Vector3& vel) {
      this->theta() = theta;
      this->position() = pos;
      this->velocity() = vel;
    }

    const Vector9& vector() const { return v_; }

    Eigen::Block<Vector9, 3, 1> theta() { return v_.segment<3>(0); }
    Eigen::Block<constV9, 3, 1> theta() const { return v_.segment<3>(0); }

    Eigen::Block<Vector9, 3, 1> position() { return v_.segment<3>(3); }
    Eigen::Block<constV9, 3, 1> position() const { return v_.segment<3>(3); }

    Eigen::Block<Vector9, 3, 1> velocity() { return v_.segment<3>(6); }
    Eigen::Block<constV9, 3, 1> velocity() const { return v_.segment<3>(6); }

   private:
     /** Serialization function */
     friend class boost::serialization::access;
     template <class ARCHIVE>
     void serialize(ARCHIVE& ar, const unsigned int /*version*/) {
       namespace bs = ::boost::serialization;
       ar & bs::make_nvp("v_", bs::make_array(v_.data(), v_.size()));
     }
  };

 protected:

  /// Parameters. Declared mutable only for deprecated predict method.
  /// TODO(frank): make const once deprecated method is removed
#ifdef ALLOW_DEPRECATED_IN_GTSAM4
  mutable
#endif
  boost::shared_ptr<Params> p_;

  /// Acceleration and gyro bias used for preintegration
  imuBias::ConstantBias biasHat_;

  /// Time interval from i to j
  double deltaTij_;

  /**
   * Preintegrated navigation state, from frame i to frame j
   * Note: relative position does not take into account velocity at time i, see deltap+, in [2]
   * Note: velocity is now also in frame i, as opposed to deltaVij in [2]
   */
  /// Current estimate of deltaXij_k
  TangentVector deltaXij_;

  Matrix3 delRdelBiasOmega_; ///< Jacobian of preintegrated rotation w.r.t. angular rate bias
  Matrix3 delPdelBiasAcc_;   ///< Jacobian of preintegrated position w.r.t. acceleration bias
  Matrix3 delPdelBiasOmega_; ///< Jacobian of preintegrated position w.r.t. angular rate bias
  Matrix3 delVdelBiasAcc_;   ///< Jacobian of preintegrated velocity w.r.t. acceleration bias
  Matrix3 delVdelBiasOmega_; ///< Jacobian of preintegrated velocity w.r.t. angular rate bias

  /// Default constructor for serialization
  PreintegrationBase() {
    resetIntegration();
  }

public:
  /// @name Constructors
  /// @{

  /**
   *  Constructor, initializes the variables in the base class
   *  @param p    Parameters, typically fixed in a single application
   *  @param bias Current estimate of acceleration and rotation rate biases
   */
  PreintegrationBase(const boost::shared_ptr<Params>& p,
      const imuBias::ConstantBias& biasHat = imuBias::ConstantBias());

  /**
   *  Constructor which takes in all members for generic construction
   */
  PreintegrationBase(const boost::shared_ptr<Params>& p, const imuBias::ConstantBias& biasHat,
                     double deltaTij, const TangentVector& zeta, const Matrix3& delPdelBiasAcc,
                     const Matrix3& delPdelBiasOmega, const Matrix3& delVdelBiasAcc,
                     const Matrix3& delVdelBiasOmega)
      : p_(p),
        biasHat_(biasHat),
        deltaTij_(deltaTij),
        deltaXij_(zeta),
        delPdelBiasAcc_(delPdelBiasAcc),
        delPdelBiasOmega_(delPdelBiasOmega),
        delVdelBiasAcc_(delVdelBiasAcc),
        delVdelBiasOmega_(delVdelBiasOmega) {}
  /// @}

  /// @name Basic utilities
  /// @{
  /// Re-initialize PreintegratedMeasurements
  void resetIntegration();

  /// check parameters equality: checks whether shared pointer points to same Params object.
  bool matchesParamsWith(const PreintegrationBase& other) const {
    return p_ == other.p_;
  }

  /// shared pointer to params
  const boost::shared_ptr<Params>& params() const {
    return p_;
  }

  /// const reference to params
  const Params& p() const {
    return *boost::static_pointer_cast<Params>(p_);
  }

#ifdef ALLOW_DEPRECATED_IN_GTSAM4
  void set_body_P_sensor(const Pose3& body_P_sensor) {
    p_->body_P_sensor = body_P_sensor;
  }
#endif
/// @}

  /// @name Instance variables access
  /// @{
  const imuBias::ConstantBias& biasHat() const { return biasHat_; }
  const double& deltaTij() const { return deltaTij_; }

  const Vector9& zeta() const { return deltaXij_.vector(); }

  Vector3 theta() const { return deltaXij_.theta(); }
  Vector3 deltaPij() const { return deltaXij_.position(); }
  Vector3 deltaVij() const { return deltaXij_.velocity(); }

  Rot3 deltaRij() const { return Rot3::Expmap(deltaXij_.theta()); }
  NavState deltaXij() const { return NavState::Retract(deltaXij_.vector()); }

  const Matrix3& delRdelBiasOmega() const { return delRdelBiasOmega_; }
  const Matrix3& delPdelBiasAcc() const { return delPdelBiasAcc_; }
  const Matrix3& delPdelBiasOmega() const { return delPdelBiasOmega_; }
  const Matrix3& delVdelBiasAcc() const { return delVdelBiasAcc_; }
  const Matrix3& delVdelBiasOmega() const { return delVdelBiasOmega_; }

  // Exposed for MATLAB
  Vector6 biasHatVector() const { return biasHat_.vector(); }
  /// @}

  /// @name Testable
  /// @{
  GTSAM_EXPORT friend std::ostream& operator<<(std::ostream& os, const PreintegrationBase& pim);
  void print(const std::string& s) const;
  bool equals(const PreintegrationBase& other, double tol) const;
  /// @}

  /// @name Main functionality
  /// @{

  /// Subtract estimate and correct for sensor pose
  /// Compute the derivatives due to non-identity body_P_sensor (rotation and centrifugal acc)
  /// Ignore D_correctedOmega_measuredAcc as it is trivially zero
  std::pair<Vector3, Vector3> correctMeasurementsByBiasAndSensorPose(
      const Vector3& j_measuredAcc, const Vector3& j_measuredOmega,
      OptionalJacobian<3, 3> D_correctedAcc_measuredAcc = boost::none,
      OptionalJacobian<3, 3> D_correctedAcc_measuredOmega = boost::none,
      OptionalJacobian<3, 3> D_correctedOmega_measuredOmega = boost::none) const;

  // Update integrated vector on tangent manifold zeta with acceleration
  // readings a_body and gyro readings w_body, bias-corrected in body frame.
  static TangentVector UpdateEstimate(const Vector3& a_body,
                                      const Vector3& w_body, double dt,
                                      const TangentVector& zeta,
                                      OptionalJacobian<9, 9> A = boost::none,
                                      OptionalJacobian<9, 3> B = boost::none,
                                      OptionalJacobian<9, 3> C = boost::none);

  /// Calculate the updated preintegrated measurement, does not modify
  /// It takes measured quantities in the j frame
  PreintegrationBase::TangentVector updatedDeltaXij(
      const Vector3& j_measuredAcc, const Vector3& j_measuredOmega, double dt,
      OptionalJacobian<9, 9> A = boost::none,
      OptionalJacobian<9, 3> B = boost::none,
      OptionalJacobian<9, 3> C = boost::none) const;

  /// Update preintegrated measurements and get derivatives
  /// It takes measured quantities in the j frame
  void update(const Vector3& j_measuredAcc, const Vector3& j_measuredOmega,
              const double deltaT, Matrix3* D_incrR_integratedOmega, Matrix9* A,
              Matrix93* B, Matrix93* C);

  /// Given the estimate of the bias, return a NavState tangent vector
  /// summarizing the preintegrated IMU measurements so far
  Vector9 biasCorrectedDelta(const imuBias::ConstantBias& bias_i,
      OptionalJacobian<9, 6> H = boost::none) const;

  /// Predict state at time j
  NavState predict(const NavState& state_i, const imuBias::ConstantBias& bias_i,
                   OptionalJacobian<9, 9> H1 = boost::none,
                   OptionalJacobian<9, 6> H2 = boost::none) const;

  /// Compute errors w.r.t. preintegrated measurements and jacobians wrt pose_i, vel_i, bias_i, pose_j, bias_j
  Vector9 computeErrorAndJacobians(const Pose3& pose_i, const Vector3& vel_i,
      const Pose3& pose_j, const Vector3& vel_j,
      const imuBias::ConstantBias& bias_i, OptionalJacobian<9, 6> H1 =
          boost::none, OptionalJacobian<9, 3> H2 = boost::none,
      OptionalJacobian<9, 6> H3 = boost::none, OptionalJacobian<9, 3> H4 =
          boost::none, OptionalJacobian<9, 6> H5 = boost::none) const;

  /// @}

#ifdef ALLOW_DEPRECATED_IN_GTSAM4
  /// @name Deprecated
  /// @{

  /// @deprecated predict
  PoseVelocityBias predict(const Pose3& pose_i, const Vector3& vel_i,
      const imuBias::ConstantBias& bias_i, const Vector3& n_gravity,
      const Vector3& omegaCoriolis, const bool use2ndOrderCoriolis = false) const;

  /// @}
#endif

private:
  /** Serialization function */
  friend class boost::serialization::access;
  template<class ARCHIVE>
  void serialize(ARCHIVE & ar, const unsigned int /*version*/) {
    namespace bs = ::boost::serialization;
    ar & BOOST_SERIALIZATION_NVP(p_);
    ar & BOOST_SERIALIZATION_NVP(deltaTij_);
    ar & BOOST_SERIALIZATION_NVP(deltaXij_);
    ar & BOOST_SERIALIZATION_NVP(biasHat_);
    ar & bs::make_nvp("delRdelBiasOmega_", bs::make_array(delRdelBiasOmega_.data(), delRdelBiasOmega_.size()));
    ar & bs::make_nvp("delPdelBiasAcc_", bs::make_array(delPdelBiasAcc_.data(), delPdelBiasAcc_.size()));
    ar & bs::make_nvp("delPdelBiasOmega_", bs::make_array(delPdelBiasOmega_.data(), delPdelBiasOmega_.size()));
    ar & bs::make_nvp("delVdelBiasAcc_", bs::make_array(delVdelBiasAcc_.data(), delVdelBiasAcc_.size()));
    ar & bs::make_nvp("delVdelBiasOmega_", bs::make_array(delVdelBiasOmega_.data(), delVdelBiasOmega_.size()));
  }
};

} /// namespace gtsam
