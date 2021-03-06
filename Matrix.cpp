#include "Matrix.h"
#include <iostream>
#include <fstream>
#include <math.h>

using namespace std;

Matrix::Matrix(void)
{
	triangle = false;
	r = 0;
	m = 0;
}


void Matrix::resize(int x, int y)
{
	r = x;
	m = y;
	n = x*y;
	if ((type && n > 0) || (type == 0 && n > 0)) // Buffer or real vector
	{
		//if (CBLAS)
		//	data = (double*)mkl_realloc(data, sizeof(double)*n);
		//else
			data = (double*)realloc(data, sizeof(double)*n);
		if (type == 0) //Changes if it is resized. 
			type = 1;
	}

	if (n == 0)
		type = 0; // Abstract

}


Matrix::~Matrix(void)
{
	// Vector destructor takes care of it
}

Matrix::Matrix(int r,int m,int real) : Vector(r*m,real)
{
	this->r = r;
	this->m = m;
	triangle = false;
}


Matrix::Matrix(int r) : Vector(r*r)
{
	this->r = r;
	this->m = r;
	triangle = false;
}

Matrix::Matrix(double* data, int d) :Vector(data, d*d) {

}




void Matrix::zero()
{
	memset(data,0,r*m*sizeof(double));
}

void Matrix::eye()
{
	memset(data, 0, r*m*sizeof(double));
	int s = r < m ? r : m;
	for (auto i = 0; i < s; i++)
		data[i*m + i] = 1;
}

void Matrix::readMatrix(const char* filename)
{
	ifstream file(filename);
	int i,j;
	if (!file.is_open())
		printf("Could not operfile...\n");
	else
	{
		printf("Reading %s...\n",filename);
		file >> r >> m;
		n = r*m;
		//if (CBLAS)
		//	data = (double*) mkl_malloc(sizeof(double)*r*m,64);
		//else
			data = (double*)malloc(sizeof(double)*r*m);
		type = 1;
		for(i=0;i<r;i++)
			for(j=0;j<m;j++)
				file >> data[i*m+j];
		file.close();
	}
}

void Matrix::writeMatrix(const char* filename)
{
	ofstream file(filename);
	int i,j;
	file << r << " " << m << endl;
	for (i=0;i<r;i++)
	{
		for(j=0;j<m;j++)
			file << data[i*m+j] << " ";
		file << endl;
	}
	file.close();
}


void Matrix::readBin(string filename)
{
	ifstream likefile(filename, ios::in| ios::binary);
	likefile >> *this;
	likefile.close();
}

void Matrix::writeBin(string filename)
{
	ofstream likefile(filename, ios::out | ios::binary);
	likefile << *this;
	likefile.close();
}


/* Get Row */
Vector& Matrix::operator[](int i){
	absbuffer.get().data = data + m*i;
	absbuffer.get().n = m;
	return absbuffer.next();
}

Vector& Matrix::operator()(int i) {
	absbuffer.get().data = data + m*i;
	absbuffer.get().n = m;
	return absbuffer.next();
}



double& Matrix::operator()(int i,int j) {
	return data[i*m + j];
}

double Matrix::sumlogdiag()
{
	int d = r<m?r:m; //Smaller
	int i;
	double sum=0;
	for(i=0;i<d;i++)
		sum += ::log(data[i+m*i]);
	return sum;
}


/* Cholesky decomposition of square matrix */
Matrix& Matrix::chol()
{
	double s = 0;
	int i,j,k;
	Matrix& mat = matbuffer.get();
	// double debug;
	mat.zero();
    for ( i = 0; i < m ; i++)
	{
        for ( j = 0; j < i; j++) {
            for (s=0,k = 0; k < j; k++)
                s += mat.data[i* m  + k] * mat.data[j* m  + k];
            mat.data[i* m  + j] = ( (data[i* m  + j] - s) / mat.data[j* m  + j] ) ;
//		if (mat.data[i*m + j]!=mat.data[i*m + j])
//			printf("ERROR Matrix is singular\n");
        }

	for (s=0,k = 0; k < i; k++)
		s += mat.data[i* m  + k] * mat.data[i* m  + k];
	mat.data[i*m + i] = ::sqrt(data[i* m  + i] - s);


	}


	mat.triangle = true;
	return matbuffer.next();
}


