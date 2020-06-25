#ifndef HYPRE_CUDA_LINEAR_SYSTEM_ASSEMBLER_H
#define HYPRE_CUDA_LINEAR_SYSTEM_ASSEMBLER_H

#include "HypreCudaAssembler.h"

#ifdef KOKKOS_ENABLE_CUDA

namespace sierra {
namespace nalu {

class MemoryPool {

public:

  /**
   * MemoryPool Constructor controls the allocation of temporary memory, which can be
   *  shared between Matrix and Rhs assembles
   *
   * @param name of the linear system being assembled
   * @param N the amount of memory to allocate
   * @param rank the MPI rank 
   */
  MemoryPool(std::string name, HypreIntType N, int rank);

  /**
   *  Destructor 
   */
  virtual ~MemoryPool();

  /**
   * memoryInGBS computes the amount of device memory used in GBs
   *
   * @return the amount of device memory used in GBs
   */
  double memoryInGBs() const;

  /**
   * deviceMemoryInGBS gets the amount of free and total memory in GBs
   *
   * @param free the amount of free memory in GBs
   * @param total the amount of total memory in GBs
   */
  void deviceMemoryInGBs(double & free, double & total) const;

  /**
   * get_d_workspace gets a pointer in device memory to the workspace
   *
   * @return a pointer in device memory to the workspace
   */
  HypreIntType * get_d_workspace() { return _d_workspace; }

protected:

private:

  /* amount of memory being used */
  std::string _name="";
  HypreIntType _memoryUsed=0;
  HypreIntType _N=0;
  int _rank=0;

  /* Cuda pointers and allocations for temporaries */
  HypreIntType * _d_workspace=NULL;

};



class MatrixAssembler {

public:

  /**
   * MatrixAssembler Constructor 
   *
   * @param sort whether or not to sort the CSR matrix (prior to full assembly) based on the element ids AND the values
   * @param iLower the first row
   * @param iUpper the ending row (inclusive)
   * @param jLower the first column
   * @param jUpper the ending column (inclusive)
   * @param global_num_rows the number of rows in the global matrix
   * @param global_num_cols the number of columns in the global matrix
   * @param nDataPtsToAssemble the number of data points to assemble into a CSR matrix
   * @param rank the mpi rank
   * @param num_rows the number of rows in the kokkos data structure
   * @param kokkos_row_indices Kokkos (device) pointer for the row coordinates
   * @param kokkos_row_start Kokkos (device) pointer to the start of the row inside (rows, cols, data) structures
   */
  MatrixAssembler(std::string name, bool sort, HypreIntType iLower, HypreIntType iUpper, HypreIntType jLower, HypreIntType jUpper,
		  HypreIntType global_num_rows, HypreIntType global_num_cols, HypreIntType nDataPtsToAssemble, int rank,
		  HypreIntType num_rows, HypreIntType * kokkos_row_indices, HypreIntType * kokkos_row_start);

  /**
   *  Destructor 
   */
  virtual ~MatrixAssembler();

  /**
   * memoryInGBS computes the amount of device memory used in GBs
   *
   * @return the amount of device memory used in GBs
   */
  double memoryInGBs() const;

  /**
   * deviceMemoryInGBS gets the amount of free and total memory in GBs
   *
   * @param free the amount of free memory in GBs
   * @param total the amount of total memory in GBs
   */
  void deviceMemoryInGBs(double & free, double & total) const;

  /**
   * getHasShared gets whether or not this has a shared matrix
   *
   * @return whether or not this has a shared matrix
   */
  bool getHasShared() const { return _has_shared; }

  /**
   * getNumRows gets the number of rows in both the owned and shared parts
   *
   * @return the number of rows in both the owned and shared parts
   */
  HypreIntType getNumRows() const { return _num_rows; }

  /**
   * getNumRowsOwned gets the number of rows in the owned part
   *
   * @return the number of rows in the owned part
   */
  HypreIntType getNumRowsOwned() const { return _num_rows_owned; }

  /**
   * getNumRowsShared gets the number of rows in the shared part
   *
   * @return the number of rows in the shared part
   */
  HypreIntType getNumRowsShared() const { return _num_rows_shared; }

  /**
   * getNumNonzeros gets the number of nonzeros in both the owned and shared parts
   *
   * @return the number of nonzeros in both the owned and shared parts
   */
  HypreIntType getNumNonzeros() const { return _num_nonzeros; }

