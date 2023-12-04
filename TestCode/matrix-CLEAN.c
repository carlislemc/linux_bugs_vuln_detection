matrix_status add_row(matrix* Mat, double* NewRow, int Len) {
	if(Len != Mat->cols) { return MATRIX_ERROR; }

	Mat->rows++;
	Mat->items = (double**)realloc(Mat->items, Mat->rows * sizeof(double*));
	memcpy((void*)Mat->items[Mat->rows], (void*)NewRow, Len * sizeof(double));
	
	return MATRIX_OK;
}
