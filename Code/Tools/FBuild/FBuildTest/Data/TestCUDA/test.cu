//
// Simple cuda test code
//

__global__ void increment(int *a) 
{
	a[threadIdx.x] += 1; // b[threadIdx.x];
}

int main()
{
	const int dataSize = 16; 
	int a[dataSize] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15,16};

	// allocate work buffer
	int *ad;
	const int bufferSize = dataSize * sizeof(int);
	cudaMalloc( (void**)&ad, bufferSize ); 

	// copy input to work buffer
	cudaMemcpy( ad, a, bufferSize, cudaMemcpyHostToDevice ); 

	// do increments
	dim3 dimBlock( dataSize, 1 );
	dim3 dimGrid( 1, 1 );
	increment<<<dimGrid, dimBlock>>>(ad);

	// copy back result
	cudaMemcpy( a, ad, bufferSize, cudaMemcpyDeviceToHost );

	// cleanup
	cudaFree( ad );
	
	return 0;
}