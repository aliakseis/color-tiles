#pragma once

#include <vector>
#include <array>

enum { 
	DIM = 14,
	//NUM_COURSES = 1000000,
};

typedef std::array< std::array<int, DIM>, DIM> Board;

std::vector<int> DoSolve(Board& originalBoard, int numCourses);
