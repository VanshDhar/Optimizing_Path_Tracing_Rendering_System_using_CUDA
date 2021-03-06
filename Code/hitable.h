#ifndef HITABLEH
#define HITABLEH

#include "ray.h"
#include "aabb.h"

class material;

struct hit_record {
	float t;
	float u;
	float v;
	vec3 p;
	vec3 normal;
	material *mat_ptr;
};

class hitable {
public:
	__device__ virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const = 0;
	__device__ virtual bool bounding_box(float t0, float t1, aabb& box) const = 0;
};

class xy_rect : public hitable {
public:
	__device__ xy_rect() {}
	__device__ xy_rect(float _x0, float _x1, float _y0, float _y1, float _k, material *mat)
		: x0(_x0), x1(_x1), y0(_y0), y1(_y1), k(_k), mp(mat) {};
	__device__ virtual bool hit(const ray& r, float t0, float t1, hit_record& rec) const;
	__device__ virtual bool bounding_box(float t0, float t1, aabb& box) const {
		box = aabb(vec3(x0, y0, k - 0.0001), vec3(x1, y1, k + 0.0001));
		return true;
	}
	material *mp;
	float x0, x1, y0, y1, k;
};

__device__ bool xy_rect::hit(const ray& r, float t0, float t1, hit_record& rec) const {
	float t = (k - r.origin().z()) / r.direction().z();
	if (t < t0 || t > t1)
		return false;
	float x = r.origin().x() + t * r.direction().x();
	float y = r.origin().y() + t * r.direction().y();
	if (x < x0 || x > x1 || y < y0 || y > y1)
		return false;
	rec.u = (x - x0) / (x1 - x0);
	rec.v = (y - y0) / (y1 - y0);
	rec.t = t;
	rec.mat_ptr = mp;
	rec.p = r.point_at_parameter(t);
	rec.normal = vec3(0, 0, 1);
	return true;
}

class xz_rect : public hitable {
public:
	__device__ xz_rect() {}
	__device__ xz_rect(float _x0, float _x1, float _z0, float _z1, float _k, material *mat)
		: x0(_x0), x1(_x1), z0(_z0), z1(_z1), k(_k), mp(mat) {};
	__device__ virtual bool hit(const ray& r, float t0, float t1, hit_record& rec) const;
	__device__ virtual bool bounding_box(float t0, float t1, aabb& box) const {
		box = aabb(vec3(x0, k - 0.0001, z0), vec3(x1, k + 0.0001, z1));
		return true;
	}
	material *mp;
	float x0, x1, z0, z1, k;
};

class yz_rect : public hitable {
public:
	__device__ yz_rect() {}
	__device__ yz_rect(float _y0, float _y1, float _z0, float _z1, float _k, material *mat)
		: y0(_y0), y1(_y1), z0(_z0), z1(_z1), k(_k), mp(mat) {};
	__device__ virtual bool hit(const ray& r, float t0, float t1, hit_record& rec) const;
	__device__ virtual bool bounding_box(float t0, float t1, aabb& box) const {
		box = aabb(vec3(k - 0.0001, y0, z0), vec3(k + 0.0001, y1, z1));
		return true;
	}
	material  *mp;
	float y0, y1, z0, z1, k;
};

__device__ bool xz_rect::hit(const ray& r, float t0, float t1, hit_record& rec) const {
	float t = (k - r.origin().y()) / r.direction().y();
	if (t < t0 || t > t1)
		return false;
	float x = r.origin().x() + t * r.direction().x();
	float z = r.origin().z() + t * r.direction().z();
	if (x < x0 || x > x1 || z < z0 || z > z1)
		return false;
	rec.u = (x - x0) / (x1 - x0);
	rec.v = (z - z0) / (z1 - z0);
	rec.t = t;
	rec.mat_ptr = mp;
	rec.p = r.point_at_parameter(t);
	rec.normal = vec3(0, 1, 0);
	return true;
}

__device__ bool yz_rect::hit(const ray& r, float t0, float t1, hit_record& rec) const {
	float t = (k - r.origin().x()) / r.direction().x();
	if (t < t0 || t > t1)
		return false;
	float y = r.origin().y() + t * r.direction().y();
	float z = r.origin().z() + t * r.direction().z();
	if (y < y0 || y > y1 || z < z0 || z > z1)
		return false;
	rec.u = (y - y0) / (y1 - y0);
	rec.v = (z - z0) / (z1 - z0);
	rec.t = t;
	rec.mat_ptr = mp;
	rec.p = r.point_at_parameter(t);
	rec.normal = vec3(1, 0, 0);
	return true;
}

