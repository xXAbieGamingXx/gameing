#include "plane.h"

struct plane_desc plane_list[] = {
	{
		.x = 5,
	},
	{
		.rz = 90,
	}
};

int plane_list_size = sizeof(plane_list)/sizeof(struct plane_desc);
