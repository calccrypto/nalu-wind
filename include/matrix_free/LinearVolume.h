// Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS), National Renewable Energy Laboratory, University of Texas Austin,
// Northwest Research Associates. Under the terms of Contract DE-NA0003525
// with NTESS, the U.S. Government retains certain rights in this software.
//
// This software is released under the BSD 3-clause license. See LICENSE file
// for more details.
//

#ifndef LINEAR_VOLUME_H
#define LINEAR_VOLUME_H

#include "matrix_free/PolynomialOrders.h"
#include "matrix_free/KokkosFramework.h"
#include "matrix_free/LocalArray.h"

namespace sierra {
namespace nalu {
namespace matrix_free {
namespace geom {

namespace impl {

template <int p>
struct volume_metric_t
{
  static scalar_view<p> invoke(
    const_scalar_view<p> alpha, const_vector_view<p> coordinates);
  static scalar_view<p> invoke(const_vector_view<p> coordinates);
};

} // namespace impl
P_INVOKEABLE(volume_metric)

} // namespace geom
} // namespace matrix_free
} // namespace nalu
} // namespace sierra

#endif