class flip_normals : public hitable {
public:
	__device__ flip_normals(hitable *p) : ptr(p) {}

	__device__ virtual bool hit(
		const ray& r, float t_min, float t_max, hit_record& rec) const {

		if (ptr->hit(r, t_min, t_max, rec)) {
			rec.normal = -rec.normal;
			return true;
		}
		else
			return false;
	}

	__device__ virtual bool bounding_box(float t0, float t1, aabb& box) const {
		return ptr->bounding_box(t0, t1, box);
	}

	hitable *ptr;
};

class translate : public hitable {
public:
	__device__ translate(hitable *p, const vec3& displacement)
		: ptr(p), offset(displacement) {}
	__device__ virtual bool hit(
		const ray& r, float t_min, float t_max, hit_record& rec) const;
	__device__ virtual bool bounding_box(float t0, float t1, aabb& box) const;
	hitable *ptr;
	vec3 offset;
};

__device__ bool translate::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
	ray moved_r(r.origin() - offset, r.direction(), r.time());
	if (ptr->hit(moved_r, t_min, t_max, rec)) {
		rec.p += offset;
		return true;
	}
	else
		return false;
}

__device__ bool translate::bounding_box(float t0, float t1, aabb& box) const {
	if (ptr->bounding_box(t0, t1, box)) {
		box = aabb(box.min() + offset, box.max() + offset);
		return true;
	}
	else
		return false;
}

class rotate_y : public hitable {
public:
	__device__ rotate_y(hitable *p, float angle);
	__device__ virtual bool hit(
		const ray& r, float t_min, float t_max, hit_record& rec) const;
	__device__ virtual bool bounding_box(float t0, float t1, aabb& box) const {
		box = bbox; return hasbox;
	}
	hitable *ptr;
	float sin_theta;
	float cos_theta;
	bool hasbox;
	aabb bbox;
};

__device__ rotate_y::rotate_y(hitable *p, float angle) : ptr(p) {
	float radians = (3.1428 / 180.) * angle;
	sin_theta = sin(radians);
	cos_theta = cos(radians);
	hasbox = ptr->bounding_box(0, 1, bbox);
	vec3 min(FLT_MAX, FLT_MAX, FLT_MAX);
	vec3 max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			for (int k = 0; k < 2; k++) {
				float x = i * bbox.max().x() + (1 - i)*bbox.min().x();
				float y = j * bbox.max().y() + (1 - j)*bbox.min().y();
				float z = k * bbox.max().z() + (1 - k)*bbox.min().z();
				float newx = cos_theta * x + sin_theta * z;
				float newz = -sin_theta * x + cos_theta * z;
				vec3 tester(newx, y, newz);
				for (int c = 0; c < 3; c++)
				{
					if (tester[c] > max[c])
						max[c] = tester[c];
					if (tester[c] < min[c])
						min[c] = tester[c];
				}
			}
		}
	}
	bbox = aabb(min, max);
}

__device__ bool rotate_y::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
	vec3 origin = r.origin();
	vec3 direction = r.direction();
	origin[0] = cos_theta * r.origin()[0] - sin_theta * r.origin()[2];
	origin[2] = sin_theta * r.origin()[0] + cos_theta * r.origin()[2];
	direction[0] = cos_theta * r.direction()[0] - sin_theta * r.direction()[2];
	direction[2] = sin_theta * r.direction()[0] + cos_theta * r.direction()[2];
	ray rotated_r(origin, direction, r.time());
	if (ptr->hit(rotated_r, t_min, t_max, rec)) {
		vec3 p = rec.p;
		vec3 normal = rec.normal;
		p[0] = cos_theta * rec.p[0] + sin_theta * rec.p[2];
		p[2] = -sin_theta * rec.p[0] + cos_theta * rec.p[2];
		normal[0] = cos_theta * rec.normal[0] + sin_theta * rec.normal[2];
		normal[2] = -sin_theta * rec.normal[0] + cos_theta * rec.normal[2];
		rec.p = p;
		rec.normal = normal;
		return true;
	}
	else
		return false;
}

#endif