// Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS), National Renewable Energy Laboratory, University of Texas Austin,
// Northwest Research Associates. Under the terms of Contract DE-NA0003525
// with NTESS, the U.S. Government retains certain rights in this software.
//
// This software is released under the BSD 3-clause license. See LICENSE file
// for more details.
//

#include "matrix_free/StkSimdGatheredElementData.h"

#include "matrix_free/PolynomialOrders.h"
#include "matrix_free/StkSimdConnectivityMap.h"

#include "Kokkos_ExecPolicy.hpp"
#include "Kokkos_Macros.hpp"

#include "stk_mesh/base/Field.hpp"
#include "stk_mesh/base/Types.hpp"
#include "stk_mesh/base/NgpMesh.hpp"
#include "stk_mesh/base/NgpField.hpp"
#include "stk_simd/Simd.hpp"

namespace sierra {
namespace nalu {
namespace matrix_free {
namespace impl {

template <int p>
void
stk_simd_scalar_field_gather_t<p>::invoke(
  const_elem_mesh_index_view<p> conn,
  const stk::mesh::NgpField<double>& field,
  scalar_view<p> simd_element_field)
{
  Kokkos::parallel_for(
    Kokkos::RangePolicy<exec_space, int>(0, conn.extent_int(0)),
    KOKKOS_LAMBDA(int index) {
      for (int n = 0; n < simd_len; ++n) {
        for (int k = 0; k < p + 1; ++k) {
          for (int j = 0; j < p + 1; ++j) {
            for (int i = 0; i < p + 1; ++i) {
              const auto mesh_index = valid_mesh_index(conn(index, 0, 0, 0, n))
                                        ? conn(index, k, j, i, n)
                                        : conn(index, k, j, i, 0);
              stk::simd::set_data(
                simd_element_field(index, k, j, i), n,
                field.get(mesh_index, 0));
            }
          }
        }
      }
    });
}
INSTANTIATE_POLYSTRUCT(stk_simd_scalar_field_gather_t);

template <int p>
void
stk_simd_vector_field_gather_t<p>::invoke(
  const_elem_mesh_index_view<p> conn,
  const stk::mesh::NgpField<double>& field,
  vector_view<p> simd_element_field)
{
  Kokkos::parallel_for(
    Kokkos::RangePolicy<exec_space, int>(0, conn.extent_int(0)),
    KOKKOS_LAMBDA(int index) {
      for (int n = 0; n < simd_len; ++n) {
        for (int k = 0; k < p + 1; ++k) {
          for (int j = 0; j < p + 1; ++j) {
            for (int i = 0; i < p + 1; ++i) {
              const auto mesh_index = valid_mesh_index(conn(index, 0, 0, 0, n))
                                        ? conn(index, k, j, i, n)
                                        : conn(index, k, j, i, 0);
              stk::simd::set_data(
                simd_element_field(index, k, j, i, 0), n,
                field.get(mesh_index, 0));
              stk::simd::set_data(
                simd_element_field(index, k, j, i, 1), n,
                field.get(mesh_index, 1));
              stk::simd::set_data(
                simd_element_field(index, k, j, i, 2), n,
                field.get(mesh_index, 2));
            }
          }
        }
      }
    });
}
INSTANTIATE_POLYSTRUCT(stk_simd_vector_field_gather_t);

template <int p>
void
stk_simd_face_scalar_field_gather_t<p>::invoke(
  const_face_mesh_index_view<p> conn,
  const stk::mesh::NgpField<double>& field,
  face_scalar_view<p> simd_element_field)
{
  Kokkos::parallel_for(
    Kokkos::RangePolicy<exec_space, int>(0, conn.extent_int(0)),
    KOKKOS_LAMBDA(int index) {
      for (int n = 0; n < simd_len; ++n) {
        for (int j = 0; j < p + 1; ++j) {
          for (int i = 0; i < p + 1; ++i) {
            const auto mesh_index = valid_mesh_index(conn(index, 0, 0, n))
                                      ? conn(index, j, i, n)
                                      : conn(index, j, i, 0);
            stk::simd::set_data(
              simd_element_field(index, j, i), n, field.get(mesh_index, 0));
          }
        }
      }
    });
}
INSTANTIATE_POLYSTRUCT(stk_simd_face_scalar_field_gather_t);

template <int p>
void
stk_simd_face_vector_field_gather_t<p>::invoke(
  const_face_mesh_index_view<p> conn,
  const stk::mesh::NgpField<double>& field,
  face_vector_view<p> simd_element_field)
{
  Kokkos::parallel_for(
    Kokkos::RangePolicy<exec_space, int>(0, conn.extent_int(0)),
    KOKKOS_LAMBDA(int index) {
      for (int n = 0; n < simd_len; ++n) {
        for (int j = 0; j < p + 1; ++j) {
          for (int i = 0; i < p + 1; ++i) {
            const auto mesh_index = valid_mesh_index(conn(index, 0, 0, n))
                                      ? conn(index, j, i, n)
                                      : conn(index, j, i, 0);
            stk::simd::set_data(
              simd_element_field(index, j, i, 0), n, field.get(mesh_index, 0));
            stk::simd::set_data(
              simd_element_field(index, j, i, 1), n, field.get(mesh_index, 1));
            stk::simd::set_data(
              simd_element_field(index, j, i, 2), n, field.get(mesh_index, 2));
          }
        }
      }
    });
}
INSTANTIATE_POLYSTRUCT(stk_simd_face_vector_field_gather_t);
} // namespace impl
} // namespace matrix_free
} // namespace nalu
} // namespace sierra
namespace sierra {
namespace nalu {
namespace matrix_free {

void
stk_simd_scalar_node_gather(
  const_node_mesh_index_view conn,
  const stk::mesh::NgpField<double>& field,
  node_scalar_view simd_node_field)
{
  Kokkos::parallel_for(
    Kokkos::RangePolicy<exec_space, int>(0, conn.extent_int(0)),
    KOKKOS_LAMBDA(int index) {
      for (int n = 0; n < simd_len; ++n) {
        const auto simd_mesh_index = conn(index, n);
        const auto mesh_index =
          valid_mesh_index(simd_mesh_index) ? simd_mesh_index : conn(index, 0);
        stk::simd::set_data(
          simd_node_field(index), n, field.get(mesh_index, 0));
      }
    });
}
} // namespace matrix_free
} // namespace nalu
} // namespace sierra