  /**
   * getNumNonzerosOwned gets the number of nonzeros in the owned part
   *
   * @return the number of nonzeros in the owned part
   */
  HypreIntType getNumNonzerosOwned() const { return _num_nonzeros_owned; }

  /**
   * getNumNonzerosShared gets the number of nonzeros in the shared part
   *
   * @return the number of nonzeros in he shared part
   */
  HypreIntType getNumNonzerosShared() const { return _num_nonzeros_shared; }

  /**
   * setTemporaryDataArrayPtrs this function assigns pointers to the temporary data arrays needed in assembly
   *
   * @param d_workspace pointer to the work space 
   */
  void setTemporaryDataArrayPtrs(HypreIntType * d_workspace);

  /**
   * copyCSRMatrixToHost copies the assembled CSR matrix to the host (page locked memory)
   */
  void copyCSRMatrixToHost();

  /**
   * copyOwnedCSRMatrixToHost copies the assembled owned CSR matrix to the host (page locked memory)
   */
  void copyOwnedCSRMatrixToHost();

  /**
   * copySharedCSRMatrixToHost copies the assembled shared CSR matrix to the host (page locked memory)
   */
  void copySharedCSRMatrixToHost();

  /**
   * assemble : assemble the symbolic and numeric parts of the CSR matrix
   *
   * @param cols host pointer for the column coordinates
   * @param data host pointer for the data values
   *
   */
  void assemble(Kokkos::View<HypreIntType *>& cols, Kokkos::View<double *>& data);

  /**
   * get the host row indices ptr in page locked memory
   *
   * @return the pointer to the host row indices
   */
  HypreIntType * getHostRowIndicesPtr() { return _h_row_indices; }

  /**
   * get the host row counts ptr in page locked memory
   *
   * @return the pointer to the host row counts
   */
  HypreIntType * getHostRowCountsPtr() { return _h_row_counts; }

  /**
   * get the host column indices ptr in page locked memory
   *
   * @return the pointer to the host column indices 
   */
  HypreIntType * getHostColIndicesPtr() { return _h_col_indices; }

  /**
   * get the host values ptr in page locked memory
   *
   * @return the pointer to the host values
   */
  double * getHostValuesPtr() { return _h_values; }

  /**
   * get the host owned row indices ptr in page locked memory
   *
   * @return the pointer to the host owned row indices
   */
  HypreIntType * getHostOwnedRowIndicesPtr() { return _h_row_indices_owned; }

  /**
   * get the host owned row counts ptr in page locked memory
   *
   * @return the pointer to the host owned row counts
   */
  HypreIntType * getHostOwnedRowCountsPtr() { return _h_row_counts_owned; }
  
  /**
   * get the host owned column indices ptr in page locked memory
   *
   * @return the pointer to the host owned column indices 
   */
  HypreIntType * getHostOwnedColIndicesPtr() { return _h_col_indices_owned; }

  /**
   * get the host owned values ptr in page locked memory
   *
   * @return the pointer to the host owned values
   */
  double * getHostOwnedValuesPtr() { return _h_values_owned; }

  /**
   * get the host shared row indices ptr in page locked memory
   *
   * @return the pointer to the host shared row indices
   */
  HypreIntType * getHostSharedRowIndicesPtr() { return _h_row_indices_shared; }

  /**
   * get the host shared row counts ptr in page locked memory
   *
   * @return the pointer to the host shared row counts
   */
  HypreIntType * getHostSharedRowCountsPtr() { return _h_row_counts_shared; }
  
  /**
   * get the host shared column indices ptr in page locked memory
   *
   * @return the pointer to the host shared column indices 
   */
  HypreIntType * getHostSharedColIndicesPtr() { return _h_col_indices_shared; }

  /**
   * get the host shared values ptr in page locked memory
   *
   * @return the pointer to the host shared values
   */
  double * getHostSharedValuesPtr() { return _h_values_shared; }

protected:

private:

  /* cuda timers */
  cudaEvent_t _start, _stop;
  float _assembleTime=0.f;
  float _xferTime=0.f;
  float _xferHostTime=0.f;
  int _nAssemble=0;
  
  /* amount of memory being used */
  HypreIntType _memoryUsed=0;

  /* the kokkos pointers */
  HypreIntType * _d_kokkos_row_indices=NULL;
  HypreIntType * _d_kokkos_row_start=NULL;

