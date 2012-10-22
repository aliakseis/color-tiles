#pragma once

#include <vector>

enum { 
	DIM = 14,
	//NUM_COURSES = 1000000,
};

typedef int Board[DIM][DIM];

std::vector<int> DoSolve(Board& originalBoard, int numCourses);
