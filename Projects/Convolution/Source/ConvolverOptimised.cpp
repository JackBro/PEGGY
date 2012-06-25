#if !defined NO_REFERENCE_OPT

#include "ConvolverOptimised.h"

// cons
	ConvolverOptimised::
	ConvolverOptimised() :
		ConvolverReference()
{
	SetName("Opt ");
}

void
	ConvolverOptimised::
	Execute() throw (...)
{
	if (GetWidth() < 2 * GetRadius() || GetHeight() < 2 * GetRadius())
	{
		// Run the parent execution function, this one can't handle small data
		// at all without sacrificing the optimisations.
		ConvolverReference::Execute();
		return;
	}
	//DoTraceMessage(TIMING, "%s%s%s", "Start (", GetName(), "): Set");
	Log("Set");
	const size_t
		height = GetHeight(),
		width = GetWidth(),
		radius = GetRadius(),
		hm1 = height - 1,
		wm1 = width - 1;
	// Create a local summed filter for faster processing and save the mid point
	// of the global filter so the loops can access the right data in order.
	float
		* nf = new float [radius],
		* filter = &m_filter[radius];
	for (size_t i = 0; i != radius; ++i)
	{
		nf[i] = m_filter[0];
		for (size_t j = radius - i; j != 0; --j)
		{
			nf[i] += m_filter[j];
		}
	}
	// Do the X convolution
	//DoTraceMessage(TIMING, "%s%s%s", "End (", GetName(), "): Set");
	End("Set");
	//DoTraceMessage(TIMING, "%s%s%s", "Start (", GetName(), "): X");
	Log("X");
	for (size_t i = 0; i != height; ++i)
	{
		// In this code I and j appear to maintain the same values, e.g.
		// they're both incremented at the same time, but the important thing
		// to remember is that j is the relative position in the current row,
		// wheras I is the absolute position in the data array.
		size_t
			I = i * width,
			j = 1,
			T = i;
		float
			minv = m_data[I],
			maxv = m_data[I + wm1],
			sum = (minv * nf[0]);
		// Now do the downward slope.
		for (size_t u = 0; u != radius; )
		{
			++u;
			sum += m_data[I + u] * filter[u];
		}
		// Set the total.
		m_smoothX[T] = sum;
		// Transpose the output so we can read in the data in exactly the same
		// way for doing the vertical half, which should be better in terms of
		// cache access.
		T += height;
		++I;
		while (j != radius)
		{
			// Do border cases where the apron needs clamping at the bottom.
			// This code generates a new array in which every element is the
			// sum of all the kernel values in indexes lower than the current,
			// this means we don't need any loop at all here.  It also combines
			// the first real element in to this multiplication for a bit of
			// extra speed.
			sum = (minv * nf[j]) + (m_data[I] * filter[0]);
			// Would be interesting to know what effect cache misses have on
			// this code for rediculously big filter sizes as the two addresses
			// being read may be quite far apart.  This code does all the PAIRS
			// of items within real data, i.e. for every filter value there are
			// two bits of data which use that value, do those pairs here.
			// Note that this code doesn't work if the data size is less than
			// twice the filter radius.
			for (size_t u = 1; u != j; ++u)
			{
				sum += (m_data[I - u] + m_data[I + u]) * filter[u];
			}
			// Now do the downward slope.  This code is very slightly different
			// to the code outside the loop.  Due to the way this all works,
			// the first two times both have an initial "u" value of 0.
			for (size_t u = j - 1; u != radius; )
			{
				++u;
				sum += m_data[I + u] * filter[u];
			}
			// Set the total.
			m_smoothX[T] = sum;
			T += height;
			++j;
			++I;
		}
		// We have done the low end border cases, now do the middle complete
		// values, this is the simplest bit of code as there are no awkward
		// overflows to deal with.
		for (size_t end = width - radius; j != end; ++j)
		{
			sum = (m_data[I] * filter[0]);
			for (size_t u = 0; u != radius; )
			{
				++u;
				sum += (m_data[I - u] + m_data[I + u]) * filter[u];
			}
			m_smoothX[T] = sum;
			T += height;
			++I;
		}
		// Now do the end border cases, this is the reverse of the start border
		// case code (sort of, it's done in the same order though).
		for (size_t rev = radius - 1; j != wm1; ++j)
		{
			sum = (maxv * nf[rev]) + (m_data[I] * filter[0]);
			--rev;
			for (size_t u = 0; u != rev; )
			{
				++u;
				sum += (m_data[I - u] + m_data[I + u]) * filter[u];
			}
			for (size_t u = rev; u != radius; )
			{
				++u;
				sum += m_data[I - u] * filter[u];
			}
			m_smoothX[T] = sum;
			T += height;
			++I;
		}
		// The very last item!
		sum = (maxv * nf[0]);
		for (size_t u = 0; u != radius; )
		{
			++u;
			sum += m_data[I - u] * filter[u];
		}
		m_smoothX[T] = sum;
	}
	//DoTraceMessage(TIMING, "%s%s%s", "End (", GetName(), "): X");
	End("X");
	//DoTraceMessage(TIMING, "%s%s%s", "Start (", GetName(), "): Y");
	Log("Y");
	// Do the Y convolution, because of the transposition in the code above,
	// this is VERY similar, just a few variables swapped, there's no need for
	// extra code to calculate exact indices.
	for (size_t i = 0; i != width; ++i)
	{
		size_t
			I = i * height,
			j = 1,
			T = i;
		float
			minv = m_smoothX[I],
			maxv = m_smoothX[I + hm1],
			sum = (minv * nf[0]);
		for (size_t u = 0; u != radius; )
		{
			++u;
			sum += m_smoothX[I + u] * filter[u];
		}
		m_smoothY[T] = sum;
		T += width;
		++I;
		while (j != radius)
		{
			sum = (minv * nf[j]) + (m_smoothX[I] * filter[0]);
			for (size_t u = 1; u != j; ++u)
			{
				sum += (m_smoothX[I - u] + m_smoothX[I + u]) * filter[u];
			}
			for (size_t u = j - 1; u != radius; )
			{
				++u;
				sum += m_smoothX[I + u] * filter[u];
			}
			m_smoothY[T] = sum;
			T += width;
			++j;
			++I;
		}
		for (size_t end = height - radius; j != end; ++j)
		{
			sum = (m_smoothX[I] * filter[0]);
			for (size_t u = 0; u != radius; )
			{
				++u;
				sum += (m_smoothX[I - u] + m_smoothX[I + u]) * filter[u];
			}
			m_smoothY[T] = sum;
			T += width;
			++I;
		}
		for (size_t rev = radius - 1; j != hm1; ++j)
		{
			sum = (maxv * nf[rev]) + (m_smoothX[I] * filter[0]);
			--rev;
			for (size_t u = 0; u != rev; )
			{
				++u;
				sum += (m_smoothX[I - u] + m_smoothX[I + u]) * filter[u];
			}
			for (size_t u = rev; u != radius; )
			{
				++u;
				sum += m_smoothX[I - u] * filter[u];
			}
			m_smoothY[T] = sum;
			T += width;
			++I;
		}
		sum = (maxv * nf[0]);
		for (size_t u = 0; u != radius; )
		{
			++u;
			sum += m_smoothX[I - u] * filter[u];
		}
		m_smoothY[T] = sum;
	}
	//DoTraceMessage(TIMING, "%s%s%s", "End (", GetName(), "): Y");
	End("Y");
	//DoTraceMessage(TIMING, "%s%s%s", "Start (", GetName(), "): Copy");
	Log("Copy");
	// Now store the data back to main memory.
	
	memcpy(GetStore(), m_smoothY, height * width * sizeof (float));
	delete [] nf;
	//DoTraceMessage(TIMING, "%s%s%s", "End (", GetName(), "): Copy");
	End("Copy");
}

#endif
