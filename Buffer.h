#pragma once
//#define thread_specific __declspec( thread )
//#define thread_local thread_local 
#include <vector>
//#include <mkl.h>
using namespace std;

// Altough optimization methods of C++ avoids unnecessary copies it is still not sufficient for temprorary objects
// This buffer creates a memory space for those calculations. I used templates to be generalizable. 
template <class T>
class Buffer
{
public:
	int buffersize;
	int dim;
	int bufferindex;
	int oldindex;
	vector<T> data;
	Buffer(int size,int d,int real=2)
	{
		buffersize = size;
		dim = d;
		data.reserve(buffersize);
		bufferindex = 0;
		for (int j=0;j<buffersize;j++)
		{

			data.emplace_back(dim);
			data[j].type = real;

			if (real == 0 && d>0)  // Altough it is not a good idea to first allocate then deallocate here , I don't want to change interface of constructors.
				//if (CBLAS)
				//	mkl_free(data[j].data);
				//else
					free(data[j].data);
		}
	}

	Buffer()
	{
		buffersize = dim = bufferindex = 0;
	}

	inline T& next()
	{
		oldindex = bufferindex;
		bufferindex = ( bufferindex + 1 ) % buffersize;
		return data[oldindex];
	}

	inline T& get()
	{
		return data[bufferindex];
	}

	inline double& operator[](const int i)
	{
		return data[bufferindex][i];
	}

	~Buffer(void)
	{
	}	
};


extern thread_local int threadid;
template <class T>
class MultiBuffer
{
public:
	
	static int nthreads;
	MultiBuffer(int nthreads,int buffersize,int dim,int real=2)
	{
		MultiBuffer::nthreads = nthreads;
		threadid = 0; // Master Thread
		data.reserve(nthreads);
		for (int i=0;i<nthreads;i++)
		{
			data.emplace_back(buffersize,dim,real);
		}
	}

	MultiBuffer()
	{
		nthreads = 0;
	}

	inline T& next()
	{
		return data[threadid].next();
	}

	inline T& get()
	{
		return data[threadid].get();
	}

	inline double& operator[](const int i)
	{
		return data[threadid][i];	
	}


	~MultiBuffer(void){
			
	}
	vector<Buffer<T> > data;
};

//template <class T>
//static int MultiBuffer<T>::threadid;
template <class T>
int MultiBuffer<T>::nthreads;