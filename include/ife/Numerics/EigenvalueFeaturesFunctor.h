#ifndef __EigenvalueFeaturesFunctor_h
#define __EigenvalueFeaturesFunctor_h

#include "Symmetric3x3EigenvalueSolver.h"
#include <cmath>

/* Calculate combinations of eigenvalues
 */
template< typename TRealType >
struct EigenvalueFeaturesFunctor :
  public Symmetric3x3EigenvalueSolver<TRealType>
{
  typedef Symmetric3x3EigenvalueSolver<TRealType> SuperClass;
  typedef typename SuperClass::InputType InputType;
  typedef typename SuperClass::OutputType OutputType;
  
  EigenvalueFeaturesFunctor(){};
  ~EigenvalueFeaturesFunctor(){};
  
  inline OutputType operator()(const InputType &A) const {
    assert(A.Size() == 6);
    auto ev = SuperClass::operator()(A);
    OutputType features(6);
    features[0] = ev[0];
    features[1] = ev[1];
    features[2] = ev[2];
    features[3] = ev[0] + ev[1] + ev[2];
    features[4] = ev[0] * ev[1] * ev[2];
    features[5] = std::sqrt(ev[0]*ev[0] + ev[1]*ev[1] + ev[2]*ev[2]);
    return features;
  }
};

#endif