  /* The final csr matrix pointers */
  HypreIntType _num_rows=0;
  HypreIntType _num_nonzeros=0;
  HypreIntType * _d_row_indices=NULL;
  unsigned long long int * _d_row_counts=NULL;
  HypreIntType * _d_col_indices=NULL;
  double *_d_values=NULL;
  /* host pointers in page locked memory */
  HypreIntType * _h_row_indices=NULL;
  HypreIntType * _h_row_counts=NULL;
  HypreIntType * _h_col_indices=NULL;
  double *_h_values=NULL;

  /* owned CSR matrix */
  HypreIntType _num_rows_owned=0;
  HypreIntType _num_nonzeros_owned=0;  
  HypreIntType * _d_row_indices_owned=NULL;
  unsigned long long int * _d_row_counts_owned=NULL;
  HypreIntType * _d_col_indices_owned=NULL;
  double *_d_values_owned=NULL;
  /* host pointers in page locked memory */
  HypreIntType * _h_row_indices_owned=NULL;
  HypreIntType * _h_row_counts_owned=NULL;
  HypreIntType * _h_col_indices_owned=NULL;
  double *_h_values_owned=NULL;

  /* shared (not owned) CSR matrix */
  HypreIntType _num_rows_shared=0;
  HypreIntType _num_nonzeros_shared=0;  
  HypreIntType * _d_row_indices_shared=NULL;
  unsigned long long int * _d_row_counts_shared=NULL;
  HypreIntType * _d_col_indices_shared=NULL;
  double *_d_values_shared=NULL;
  /* host pointers in page locked memory */
  HypreIntType * _h_row_indices_shared=NULL;
  HypreIntType * _h_row_counts_shared=NULL;
  HypreIntType * _h_col_indices_shared=NULL;
  double *_h_values_shared=NULL;

  /* meta data */
  std::string _name="";
  bool _sort=false;
  HypreIntType _iLower=0;
  HypreIntType _iUpper=0;
  HypreIntType _jLower=0;
  HypreIntType _jUpper=0;
  HypreIntType _global_num_rows=0;
  HypreIntType _global_num_cols=0;
  HypreIntType _num_rows_this_rank=0;
  HypreIntType _num_cols_this_rank=0;
  HypreIntType _nDataPtsToAssemble=0;
  HypreIntType _nBogusPtsToIgnore=0;
  int _rank=0;
  bool _col_index_determined=false;
  bool _csrMatMemoryAdded=false;
  bool _has_shared=false;

  /* Cuda pointers and allocations for temporaries */
  HypreIntType * _d_cols=NULL;
  double * _d_data=NULL;
  HypreIntType * _d_workspace=NULL;
};



class RhsAssembler {

public:

  /**
   * RhsAssembler Constructor 
   *
   * @param name of the linear system being assembled
   * @param sort whether or not to sort the rhs vector (prior to full assembly) based on the element ids AND the values
   * @param iLower the first row
   * @param iUpper the ending row (inclusive)
   * @param global_num_rows the number of rows in the global matrix
   * @param nDataPtsToAssemble the number of data points to assemble into a rhs vector
   * @param rank the mpi rank
   * @param num_rows the number of rows in the kokkos data structure
   * @param kokkos_row_indices Kokkos (device) pointer for the row coordinates
   * @param kokkos_row_start Kokkos (device) pointer to the start of the row insides (rows, data) structures
   */
  RhsAssembler(std::string name, bool sort, HypreIntType iLower, HypreIntType iUpper,
	       HypreIntType global_num_rows, HypreIntType nDataPtsToAssemble, int rank,
	       HypreIntType num_rows, HypreIntType * kokkos_row_indices, HypreIntType * kokkos_row_start);

  /**
   *  Destructor 
   */
  virtual ~RhsAssembler();

  /**
   * memoryInGBS computes the amount of device memory used in GBs
   *
   * @return the amount of device memory used in GBs
   */
  double memoryInGBs() const;

  /**
   * deviceMemoryInGBS gets the amount of free and total memory in GBs
   *
   * @param free the amount of free memory in GBs
   * @param total the amount of total memory in GBs
   */
  void deviceMemoryInGBs(double & free, double & total) const;

  /**
   * getHasShared gets whether or not this has a shared matrix
   *
   * @return whether or not this has a shared matrix
   */
  bool getHasShared() const { return _has_shared; }

  /**
   * getNumRows gets the number of rows in both the owned and shared parts
   *
   * @return the number of rows in both the owned and shared parts
   */
  HypreIntType getNumRows() const { return _num_rows; }