Matrix & Matrix::chol(Vector x)
{
	Matrix& L = matbuffer.get();
	L = *this;
	double r,c,s;
	if (!triangle) {
		printf("Error in Matrix::chol(Vector x): You can only do incremental updating when the matrix is already a decomposition.\n");
		exit(1);
	}
	for (int i = 0; i < x.n; i++) {
		r = ::sqrt(pow(L(i, i), 2) + pow(x[i], 2));
		c = r / L(i, i);
		s = x[i] / L(i, i);
		L(i, i) = r;
		for (int j = 1; j < x.n-i; j++) {
			/*L(i, i + j) = (L(i, i + j) + s*x(i + j)) / c;
			x.data[i + j] = c*x(i + j) - s*L(i, i + j);*/
			L(i + j, i) = (L(i + j, i) + s*x[i + j]) / c;
			x.data[i + j] = c*x[i + j] - s*L(i + j, i);
		}
	}

	return matbuffer.next();
}


Matrix Matrix::copy()
{
	Matrix res(r,m);
	res.n = n;
	memcpy(res.data, data, n * sizeof(double));
	return res;
}


Matrix& Matrix::qr()
{
	// Modified gram schimit
	double s = 0;
	int i, j;
	Matrix& Q = matbuffer.next();
	Matrix& R = matbuffer.next();
	Matrix v = this->transpose(); // Work on row  vectors
	Q.zero();
	R.zero();
	


	for (i = 0; i < r; i++)
	{

		for (j = 0; j < i; j++)
		{
			R(j,i) = Q(j)*v(i);
			v(i) = v(i) - Q(j) * R(j, i);
		}

		double normval = v(i).norm();
		R(i)[i] = normval;
		Q(i) = v(i) / normval;
	}

	return R;
}


Vector& Matrix::diag()
{
	Vector& vec = buffer.get();
	int lowdim = r < m ? r : m;
	for (auto i = 0; i < lowdim; i++)
		vec.data[i] = this->data[i*m + i];
	return buffer.next();
}

Vector& Matrix::mean()
{
	Vector& vec = buffer.get();
	vec.zero();
	for (auto i = 0; i < r; i++)
	for (auto j = 0; j < m; j++)
		vec.data[j] += this->data[i*m + j]/r;
	return buffer.next();
}


Matrix& Matrix::operator*(double scalar)
{
	int i;
	Matrix& mati = matbuffer.get();
	mati.m = m; mati.r = r; mati.triangle = triangle;
	for(i=0;i<n;i++)
		mati.data[i] = data[i]*scalar;
	return matbuffer.next();
}

Matrix& Matrix::transpose()
{
	Matrix mati = matbuffer.get();
	double res = 0;

	for (auto i = 0; i < r; i++)
		for (auto j = 0; j < m; j++) {
			mati.data[i*m + j] = data[j*m + i];
		}
	return matbuffer.next();
}

Matrix Matrix::transpose_xy()
{
	Matrix mati; mati.resize(m, r);
	for (auto i = 0; i < r; i++)
		for (auto j = 0; j < m; j++) {
			mati.data[j*r + i] = data[i*m + j];
		}
	return mati;
}

Matrix& Matrix::inverse()
{
	
	//Gaussian Elemination
	Matrix my(this->m);
	my = *this;
	Matrix&  inv = matbuffer.get();
	inv.eye();
	for (auto i = 0; i < r; i++) {
		inv(i) = inv(i) / my(i)[i];
		my(i) = my(i) / my(i)[i];
		for (auto j = 0; j < m; j++) {
			if (i != j) {
				inv(j) = inv(j) - inv(i)*my(j)[i];
				my(j) = my(j) - my(i)*my(j)[i];
			}
		}
	}
	return matbuffer.next();
}

Matrix& Matrix::scatter() // Calculate covariance of data matrix
{
	Matrix meansc = matbuffer.next(); // Allocate this matrix first
	Vector& v = this->mean();
	meansc = (v >> v)*r;
	Matrix mati = matbuffer.get();
	mati.zero();

	for (auto i = 0; i < r; i++) //
		for (auto j = 0; j < m; j++) {
			for (auto k = 0; k < m; k++) {
				mati.data[j*m + k] += data[i*m + j] * data[i*m + k];
			}
		}

	mati -= meansc;
	return matbuffer.next();
}

Matrix& Matrix::cov()
{
	return this->scatter() / (r - 1);
}


Matrix& Matrix::operator*(const Matrix& mat)
{
	int i;
	Matrix& mati = matbuffer.get();
	double res = 0;
	// Transposed first matrix, this may not be consistent in this version of the library would be refined later
	/*if (CBLAS) {
		cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans,
			this->r, mati.m, this->m, 1.0, this->data, this->m, mat.data, mati.m, 0.0, mati.data, mati.m);
	}
	else
	{*/
		for (i = 0; i < r; i++)
			for (auto j = 0; j < m; j++) {
				res = 0;
				for (auto k = 0; k < m; k++)
					res += mat.data[i*m + k] * data[k*m + j];
				mati.data[i*m + j] = res;
			}
	//}
	return matbuffer.next();
}

