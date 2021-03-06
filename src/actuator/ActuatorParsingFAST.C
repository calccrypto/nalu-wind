// Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS), National Renewable Energy Laboratory, University of Texas Austin,
// Northwest Research Associates. Under the terms of Contract DE-NA0003525
// with NTESS, the U.S. Government retains certain rights in this software.
//
// This software is released under the BSD 3-clause license. See LICENSE file
// for more details.
//
#include <actuator/ActuatorBulk.h>
#include <actuator/ActuatorBulkFAST.h>
#include <NaluParsing.h>
#include <actuator/ActuatorParsingFAST.h>
#include <NaluEnv.h>

namespace sierra {
namespace nalu {

namespace {
void
readTurbineData(int iTurb, ActuatorMetaFAST& actMetaFAST, YAML::Node turbNode)
{
  fast::fastInputs& fi = actMetaFAST.fastInputs_;
  // Read turbine data for a given turbine using the YAML node
   get_required(
     turbNode, "turbine_name", actMetaFAST.turbineNames_[iTurb]);

   std::string turbFileName;
   get_if_present(
     turbNode, "file_to_dump_turb_pts",
     actMetaFAST.turbineOutputFileNames_[iTurb]);

   get_if_present_no_default(
     turbNode, "fllt_correction",
     actMetaFAST.filterLiftLineCorrection_);

   ThrowErrorMsgIf(
     actMetaFAST.filterLiftLineCorrection_,
     "Filtered lifting line correction has not been implemented in the NGP"
     " actuator models yet.  Please use ActLineFAST instead.");
   // The value epsilon / chord [non-dimensional]
   // This is a vector containing the values for:
   //   - chord aligned (x),
   //   - tangential to chord (y),
   //   - spanwise (z)
   const YAML::Node epsilon_chord = turbNode["epsilon_chord"];
   const YAML::Node epsilon = turbNode["epsilon"];
   if (epsilon && epsilon_chord) {
     throw std::runtime_error(
       "epsilon and epsilon_chord have both been specified for Turbine " +
       std::to_string(iTurb) + "\nYou must pick one or the other.");
   }
   if (epsilon && actMetaFAST.filterLiftLineCorrection_) {
     throw std::runtime_error(
       "epsilon and fllt_correction have both been specified for "
       "Turbine " +
       std::to_string(iTurb) +
       "\nepsilon_chord and epsilon_min should be used with "
       "fllt_correction.");
   }

   std::vector<double> epsilonTemp(3);
   if (
     actMetaFAST.actuatorType_ == ActuatorType::ActLineFASTNGP ||
     actMetaFAST.actuatorType_ == ActuatorType::ActDiskFASTNGP) {
     // only require epsilon
     if (epsilon.Type() == YAML::NodeType::Scalar) {
       double isotropicEpsilon;
       get_required(turbNode, "epsilon", isotropicEpsilon);
       actMetaFAST.isotropicGaussian_ = true;
       for (int j = 0; j < 3; j++) {
         actMetaFAST.epsilon_.h_view(iTurb, j) = isotropicEpsilon;
       }
     }
     else {
       get_required(turbNode, "epsilon", epsilonTemp);
       for (int j = 0; j < 3; j++) {
         actMetaFAST.epsilon_.h_view(iTurb, j) = epsilonTemp[j];
       }
       if (
         epsilonTemp[0] == epsilonTemp[1] &&
         epsilonTemp[1] == epsilonTemp[2]) {
         actMetaFAST.isotropicGaussian_ = true;
       }
       else if (actMetaFAST.is_disk()) {
         throw std::runtime_error("ActDiskFASTNGP does not currently "
                                  "support anisotropic epsilons.");
       }
     }
   } else if (actMetaFAST.actuatorType_ == ActuatorType::AdvActLineFASTNGP) {
     // require epsilon chord and epsilon min
     get_required(turbNode, "epsilon_chord", epsilonTemp);
     for (int j = 0; j < 3; j++) {
       if (epsilonTemp[j] <= 0.0) {
         throw std::runtime_error(
           "ERROR:: zero value for epsilon_chord detected. "
           "All epsilon components must be greater than zero");
       }
       actMetaFAST.epsilonChord_.h_view(iTurb, j) = epsilonTemp[j];
     }

     // Minimum epsilon allowed in simulation. This is required when
     //   specifying epsilon/chord
     get_required(turbNode, "epsilon_min", epsilonTemp);
     for (int j = 0; j < 3; j++) {
       actMetaFAST.epsilon_.h_view(iTurb, j) = epsilonTemp[j];
     }
   }
   // check epsilon values
   for (int j = 0; j < 3; j++) {
     if (actMetaFAST.epsilon_.h_view(iTurb, j) <= 0.0) {
       throw std::runtime_error(
         "ERROR:: zero value for epsilon detected. "
         "All epsilon components must be greater than zero");
     }
   }

   // An epsilon value used for the tower
   const YAML::Node epsilon_tower = turbNode["epsilon_tower"];
   // If epsilon tower is given store it.
   // If not, use the standard epsilon value
   if (epsilon_tower) {
     if(epsilon_tower.Type()==YAML::NodeType::Scalar){
       double epsilonTower = epsilon_tower.as<double>();
       for(int j =0; j<3; j++){
         actMetaFAST.epsilonTower_.h_view(iTurb, j) = epsilonTower;
       }
     }
     else{
       epsilonTemp = epsilon_tower.as<std::vector<double>>();
       for (int j = 0; j < 3; j++) {
         actMetaFAST.epsilonTower_.h_view(iTurb, j) = epsilonTemp[j];
       }
     }
   } else {
     for (int j = 0; j < 3; j++) {
       actMetaFAST.epsilonTower_.h_view(iTurb, j) =
         actMetaFAST.epsilon_.h_view(iTurb, j);
     }
   }
   const YAML::Node epsilon_hub = turbNode["epsilon_hub"];
   if(epsilon_hub){
     if(epsilon_hub.Type() == YAML::NodeType::Scalar){
       const double epsilonHub = epsilon_hub.as<double>();
       for(int j=0; j<3; j++){
         actMetaFAST.epsilonHub_.h_view(iTurb, j) = epsilonHub;
       }
     }
     else{
       epsilonTemp = epsilon_hub.as<std::vector<double>>();
       for(int j=0; j<3; j++){
         actMetaFAST.epsilonHub_.h_view(iTurb, j) = epsilonTemp[j];
       }
     }
   }
  get_required(turbNode, "turb_id", fi.globTurbineData[iTurb].TurbID);
  get_required(
    turbNode, "fast_input_filename",
    fi.globTurbineData[iTurb].FASTInputFileName);
  get_required(
    turbNode, "restart_filename",
    fi.globTurbineData[iTurb].FASTRestartFileName);

  get_required(
    turbNode, "turbine_base_pos", fi.globTurbineData[iTurb].TurbineBasePos);
  if (turbNode["turbine_hub_pos"]) {
    NaluEnv::self().naluOutputP0()
      << "WARNING::turbine_hub_pos is not used. "
      << "The hub location is computed in OpenFAST and is controlled by the "
         "ElastoDyn input file.";
  }
  get_required(
    turbNode, "num_force_pts_blade",
    fi.globTurbineData[iTurb].numForcePtsBlade);

  actMetaFAST.maxNumPntsPerBlade_ = std::max(
    actMetaFAST.maxNumPntsPerBlade_,
    fi.globTurbineData[iTurb].numForcePtsBlade);

  get_required(
    turbNode, "num_force_pts_tower", fi.globTurbineData[iTurb].numForcePtsTwr);

  get_if_present_no_default(
    turbNode, "nacelle_cd", fi.globTurbineData[iTurb].nacelle_cd);
  get_if_present_no_default(
    turbNode, "nacelle_area", fi.globTurbineData[iTurb].nacelle_area);
  get_if_present_no_default(
    turbNode, "air_density", fi.globTurbineData[iTurb].air_density);

  int* numBlades = &(actMetaFAST.nBlades_(iTurb));
  *numBlades = 3;
  get_if_present_no_default(turbNode, "num_blades", *numBlades);
  ThrowErrorMsgIf(
    *numBlades != 3 && *numBlades != 2,
    "ERROR::ActuatorParsingFAST::Currently only 2 and 3 bladed turbines are "
    "supported.");

  if (actMetaFAST.is_disk()) {
    get_if_present_no_default(
      turbNode, "num_swept_pts", actMetaFAST.nPointsSwept_(iTurb));
    actMetaFAST.useUniformAziSampling_(iTurb) =
      actMetaFAST.nPointsSwept_(iTurb) != 0;
    ThrowErrorMsgIf(
      *numBlades != 3, "The ActuatorDisk model requires a base 3 bladed "
                       "turbine, but a 2 bladed turbine was supplied.");
  }

  actMetaFAST.numPointsTurbine_.h_view(iTurb) =
    1 // hub
    + fi.globTurbineData[iTurb].numForcePtsTwr +
    fi.globTurbineData[iTurb].numForcePtsBlade * (*numBlades);
  actMetaFAST.numPointsTotal_ += actMetaFAST.numPointsTurbine_.h_view(iTurb);
}
} // namespace

ActuatorMetaFAST
actuator_FAST_parse(const YAML::Node& y_node, const ActuatorMeta& actMeta)
{
  ActuatorMetaFAST actMetaFAST(actMeta);
  fast::fastInputs& fi = actMetaFAST.fastInputs_;
  fi.comm = NaluEnv::self().parallel_comm();
  fi.nTurbinesGlob = actMetaFAST.numberOfActuators_;

  const YAML::Node y_actuator = y_node["actuator"];
  ThrowErrorMsgIf(
    !y_actuator, "actuator argument is "
                 "missing from yaml node passed to actuator_FAST_parse");
  if (fi.nTurbinesGlob > 0) {
    fi.dryRun = false;
    get_if_present(y_actuator, "debug", fi.debug, false);
    get_required(y_actuator, "t_start", fi.tStart);
    std::string simStartType = "na";
    get_required(y_actuator, "simStart", simStartType);
    if (simStartType == "init") {
      if (fi.tStart == 0) {
        fi.simStart = fast::init;
      } else {
        throw std::runtime_error(
          "actuators: simStart type not consistent with start time for FAST");
      }
    } else if (simStartType == "trueRestart") {
      fi.simStart = fast::trueRestart;
    } else if (simStartType == "restartDriverInitFAST") {
      fi.simStart = fast::restartDriverInitFAST;
    }
    get_required(y_actuator, "n_every_checkpoint", fi.nEveryCheckPoint);
    get_required(y_actuator, "dt_fast", fi.dtFAST);

    get_required(y_actuator, "t_max", fi.tMax);

    if (y_actuator["super_controller"]) {
      get_required(y_actuator, "super_controller", fi.scStatus);
      get_required(y_actuator, "sc_libFile", fi.scLibFile);
      get_required(y_actuator, "num_sc_inputs", fi.numScInputs);
      get_required(y_actuator, "num_sc_outputs", fi.numScOutputs);
    }

    fi.globTurbineData.resize(fi.nTurbinesGlob);

    for (int iTurb = 0; iTurb < fi.nTurbinesGlob; iTurb++) {
      if (y_actuator["Turbine" + std::to_string(iTurb)]) {

        const YAML::Node cur_turbine =
          y_actuator["Turbine" + std::to_string(iTurb)];

        readTurbineData(iTurb, actMetaFAST, cur_turbine);
      } else {
        throw std::runtime_error(
          "Node for Turbine" + std::to_string(iTurb) +
          " not present in input file or I cannot read it");
      }
    }

  } else {
    throw std::runtime_error("Number of turbines <= 0 ");
  }
  return actMetaFAST;
}

} // namespace nalu
} // namespace sierra