  /**
   * getNumRowsOwned gets the number of rows in the owned part
   *
   * @return the number of rows in the owned part
   */
  HypreIntType getNumRowsOwned() const { return _num_rows_owned; }

  /**
   * getNumRowsShared gets the number of rows in the shared part
   *
   * @return the number of rows in the shared part
   */
  HypreIntType getNumRowsShared() const { return _num_rows_shared; }

  /**
   * setTemporaryDataArrayPtrs this function assigns pointers to the temporary data arrays needed in assembly
   *
   * @param d_workspace pointer to the work space 
   */
  void setTemporaryDataArrayPtrs(HypreIntType * d_workspace);

  /**
   * copyRhsVectorToHost copies the assembled RhsVector to the host (page locked memory)
   */
  void copyRhsVectorToHost();

  /**
   * copyOwnedRhsVectorToHost copies the assembled owned RhsVector to the host (page locked memory)
   */
  void copyOwnedRhsVectorToHost();

  /**
   * copySharedRhsVectorToHost copies the assembled shared RhsVector to the host (page locked memory)
   */
  void copySharedRhsVectorToHost();

  /**
   * assemble : assemble the symbolic and numeric parts of the CSR matrix
   *
   */
  void assemble(Kokkos::View<double **>& data, const int index);

  /**
   * get the host rhs ptr in page locked memory
   *
   * @return the pointer to the host rhs
   */
  double * getHostRhsPtr() { return _h_rhs; }

  /**
   * get the host rhs indices ptr in page locked memory
   *
   * @return the pointer to the host rhs indices
   */
  HypreIntType * getHostRhsIndicesPtr() { return _h_rhs_indices; }

  /**
   * get the host owned rhs ptr in page locked memory
   *
   * @return the pointer to the host owned rhs
   */
  double * getHostOwnedRhsPtr() { return _h_rhs_owned; }

  /**
   * get the host owned rhs indices ptr in page locked memory
   *
   * @return the pointer to the host owned rhs indices
   */
  HypreIntType * getHostOwnedRhsIndicesPtr() { return _h_rhs_indices_owned; }

  /**
   * get the host shared rhs ptr in page locked memory
   *
   * @return the pointer to the host shared rhs
   */
  double * getHostSharedRhsPtr() { return _h_rhs_shared; }

  /**
   * get the host shared rhs indices ptr in page locked memory
   *
   * @return the pointer to the host shared rhs indices
   */
  HypreIntType * getHostSharedRhsIndicesPtr() { return _h_rhs_indices_shared; }

protected:

private:

  /* cuda timers */
  cudaEvent_t _start, _stop;
  float _assembleTime=0.f;
  float _xferTime=0.f;
  float _xferHostTime=0.f;
  int _nAssemble=0;

  /* amount of memory being used */
  HypreIntType _memoryUsed=0;

  /* The final rhs vector */
  HypreIntType _num_rows=0;
  double *_d_rhs=NULL;
  HypreIntType *_d_rhs_indices=NULL;
  double *_h_rhs=NULL;
  HypreIntType *_h_rhs_indices=NULL;

  /* The owned rhs vector */
  HypreIntType _num_rows_owned=0;
  double *_d_rhs_owned=NULL;
  HypreIntType *_d_rhs_indices_owned=NULL;
  double *_h_rhs_owned=NULL;
  HypreIntType *_h_rhs_indices_owned=NULL;

  /* The shared rhs vector */
  HypreIntType _num_rows_shared=0;
  double *_d_rhs_shared=NULL;
  HypreIntType *_d_rhs_indices_shared=NULL;
  double *_h_rhs_shared=NULL;
  HypreIntType *_h_rhs_indices_shared=NULL;

  /* meta data */
  std::string _name="";
  bool _sort=false;
  HypreIntType _iLower=0;
  HypreIntType _iUpper=0;
  HypreIntType _global_num_rows=0;
  HypreIntType _num_rows_this_rank=0;
  HypreIntType _nDataPtsToAssemble=0;
  int _rank=0;
  bool _has_shared=false;

  /* Cuda pointers and allocations for temporaries */
  HypreIntType * _d_kokkos_row_indices=NULL;
  HypreIntType * _d_kokkos_row_start=NULL;
  double * _d_data=NULL;
  HypreIntType * _d_workspace=NULL;
};

}  // nalu
}  // sierra

#endif

#endif /* HYPRE_CUDA_LINEAR_SYSTEM_ASSEMBLER_H */
