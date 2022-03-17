/*
 * GridmapSignedDistanceField.h
 *
 *  Created on: Jul 10, 2020
 *      Author: Ruben Grandia
 *   Institute: ETH Zurich
 */

#pragma once

#include <vector>

#include <Eigen/Dense>

#include <grid_map_core/GridMap.hpp>
#include <grid_map_core/TypeDefs.hpp>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include "Gridmap3dLookup.h"

namespace grid_map {

/**
 * This class creates a dense 3D signed distance field grid for a given elevation map.
 * The 3D grid uses the same resolution as the 2D map. i.e. all voxels are of size (resolution x resolution x resolution).
 * The size of the 3D grid is the same as the map in XY direction. The size is Z direction is specified in the constructor.
 *
 * During the creation of the class all values and derivatives are pre-computed in one go. This makes repeated lookups very cheap.
 * Querying a point outside of the constructed grid will result in linear extrapolation.
 */
class GridmapSignedDistanceField {
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  using Derivative3 = Eigen::Vector3d;

  /**
   * Create a signed distance field and its derivative for an elevation layer in the grid map.
   *
   * @param gridMap : Input map to create the SDF for.
   * @param elevationLayer : Name of the elevation layer.
   * @param minHeight : Desired starting height of the 3D SDF grid.
   * @param maxHeight : Desired ending height of the 3D SDF grid. (Will be rounded up to match the resolution)
   */
  GridmapSignedDistanceField(const GridMap& gridMap, const std::string& elevationLayer, double minHeight, double maxHeight);

  /**
   * Get the signed distance value at a 3D position.
   * @param position : 3D position in the frame of the gridmap.
   * @return signed distance to the elevation surface.
   */
  double value(const Position3& position) const noexcept;

  /**
   * Get the signed distance derivative at a 3D position.
   * @param position : 3D position in the frame of the gridmap.
   * @return derivative of the signed distance field.
   */
  Derivative3 derivative(const Position3& position) const noexcept;

  /**
   * Get both the signed distance value and derivative at a 3D position.
   * @param position : 3D position in the frame of the gridmap.
   * @return {value, derivative} of the signed distance field.
   */
  std::pair<double, Derivative3> valueAndDerivative(const Position3& position) const noexcept;

  /**
   * Return the signed distance field as a pointcloud. The signed distance is assigned to the point's intensity.
   * @param decimation : specifies how many points are returned. 1: all points, 2: every second point, etc.
   * @param condition : specifies the condition on the distance value to add it to the pointcloud (default = any distance is added)
   */
  pcl::PointCloud<pcl::PointXYZI> asPointCloud(
      size_t decimation = 1, const std::function<bool(float)>& condition = [](float) { return true; }) const;

  /**
   * Return the signed distance field as a pointcloud where the distance is positive.
   * @param decimation : specifies how many points are returned. 1: all points, 2: every second point, etc.
   */
  pcl::PointCloud<pcl::PointXYZI> freeSpacePointCloud(size_t decimation = 1) const;

  /**
   * Return the signed distance field as a pointcloud where the distance is negative.
   * @param decimation : specifies how many points are returned. 1: all points, 2: every second point, etc.
   */
  pcl::PointCloud<pcl::PointXYZI> obstaclePointCloud(size_t decimation = 1) const;

 private:
  /**
   * Implementation of the signed distance field computation in this class.
   * @param elevation
   */
  void computeSignedDistance(const Matrix& elevation);

  /**
   * Simultaneously compute the signed distance and derivative in x direction at a given height
   * @param elevation [in] : elevation data
   * @param currentLayer [out] : signed distance values
   * @param dxTranspose [out] : derivative in x direction (the matrix is transposed)
   * @param sdfTranspose [tmp] : temporary variable to reuse between calls (allocated on first use)
   * @param tmp [tmp] : temporary variable (allocated on first use)
   * @param tmpTranspose [tmp] : temporary variable (allocated on first use)
   * @param height [in] : height the signed distance is created for.
   * @param resolution [in] : resolution of the map.
   * @param minHeight [in] : smallest height value in the elevation data.
   * @param maxHeight [in] : largest height value in the elevation data.
   */
  void computeLayerSdfandDeltaX(const Matrix& elevation, Matrix& currentLayer, Matrix& dxTranspose, Matrix& sdfTranspose, Matrix& tmp,
                                Matrix& tmpTranspose, float height, float resolution, float minHeight, float maxHeight) const;

  /**
   * Add the computed signed distance values and derivatives at a particular height to the grid.
   * Adds the layer to the back of data_
   * @param signedDistance : signed distance values.
   * @param dxTranspose : x components of the derivative (matrix is transposed).
   * @param dy : y components of the derivative.
   * @param dz : z components of the derivative.
   */
  void emplacebackLayerData(const Matrix& signedDistance, const Matrix& dxTranspose, const Matrix& dy, const Matrix& dz);

  //! Data structure to store together {signed distance value, derivative}.
  using node_data_t = std::array<float, 4>;

  /** Helper function to extract the sdf value */
  static double distance(const node_data_t& nodeData) noexcept { return nodeData[0]; }

  /** Helper function to extract the sdf value as float */
  static float distanceFloat(const node_data_t& nodeData) noexcept { return nodeData[0]; }

  /** Helper function to extract the sdf derivative */
  static Derivative3 derivative(const node_data_t& nodeData) noexcept { return {nodeData[1], nodeData[2], nodeData[3]}; }

  //! Object encoding the 3D grid.
  signed_distance_field::Gridmap3dLookup gridmap3DLookup_;

  //! Object encoding the signed distance value and derivative in the grid.
  std::vector<node_data_t> data_;

  //! Frame id of the grid map.
  std::string frameId_;

  //! Timestamp of the grid map (nanoseconds).
  Time timestamp_;
};

}  // namespace grid_map