#include <fstream>
#include <iostream>
#include "kinematicsDLL.h"


char *buffer;
double *stackedQ;
double *stackedU;
double *stackedX;
double *index;
double nlArray[6];
double k11u[11], k11d[11], k110[11], k11[11];
double k50[5], k5u[5], k5d[5], k5[5];
double qpup[5], qpdn[5], qps0[5], qpupdn[10];
int ret, nrows;


void init() {
	//       tx01             ty01             tz01           ry01            rz01           ry34           rz34         kAlpha         eAlpha          lCath            ry45
	k11u[0] = 826; k11u[1] = -46.0; k11u[2] =  -8.0; k11u[3] =  .2; k11u[4] =  -.4; k11u[5] =  .2; k11u[6] =  .2; k11u[7] = 1.2; k11u[8] = 1.2; k11u[9] = 110; k11u[10] =  .2;
	k110[0] = 806; k110[1] = -66.0; k110[2] = -28.0; k110[3] =   0; k110[4] = -.24; k110[5] =   0; k110[6] =   0; k110[7] =   1; k110[8] =   1; k110[9] =  95; k110[10] =   0;
	k11d[0] = 786; k11d[1] = -86.0; k11d[2] = -28.0; k11d[3] = -.2; k11d[4] = -.44; k11d[5] = -.2; k11d[6] = -.2; k11d[7] =  .8; k11d[8] =  .8; k11d[9] =  90; k11d[10] = -.2;	

	//          tx01              ty01              tz01              ry01             lCath
	k5u[0] = k11u[0]; k5u[1] = k11u[1]; k5u[2] = k11u[2]; k5u[3] = k11u[3]; k5u[4] = k11u[9];
	k50[0] = k110[0]; k50[1] = k110[1]; k50[2] = k110[2]; k50[3] = k110[3]; k50[4] = k110[9];
	k5d[0] = k11d[0]; k5d[1] = k11d[1]; k5d[2] = k11d[2]; k5d[3] = k11d[3]; k5d[4] = k11d[9];

	qpup[0] = .5; qpup[1] = .3; qpup[2] = .3; qpup[3] = 1; qpup[4] = 10;
	qpdn[0] = -.5; qpdn[1] = -.3; qpdn[2] = -.3; qpdn[3] = 1e-3; qpdn[4] = -10;
	qpupdn[0] = -.5; // joint minima
	qpupdn[1] = -.3;
	qpupdn[2] = -.3;
	qpupdn[3] = 1e-3;
	qpupdn[4] = -10;
	qpupdn[5] = .5; // maxima
	qpupdn[6] = .3;
	qpupdn[7] = .3;
	qpupdn[8] =  1;
	qpupdn[9] = 10;

	//http://ab-initio.mit.edu/wiki/index.php/NLopt_Algorithms
	nlArray[0] = 1e5; // maxIts
	nlArray[1] = 6; // max time sec
	nlArray[3] = 1e-9; // min fun val
	nlArray[4] = 1e-9; // tol fun
	nlArray[5] = 1e-9; //tol x
	//nlArray[2] = 00; // GN_DIRECT
	//nlArray[2] = 01; // GN_DIRECT_L --locally biased
	//nlArray[2] = 03; // GN_DIRECT_L_RAND
	//nlArray[2] = 04; // GN_ESCH
	//nlArray[2] = 05; // GN_ISRES
	//nlArray[2] = 06; // GN_MLSL -- slow due to local searches
	//nlArray[2] = 07; // GN_MLSL_LDS
	//nlArray[2] = 12; // LN_BOBYQA
	nlArray[2] = 13; // LN_COBYLA
	//nlArray[2] = 14; // LN_NelderMead
	//nlArray[2] = 17; // LN_PRAXIS
	//nlArray[2] = 18; // LN_SUBPLX
}
void load(char *fname) {
	int ncols = 38; //it, q0,q1,q2,q3,q4, H1, H2

	//open
	printf("\n\nOpening %s\n", fname);
	std::fstream fs;
	fs.open(fname, std::fstream::in | std::fstream::binary); //name, read/write mode|binary
	if (!fs) { std::cerr << "warning, could not open " << fname << std::endl; return; };

	//fileSize & initialize variables
	fs.seekg(0, fs.end);
	long fileSize = (long)fs.tellg();
	nrows = fileSize / sizeof(double) / ncols;
	fs.seekg(0, fs.beg);
	printf("fileSize %d with %d rows\n", fileSize, nrows);

	buffer = new char[fileSize];
	stackedQ = new double[nrows * 5];
	stackedU = new double[nrows * 3];
	stackedX = new double[nrows * 3];
	index = new double[nrows];

	//read
	fs.read(buffer, fileSize);
	printf("read %d chars\n\n", (int)fs.gcount());
	fs.close();

	//extract -- Hs in row-major
	double *pd = reinterpret_cast<double*>(buffer); //buffer is a byte array, but we know it to be double data
													//0i 1q0 2q1 3q2 4q3 5q4 : 6ux 7vx 8wx 9x 10uy 11vy 12wy 13y 14uz 15vz 16wz 17z 180 190 200 211
	for (int irow = 0; irow < nrows; irow++) {
		index[irow] = *(pd + 0 + irow*ncols);
		stackedQ[irow * 5 + 0] = *(pd + 1 + irow*ncols);
		stackedQ[irow * 5 + 1] = *(pd + 2 + irow*ncols);
		stackedQ[irow * 5 + 2] = *(pd + 3 + irow*ncols);
		stackedQ[irow * 5 + 3] = *(pd + 4 + irow*ncols);
		stackedQ[irow * 5 + 4] = *(pd + 5 + irow*ncols);

		stackedU[irow * 3 + 0] = *(pd + 6 + irow*ncols);
		stackedU[irow * 3 + 1] = *(pd + 10 + irow*ncols);
		stackedU[irow * 3 + 2] = *(pd + 14 + irow*ncols);

		stackedX[irow * 3 + 0] = *(pd + 9 + irow*ncols);
		stackedX[irow * 3 + 1] = *(pd + 13 + irow*ncols);
		stackedX[irow * 3 + 2] = *(pd + 17 + irow*ncols);

		//printf("%+5.4f: q[%+5.4f %+5.4f %+5.4f %+5.4f %+5.4f] ", index[irow], stackedQ[irow * 5 + 0], stackedQ[irow * 5 + 1], stackedQ[irow * 5 + 2], stackedQ[irow * 5 + 3], stackedQ[irow * 5 + 4]);
		//printf("u[%+5.4f %+5.4f %+5.4f]  ", stackedU[irow * 3 + 0], stackedU[irow * 3 + 1], stackedU[irow * 3 + 2]);
		//printf("x[%+5.4f %+5.4f %+5.4f]\n", stackedX[irow * 3 + 0], stackedX[irow * 3 + 1], stackedX[irow * 3 + 2]);
	}
	/*for (int i = 0; i < 800; i = i+10) {
	//printf("i[%d]: q[%+5.4f] u[%+5.4f] x[%+5.4f]\n", i, stackedQ[i], stackedU[i], stackedX[i]);
	//printf("i[%d]: q[%+5.4f] u[%+5.4f] x[%+5.4f]\n", i, stackedQ[(int)fmod(i, 5)], stackedU[i], stackedX[i]);
	printf("%+5.4f: q[%+5.4f %+5.4f %+5.4f %+5.4f %+5.4f] ", index[i], stackedQ[i * 5 + 0], stackedQ[i * 5 + 1], stackedQ[i * 5 + 2], stackedQ[i * 5 + 3], stackedQ[i * 5 + 4]);
	printf("u[%+5.4f %+5.4f %+5.4f]  ", stackedU[i * 3 + 0], stackedU[i * 3 + 1], stackedU[i * 3 + 2]);
	printf("x[%+5.4f %+5.4f %+5.4f]\n", stackedX[i * 3 + 0], stackedX[i * 3 + 1], stackedX[i * 3 + 2]);
	}//*/
}

