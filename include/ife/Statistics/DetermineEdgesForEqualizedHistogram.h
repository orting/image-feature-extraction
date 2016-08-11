#ifndef __DetermineEdgesForEqualizedHistogram_h
#define __DetermineEdgesForEqualizedHistogram_h
/*
  The general assumption when doing histogram equalization is that we have a 
  discrete image that takes values in some range [a,b). That case is relatively
  simple to handle due to the discretization. We cannot make that assumption, 
  since our images can be real valued, e.g. eigenvalues of the Hessian. So we 
  need a procedure that does not rely on discretization. This is the reason for
  the rather complicated determineEdgesForEqualizedHistogram function.

  TODO: Handle the case where we are asked for more edges than we can produce

 */


#include <algorithm>
#include <exception>
#include <iterator>

// [first, last) should be sorted
template< typename InputIt, typename OutputIt >
void
determineEdgesForEqualizedHistogram(InputIt first,
				    InputIt last,
				    OutputIt d_first,
				    size_t nBins) {
  // Now we find the equalizing edges.
  // sort the samples and partition into equal sized blocks.
  // We need to handle the case where we have many samples that are equal
  auto n = std::distance(first, last);
  if ( n < 0 ) {
    throw std::logic_error("Iterator first must come before iterator last");
  }
  size_t nSamples = std::abs(n);
  
  if (nSamples < nBins) {
    throw std::out_of_range("Too many bins. Number of bins must be less or equal to number of samples");
  }

  size_t samplesPerBin = nSamples / nBins;
  size_t sampleSurplus = nSamples - samplesPerBin*nBins;
  size_t sampleDeficit = 0;
  size_t nEdge = 0;
  auto it = first;
  while (nEdge + 1 < nBins ) {
    auto index = samplesPerBin;

    // If we have a sample surplus/deficit we distribute it evenly on the 
    // remaining bins.
    if ( sampleSurplus ) {
      auto surplusForThisSample = sampleSurplus / (nBins - nEdge);
      if ( surplusForThisSample == 0 ) {
	// We bias the surplus on the first bins
	surplusForThisSample = 1;
      }
      index += surplusForThisSample;
      sampleSurplus -= surplusForThisSample;
    }
    else if ( sampleDeficit ) {
      auto deficitForThisSample = sampleDeficit / (nBins - nEdge);
      if ( deficitForThisSample == 0 ) {
	// We bias the deficit on the first bins
	deficitForThisSample = 1;
      }
      index -= deficitForThisSample;
      sampleDeficit -= deficitForThisSample;
    }

    // If we have unique value, we have the optimal edge. But otherwise we need
    // to make adjustments depending on how many duplicates there are.
    assert( std::distance( it, last ) > index );
    std::advance(it, index);

    // Figure out how many samples have the same value    
    // Find first element not less than sample. Since the range [first, it) is
    // sorted, we know that *lb == *it. If there are no elements equal to *it
    // we will get lb = it. If there are elements equal to it, we will get lb
    // pointing to the first of these.
    auto lb = std::lower_bound(first, it, *it);

    // If the lower bound is the element itself, we are done in this iteration.
    // Otherwise we need to figure out which element to use.
    if ( lb != it ) {
      // Find first element greater than sample
      auto ub = std::upper_bound(it, last, *it);

      if ( ub == last ) {
	// All remaining values are equal. Since we define bins as [e_i, e_i+1)
	// with a rightmost phantom edge at infinity it only makes sense to set
	// the last edge to the lower bound.
	it = lb;
      }
      else {
	// We need to check if it is better to lb or ub
	// We have *lb == *e, but never *e == *ub.
	// Two edges, e1 < e2, define a bin containing the range (e1,e2]
	// Setting e = lb decreases the number of samples by distance(lb, e)
	// Setting e = ub increases the number of samples by distance(e, ub)
	auto d = std::distance( lb, it );
	assert(d >= 0);
	size_t lbdist = std::abs(d);

	d =  std::distance( it, ub );
	assert(d >= 0);
	size_t ubdist = std::abs(d);
	
	// We have two options.
	//  1. Take lbdist and get too few samples
	//  2. Take ubdist and get too many samples
	// Since we have already incorporated surplus/deficit from the
	// previous samples, I think we should make the closest choice.
	// If the distance is equal, we should let deficit/surplus determine it
	if (lbdist < ubdist || (lbdist == ubdist && sampleDeficit) ) {
	  it = lb;
	  if ( lbdist > sampleDeficit ) {
	    sampleSurplus = lbdist - sampleDeficit;
	    sampleDeficit = 0;
	  }
	  else {
	    sampleDeficit -= lbdist;
	  }
	}
	else {
	  it = ub;
	  if ( ubdist > sampleSurplus ) {
	    sampleDeficit = ubdist - sampleSurplus;
	    sampleSurplus = 0;
	  }
	  else {
	    sampleSurplus -= ubdist;
	  }
	}
      }
    }
    // We have an edge.
    *d_first++ = *it;
    ++nEdge;
  }
}  
#endif