Vector& Matrix::operator*(const Vector& v)
{
	Vector& v2 = buffer.get();
	// Transposed first matrix, this may not be consistent in this version of the library would be refined later
	/*if (CBLAS) {
		cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans,
			this->r, 1, this->m, 1.0, this->data, this->m, v.data, 1, 0.0, v2.data, 1);
	}
	else
	{*/
		double res = 0;
		for (auto j = 0; j < m; j++) {
			res = 0;
			for (auto k = 0; k < m; k++)
				res += v.data[k] * data[k*m + j];
			v2.data[j] = res;
		}
		
	//}
	return buffer.next();
}



Matrix& Matrix::operator/(double scalar)
{
	int i;
	Matrix& mati = matbuffer.get();
	mati.m = m; mati.r = r; mati.triangle = triangle;
	double divone = 1.0/scalar;
	for(i=0;i<n;i++)
		mati.data[i] = data[i]*divone;
	return matbuffer.next();
}

Matrix& Matrix::operator+(Matrix& mat)
{
	int i;
	Matrix& mati = matbuffer.get();
	/*if (CBLAS) {
		cblas_dcopy(this->n, this->data, 1, mati.data, 1);
		cblas_daxpy(mati.n, 1.0, mat.data, 1, mati.data, 1);
	}
	else {*/
		for (i = 0; i < n; i++)
			mati.data[i] = data[i] + mat.data[i];
	//}
	return matbuffer.next();
}

Matrix& Matrix::operator-(Matrix& mat)
{
	int i;
	Matrix& mati = matbuffer.get();
	/*if (CBLAS) {
		cblas_dcopy(this->n, this->data, 1, mati.data, 1);
		cblas_daxpy(mati.n, -1.0, mat.data, 1, mati.data, 1);
	}
	else
	{*/
		for (i = 0; i < n; i++)
			mati.data[i] = data[i] - mat.data[i];
	//}
	return matbuffer.next();
}

void  Matrix::operator=(const Matrix& mat)
{
	Vector::operator=(mat);
	triangle = mat.triangle;
	m = mat.m;
	r = mat.r;
}

void Matrix::operator<=(const Matrix& mat) {
	n = mat.n;
	type = 0;
	data = mat.data;
	m = mat.m;
	r = mat.r;
}


Matrix Matrix::submat(int r1, int r2, int c1, int c2)
{

	if (r2 > r)
		r2 = r;
	if (c2 > m)
		c2 = m;

	int rows = r2 - r1;
	int cols = c2 - c1;

	if (rows < 0 || cols < 0)
		return NULLMAT;

	Matrix mat(rows, cols , 2); // Persistent memory use with caution
	for (auto i = 0; i < rows && ((i+r1) < r); i++)
		for (auto j = 0; j < cols && ((j+c1) < m); j++)
			mat.data[i*mat.m + j]= data[ (r1 + i)*m + c1 + j];
	return mat;
}



void Matrix::print()
{
	for (int i = 0; i < r; i++) {
		printf( "|");
		for (int j = 0; j < m; j++)
			printf(" %.2f", data[i*m + j]);
		printf("|\n");
	}
}

// ????
Matrix::Matrix(Matrix&& mat) : Vector(mat)
{
	triangle = mat.triangle;
	m = mat.m;
	r = mat.r;
}

Matrix::Matrix(const Matrix& mat) : Vector(mat) , r(mat.r) , m(mat.m) , triangle(mat.triangle)
{
}


ostream& operator<<(ostream& os, const Matrix& v)
{   // It does not save type , it save as real vector always
		os.write((char*)&v.r, sizeof(int));
		os.write((char*)&v.m, sizeof(int));
		os.write((char*)v.data, sizeof(double)*v.n);
		// os.write((char*) &v.triangle,sizeof(int));  // Not used greatly
		
	return os;
}

istream& operator>>(istream& is, Matrix& v)
{   // It does not save type , it save as real vector always
	// istream need to be binary
	
	is.read((char*)&v.r, sizeof(int));
	is.read((char*)&v.m, sizeof(int));
	v.resize(v.r,v.m);
	is.read((char*)v.data, sizeof(double)*v.n);
	v.type = 1; // Real
	return is;
}




Matrix eye(int d)
{ 
	Matrix m(d);
	m.eye();
	return m;
}

Matrix zeros(int d1, int d2)
{
	Matrix m(d1,d2);
	m.zero();
	return m;
}


// You can create a matrix using mat({{1,2,3},{1,3,3},{1,4,3}})
Matrix mat(std::initializer_list<std::initializer_list<double>> numbers) {

	int size = numbers.size();
	Matrix m(size, size); // To do : It only supports square matrices now
	int i = 0;
	for (auto nums : numbers)
		for (auto val : nums)
		{
			m.data[i] = val;
			i = i + 1;
		}
	return m;
}