void check_qp0_xyz5a() {
	double fmin = 22;
	printf("\nqp0_xyz5a\n");
	for (int i = 0; i < 5; i++) { qps0[i] = .2; }
	//printf("qpup[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpup[i]); } printf("]\n");
	//printf("qps0[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qps0[i]); } printf("]\n");
	//printf("qpdn[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpdn[i]); } printf("]\n");
	ret = funIP_qp0_xyz5A(nrows, stackedQ, stackedX, k50, qps0, &fmin); printf("val0 = %f\n", fmin);
	ret = estimate_qp0_xyz5A(nrows, stackedQ, stackedX, k50, qps0, qpup, qpdn, nlArray, &fmin);
	printf("ret %d  fmin %f\n", ret, fmin);
	printf("qpup[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpup[i]); } printf("]\n");
	printf("qps0[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qps0[i]); } printf("]\n");
	printf("qpdn[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpdn[i]); } printf("]\n");
}

void check_qp0_xyzuxuyuz5a() {
	double fmin = 22;
	printf("\nqp0_xyzuxuyuz5a\n");
	for (int i = 0; i < 5; i++) { qps0[i] = .2; }
	//printf("qpup[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpup[i]); } printf("]\n");
	//printf("qps0[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qps0[i]); } printf("]\n");
	//printf("qpdn[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpdn[i]); } printf("]\n");
	ret = funIP_qp0_xyzuxuyuz5A(nrows, stackedQ, stackedX, stackedU, k50, qps0, &fmin); printf("val0 = %f\n", fmin);
	ret = estimate_qp0_xyzuxuyuz5A(nrows, stackedQ, stackedX, stackedU, k50, qps0, qpup, qpdn, nlArray, &fmin);
	printf("ret %d  fmin %f\n", ret, fmin);
	printf("qpup[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpup[i]); } printf("]\n");
	printf("qps0[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qps0[i]); } printf("]\n");
	printf("qpdn[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpdn[i]); } printf("]\n");
}

void check_kn0_xyz5a() {
	double fmin = 22;
	printf("\nkn0_xyz5a\n");
	for (int i = 0; i < 5; i++) { k5[i] = k50[i]; }
	ret = fun_kn0_xyz5A(nrows, stackedQ, stackedX, k5, &fmin); printf("val0 = %f\n", fmin);
	ret = estimate_kn0_xyz5A(nrows, stackedQ, stackedX, k5, k5u, k5d, nlArray, &fmin);
	printf("ret %d  fmin %f\n", ret, fmin);
	printf("k5u[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", k5u[i]); } printf("]\n");
	printf("k50[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", k5[i]); } printf("]\n");
	printf("k5d[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", k5d[i]); } printf("]\n");
}

void check_qp0kn0_xyz5a_flipflop() {
	double fmin = 22;
	printf("\nqp0kn0_xyz5a\n");
	qpup[0] = .5; qpup[1] = .3; qpup[2] = .3; qpup[3] = 1; qpup[4] = 10;
	qpdn[0] = -.5; qpdn[1] = -.3; qpdn[2] = -.3; qpdn[3] = 1e-3; qpdn[4] = -10;
	for (int i = 0; i < 5; i++) { qps0[i] = 0; }
	for (int i = 0; i < 10; i++) {
		ret = estimate_qp0_xyz5A(nrows, stackedQ, stackedX, k50, qps0, qpup, qpdn, nlArray, &fmin);
		for (int j = 0; j < nrows; j++) {
			stackedQ[j * 5 + 0] += qps0[0]; //+ inside the solver
			stackedQ[j * 5 + 1] += qps0[1];
			stackedQ[j * 5 + 2] += qps0[2];
			stackedQ[j * 5 + 3] += qps0[3];
			stackedQ[j * 5 + 4] += qps0[4];
		}
		ret = estimate_kn0_xyz5A(nrows, stackedQ, stackedX, k50, k5u, k5d, nlArray, &fmin);
		for (int j = 0; j < nrows; j++) {
			stackedQ[j * 5 + 0] -= qps0[0];
			stackedQ[j * 5 + 1] -= qps0[1];
			stackedQ[j * 5 + 2] -= qps0[2];
			stackedQ[j * 5 + 3] -= qps0[3];
			stackedQ[j * 5 + 4] -= qps0[4];
		}
		printf("%d qp0[%8.3f %8.3f %8.3f %8.3f %8.3f] kn0[%8.3f %8.3f %8.3f %8.3f %8.3f] = fmin %8.3f\n", i, qps0[0], qps0[1], qps0[2], qps0[3], qps0[4], k50[0], k50[1], k50[2], k50[3], k50[4], fmin);
	}
}

void check_qp0kn0_xyz5A() {
	double fmin = 22;
	printf("\nqp0kn0_xyz5a\n");
	for (int i = 0; i < 5; i++) { k5[i] = k50[i]; }
	for (int i = 0; i < 5; i++) { qps0[i] = 0; }
	ret = estimate_qp0kn0_xyz5A(nrows, stackedQ, stackedX, k5, k5u, k5d, qps0, qpupdn, nlArray, &fmin);
	printf("ret %d  fmin %f\n", ret, fmin);
	printf("qpup[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpupdn[5+i]); } printf("]\n");
	printf("qps0[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qps0[i]); } printf("]\n");
	printf("qpdn[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpupdn[i]); } printf("]\n");
	printf("k5u[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", k5u[i]); } printf("]\n");
	printf("k50[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", k5[i]); } printf("]\n");
	printf("k5d[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", k5d[i]); } printf("]\n");
	std::cout << "puts the initial cathter bend into the pitch, need xyzuxuyuz task\n";
}


void check_qp0kn0_xyzpp11A() {
	double fmin = 22;
	printf("\nqp0kn0_xyzpp11a\n");
	for (int i = 0; i < 5; i++) { k11[i] = k110[i]; }
	for (int i = 0; i < 5; i++) { qps0[i] = 0; }
	ret = estimate_qp0kn0_xyzpp11A(nrows, stackedQ, stackedX, stackedU, k11, k11u, k11d, qps0, qpupdn, nlArray, &fmin);
	printf("ret %d  fmin %f\n", ret, fmin);
	printf("qpup[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpupdn[5 + i]); } printf("]\n");
	printf("qps0[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qps0[i]); } printf("]\n");
	printf("qpdn[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpupdn[i]); } printf("]\n");
	printf("k11u[");  for (int i = 0; i < 11; i++) { printf("%8.3f ", k11u[i]); } printf("]\n");
	printf("k110[");  for (int i = 0; i < 11; i++) { printf("%8.3f ", k11[i]); } printf("]\n");
	printf("k11d[");  for (int i = 0; i < 11; i++) { printf("%8.3f ", k11d[i]); } printf("]\n");
}

void check_qp0kn0_xyzuxuyuz11A() {
	double fmin = 22;
	printf("\nqp0kn0_xyzuxuyuz11a\n");
	for (int i = 0; i < 5; i++) { k11[i] = k110[i]; }
	for (int i = 0; i < 5; i++) { qps0[i] = 0; }
	ret = estimate_qp0kn0_xyzuxuyuz11A(nrows, stackedQ, stackedX, stackedU, k11, k11u, k11d, qps0, qpupdn, nlArray, &fmin);
	printf("ret %d  fmin %f\n", ret, fmin);
	printf("qpup[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpupdn[5 + i]); } printf("]\n");
	printf("qps0[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qps0[i]); } printf("]\n");
	printf("qpdn[");  for (int i = 0; i < 5; i++) { printf("%8.3f ", qpupdn[i]); } printf("]\n");
	printf("k11u[");  for (int i = 0; i < 11; i++) { printf("%8.3f ", k11u[i]); } printf("]\n");
	printf("k110[");  for (int i = 0; i < 11; i++) { printf("%8.3f ", k11[i]); } printf("]\n");
	printf("k11d[");  for (int i = 0; i < 11; i++) { printf("%8.3f ", k11d[i]); } printf("]\n");
}
void main() {
	init();
	load("testSquareQps.dat");

	//check_qp0_xyz5a();
	//check_qp0_xyzuxuyuz5a();
	//check_kn0_xyz5a();
	//check_qp0kn0_xyz5a_flipflop();
	check_qp0kn0_xyz5A();
	check_qp0kn0_xyzpp11A();
	check_qp0kn0_xyzuxuyuz11A();
	
	std::cout << "\n\nPress Enter";
	std::getchar();
	printf("done\n");

	delete[] buffer; //everybody do your share
	delete[] stackedQ;
}


