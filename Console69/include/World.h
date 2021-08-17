#pragma once
#include "Console69.h"
#include <fstream>
#include <strstream>
#include <algorithm>

class World : public Console69
{
public:
	World()
	{
		appName = L"World";
	}

private:
	struct Vector3
	{
		float x{};
		float y{};
		float z{};
		float w{ 1 };
	};

	struct Triangle
	{
		Vector3 vertices[3];
		wchar_t symbol;
		short color;
	};

	struct Mesh
	{
		std::vector<Triangle> triangles;

		bool LoadObjFile(const std::string& file)
		{
			std::ifstream obj(file);
			if (!obj.is_open())
				return false;

			std::vector<Vector3> cache;
			while (!obj.eof())
			{
				char line[128]{};
				char junk{};
				obj.getline(line, 128);

				std::strstream str;
				str << line;
				if (line[0] == 'v')
				{
					Vector3 v{};
					str >> junk >> v.x >> v.y >> v.z;
					cache.push_back(v);
				}
				if (line[0] == 'f')
				{
					int f[3]{};
					str >> junk >> f[0] >> f[1] >> f[2];
					triangles.push_back({ cache[f[0] - 1], cache[f[1] - 1], cache[f[2] - 1] });
				}
			}

			return true;
		}
	};

	struct Matrix
	{
		float v[4][4]{};
	};

private:
	Mesh cube;
	Matrix projection;

	Vector3 camera{10.0f, 10.0f, 0.0f};
	Vector3 viewDir;
	float yaw;
	float angle;

protected:
	virtual bool OnAwake() override
	{
		//cube.triangles =
		//{
		//	// S
		//	{ 0.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 0.0f },
		//	{ 0.0f, 0.0f, 0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f },

		//	// E
		//	{ 1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f },
		//	{ 1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,   1.0f, 0.0f, 1.0f },

		//	// N
		//	{ 1.0f, 0.0f, 1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f },
		//	{ 1.0f, 0.0f, 1.0f,   0.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f },

		//	// W                                                      
		//	{ 0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f },
		//	{ 0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 0.0f },

		//	// U                                      
		//	{ 0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 1.0f,   1.0f, 1.0f, 1.0f },
		//	{ 0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 0.0f },

		//	// D                                                    
		//	{ 1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f },
		//	{ 1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f },
		//};

		//cube.LoadObjFile("D:\\dev\\Console69\\Console69\\obj\\mountains.obj");
		cube.LoadObjFile("obj\\mountains.obj");

		float fov = 90.0f;
		float ratio = (float)GetScreenHeight() / (float)GetScreenWidth();
		float eyeNear = 0.1f;
		float eyeFar = 1000.0f;

		projection = Matrix_GetProjection(fov, ratio, eyeNear, eyeFar);

		return true;
	}

	virtual bool OnUpdate(float deltaTime) override
	{
		float speed = 8.0f;

		if (GetKey(VK_UP).Hold)
			camera.y += speed * deltaTime;
		if (GetKey(VK_DOWN).Hold)
			camera.y -= speed * deltaTime;
		// funky donkey Kappa
		if (GetKey(VK_LEFT).Hold)
			camera.x -= speed * deltaTime;
		if (GetKey(VK_RIGHT).Hold)
			camera.x += speed * deltaTime;

		Vector3 forward = Vector3_Mul(viewDir, speed * deltaTime);

		// the lord of savior WASD
		if (GetKey(L'W').Hold)
			camera = Vector3_Add(camera, forward);
		if (GetKey(L'S').Hold)
			camera = Vector3_Sub(camera, forward);
		if (GetKey(L'A').Hold)
			yaw -= 2.0f * deltaTime;
		if (GetKey(L'D').Hold)
			yaw += 2.0f * deltaTime;


		Matrix Z = Matrix_GetRotationZ(0.5f * angle);
		Matrix X = Matrix_GetRotationX(angle);
		Matrix translation = Matrix_GetTranslation(0.0f, 0.0f, 5.0f);

		Matrix world = Matrix_GetIdentity();
		world = Matrix_MultiplyMatrix(Z, X);
		world = Matrix_MultiplyMatrix(world, translation);

		// point at matrix for camera
		Vector3 up{ 0,1,0 };
		Vector3 target{ 0,0,1 };
		Matrix cameraRotation = Matrix_GetRotationY(yaw);
		viewDir = Matrix_MultiplyVector(cameraRotation, target);
		target = Vector3_Add(camera, viewDir);
		Matrix cameraMatrix = Matrix_PointAt(camera, target, up);

		// view matrix from camera
		Matrix viewMatrix = Matrix_QuickInverse(cameraMatrix);

		// store triangles for raster
		std::vector<Triangle> trianglesToRaster;

		for (auto& tri : cube.triangles)
		{
			Triangle projected{}, transformed{}, viewed{};

			// world matrix transformation
			transformed.vertices[0] = Matrix_MultiplyVector(world, tri.vertices[0]);
			transformed.vertices[1] = Matrix_MultiplyVector(world, tri.vertices[1]);
			transformed.vertices[2] = Matrix_MultiplyVector(world, tri.vertices[2]);

			// calculate triangles normal
			Vector3 line1 = Vector3_Sub(transformed.vertices[1], transformed.vertices[0]);
			Vector3 line2 = Vector3_Sub(transformed.vertices[2], transformed.vertices[0]);
			Vector3 normal = Vector3_CrossProduct(line1, line2);
			normal = Vector3_Normalize(normal);

			// ray from triangle to camera
			Vector3 ray = Vector3_Sub(transformed.vertices[0], camera);

			// only draw the triangles when ray is aligned with normal
			if (Vector3_DotProduct(normal, ray) < 0.0f)
			{
				// genjutsu
				Vector3 light{ 0.0f,1.0f,-1.0f };
				light = Vector3_Normalize(light);

				// how aligned
				float dp = max(0.1f, Vector3_DotProduct(light, normal));

				// console bs
				CHAR_INFO ci = GetColor(dp);
				transformed.color = ci.Attributes;
				transformed.symbol = ci.Char.UnicodeChar;

				// world space -> view space
				viewed.vertices[0] = Matrix_MultiplyVector(viewMatrix, transformed.vertices[0]);
				viewed.vertices[1] = Matrix_MultiplyVector(viewMatrix, transformed.vertices[1]);
				viewed.vertices[2] = Matrix_MultiplyVector(viewMatrix, transformed.vertices[2]);
				viewed.symbol = transformed.symbol;
				viewed.color = transformed.color;

				// clip triangles against near
				int clippedTriangles{};
				Triangle clipped[2];
				clippedTriangles = Triangle_ClipAgainstPlane(
					{ 0.0f, 0.0f, 0.1f }, { 0.0f, 0.0f, 1.0f },
					viewed, clipped[0], clipped[1]
				);

				// project when multiple triangles form the clip
				for (int n = 0; n < clippedTriangles; ++n)
				{
					// project triangles from 3D --> 2D
					projected.vertices[0] = Matrix_MultiplyVector(projection, clipped[n].vertices[0]);
					projected.vertices[1] = Matrix_MultiplyVector(projection, clipped[n].vertices[1]);
					projected.vertices[2] = Matrix_MultiplyVector(projection, clipped[n].vertices[2]);
					projected.color = clipped[n].color;
					projected.symbol = clipped[n].symbol;

					// scale into view, we moved the normalising into cartesian space
					// out of the matrix.vector function from the previous videos, so
					// do this manually
					projected.vertices[0] = Vector3_Div(projected.vertices[0], projected.vertices[0].w);
					projected.vertices[1] = Vector3_Div(projected.vertices[1], projected.vertices[1].w);
					projected.vertices[2] = Vector3_Div(projected.vertices[2], projected.vertices[2].w);

					// X/Y are inverted so put them back
					projected.vertices[0].x *= -1.0f;
					projected.vertices[1].x *= -1.0f;
					projected.vertices[2].x *= -1.0f;
					projected.vertices[0].y *= -1.0f;
					projected.vertices[1].y *= -1.0f;
					projected.vertices[2].y *= -1.0f;

					// offset verts into visible normalised space
					Vector3 vOffsetView = { 1,1,0 };
					projected.vertices[0] = Vector3_Add(projected.vertices[0], vOffsetView);
					projected.vertices[1] = Vector3_Add(projected.vertices[1], vOffsetView);
					projected.vertices[2] = Vector3_Add(projected.vertices[2], vOffsetView);
					projected.vertices[0].x *= 0.5f * (float)GetScreenWidth();
					projected.vertices[0].y *= 0.5f * (float)GetScreenHeight();
					projected.vertices[1].x *= 0.5f * (float)GetScreenWidth();
					projected.vertices[1].y *= 0.5f * (float)GetScreenHeight();
					projected.vertices[2].x *= 0.5f * (float)GetScreenWidth();
					projected.vertices[2].y *= 0.5f * (float)GetScreenHeight();

					// store triangle for sorting
					trianglesToRaster.push_back(projected);
				}
			}
 		}

		// painting algorith ( from back to front )
		sort(trianglesToRaster.begin(), trianglesToRaster.end(), [](Triangle& t1, Triangle& t2)
			{
				float z1 = (t1.vertices[0].z + t1.vertices[1].z + t1.vertices[2].z) / 3.0f;
				float z2 = (t2.vertices[0].z + t2.vertices[1].z + t2.vertices[2].z) / 3.0f;
				return z1 > z2;
			});

		// clear screen
		Fill(0, 0, GetScreenWidth(), GetScreenHeight(), Solid, FG_Black);

		// BLACK BOX...
		for (auto& tri : trianglesToRaster)
		{
			// clip triangles against all four screen edges, this could yield
			// a bunch of triangles, so create a queue that we traverse to 
			// ensure we only test new triangles generated against planes
			Triangle clipped[2];
			std::list<Triangle> cache;

			cache.push_back(tri);
			int newTriangles = 1;
			for (int p = 0; p < 4; p++)
			{
				int trianglesToAdd = 0;
				while (newTriangles > 0)
				{
					// take triangle from front of queue
					Triangle test = cache.front();
					cache.pop_front();
					newTriangles--;

					// clip it against a plane. We only need to test each 
					// subsequent plane, against subsequent new triangles
					// as all triangles after a plane clip are guaranteed
					// to lie on the inside of the plane. I like how this
					// comment is almost completely and utterly justified
					switch (p)
					{
					case 0:	trianglesToAdd = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
					case 1:	trianglesToAdd = Triangle_ClipAgainstPlane({ 0.0f, (float)GetScreenHeight() - 1, 0.0f }, { 0.0f, -1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
					case 2:	trianglesToAdd = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
					case 3:	trianglesToAdd = Triangle_ClipAgainstPlane({ (float)GetScreenWidth() - 1, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
					}

					// Clipping may yield a variable number of triangles, so
					// add these new ones to the back of the queue for subsequent
					// clipping against next planes
					for (int w = 0; w < trianglesToAdd; w++)
						cache.push_back(clipped[w]);
				}
				newTriangles = cache.size();
			}

			for (auto& t : cache)
			{
				FillTriangle(t.vertices[0].x, t.vertices[0].y,
							 t.vertices[1].x, t.vertices[1].y,
							 t.vertices[2].x, t.vertices[2].y,
							 t.symbol, t.color);
			}
		}

		return true;
	}

	// helper functions ( all below are ugly but intuitive for learning )
	Vector3 Matrix_MultiplyVector(Matrix& m, Vector3& i) const
	{
		Vector3 v;
		v.x = i.x * m.v[0][0] + i.y * m.v[1][0] + i.z * m.v[2][0] + i.w * m.v[3][0];
		v.y = i.x * m.v[0][1] + i.y * m.v[1][1] + i.z * m.v[2][1] + i.w * m.v[3][1];
		v.z = i.x * m.v[0][2] + i.y * m.v[1][2] + i.z * m.v[2][2] + i.w * m.v[3][2];
		v.w = i.x * m.v[0][3] + i.y * m.v[1][3] + i.z * m.v[2][3] + i.w * m.v[3][3];
		return v;
	}

	Matrix Matrix_GetIdentity() const
	{
		Matrix identity;
		identity.v[0][0] = 1.0f;
		identity.v[1][1] = 1.0f;
		identity.v[2][2] = 1.0f;
		identity.v[3][3] = 1.0f;
		return identity;
	}

	Matrix Matrix_GetRotationX(float rad) const
	{
		Matrix rotationX;
		rotationX.v[0][0] = 1.0f;
		rotationX.v[1][1] = cosf(rad);
		rotationX.v[1][2] = sinf(rad);
		rotationX.v[2][1] = -sinf(rad);
		rotationX.v[2][2] = cosf(rad);
		rotationX.v[3][3] = 1.0f;
		return rotationX;
	}

	Matrix Matrix_GetRotationY(float rad) const
	{
		Matrix rotationY;
		rotationY.v[0][0] = cosf(rad);
		rotationY.v[0][2] = sinf(rad);
		rotationY.v[2][0] = -sinf(rad);
		rotationY.v[1][1] = 1.0f;
		rotationY.v[2][2] = cosf(rad);
		rotationY.v[3][3] = 1.0f;
		return rotationY;
	}

	Matrix Matrix_GetRotationZ(float rad) const
	{
		Matrix rotationZ;
		rotationZ.v[0][0] = cosf(rad);
		rotationZ.v[0][1] = sinf(rad);
		rotationZ.v[1][0] = -sinf(rad);
		rotationZ.v[1][1] = cosf(rad);
		rotationZ.v[2][2] = 1.0f;
		rotationZ.v[3][3] = 1.0f;
		return rotationZ;
	}

	Matrix Matrix_GetTranslation(float x, float y, float z) const
	{
		Matrix translation;
		translation.v[0][0] = 1.0f;
		translation.v[1][1] = 1.0f;
		translation.v[2][2] = 1.0f;
		translation.v[3][3] = 1.0f;
		translation.v[3][0] = x;
		translation.v[3][1] = y;
		translation.v[3][2] = z;
		return translation;
	}

	Matrix Matrix_GetProjection(float fov, float ratio, float eyeNear, float eyeFar) const
	{
		float fFovRad = 1.0f / tanf(fov * 0.5f / 180.0f * 3.14159f);
		Matrix projection;
		projection.v[0][0] = ratio * fFovRad;
		projection.v[1][1] = fFovRad;
		projection.v[2][2] = eyeFar / (eyeFar - eyeNear);
		projection.v[3][2] = (-eyeFar * eyeNear) / (eyeFar - eyeNear);
		projection.v[2][3] = 1.0f;
		projection.v[3][3] = 0.0f;
		return projection;
	}

	Matrix Matrix_MultiplyMatrix(Matrix& m1, Matrix& m2) const
	{
		Matrix matrix;
		for (int c = 0; c < 4; c++)
			for (int r = 0; r < 4; r++)
				matrix.v[r][c] = m1.v[r][0] * m2.v[0][c] + m1.v[r][1] * m2.v[1][c] + 
								 m1.v[r][2] * m2.v[2][c] + m1.v[r][3] * m2.v[3][c];
		return matrix;
	}

	Matrix Matrix_PointAt(Vector3& pos, Vector3& target, Vector3& up) const
	{
		// calculate new forward direction
		Vector3 newForward = Vector3_Sub(target, pos);
		newForward = Vector3_Normalize(newForward);

		// calculate new Up direction
		Vector3 a = Vector3_Mul(newForward, Vector3_DotProduct(up, newForward));
		Vector3 newUp = Vector3_Sub(up, a);
		newUp = Vector3_Normalize(newUp);

		// new Right direction is easy, its just cross product
		Vector3 newRight = Vector3_CrossProduct(newUp, newForward);

		// construct Dimensioning and Translation Matrix	
		Matrix matrix;
		matrix.v[0][0] = newRight.x;	matrix.v[0][1] = newRight.y;	matrix.v[0][2] = newRight.z;	matrix.v[0][3] = 0.0f;
		matrix.v[1][0] = newUp.x;		matrix.v[1][1] = newUp.y;		matrix.v[1][2] = newUp.z;		matrix.v[1][3] = 0.0f;
		matrix.v[2][0] = newForward.x;	matrix.v[2][1] = newForward.y;	matrix.v[2][2] = newForward.z;	matrix.v[2][3] = 0.0f;
		matrix.v[3][0] = pos.x;			matrix.v[3][1] = pos.y;			matrix.v[3][2] = pos.z;			matrix.v[3][3] = 1.0f;
		return matrix;
	}

	Matrix Matrix_QuickInverse(Matrix& m) const // Only for Rotation/Translation Matrices
	{
		Matrix inverse;
		inverse.v[0][0] = m.v[0][0]; inverse.v[0][1] = m.v[1][0]; inverse.v[0][2] = m.v[2][0]; inverse.v[0][3] = 0.0f;
		inverse.v[1][0] = m.v[0][1]; inverse.v[1][1] = m.v[1][1]; inverse.v[1][2] = m.v[2][1]; inverse.v[1][3] = 0.0f;
		inverse.v[2][0] = m.v[0][2]; inverse.v[2][1] = m.v[1][2]; inverse.v[2][2] = m.v[2][2]; inverse.v[2][3] = 0.0f;
		inverse.v[3][0] = -(m.v[3][0] * inverse.v[0][0] + m.v[3][1] * inverse.v[1][0] + m.v[3][2] * inverse.v[2][0]);
		inverse.v[3][1] = -(m.v[3][0] * inverse.v[0][1] + m.v[3][1] * inverse.v[1][1] + m.v[3][2] * inverse.v[2][1]);
		inverse.v[3][2] = -(m.v[3][0] * inverse.v[0][2] + m.v[3][1] * inverse.v[1][2] + m.v[3][2] * inverse.v[2][2]);
		inverse.v[3][3] = 1.0f;
		return inverse;
	}

	Vector3 Vector3_Add(Vector3& v1, Vector3& v2) const
	{
		return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
	}

	Vector3 Vector3_Sub(Vector3& v1, Vector3& v2) const
	{
		return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
	}

	Vector3 Vector3_Mul(Vector3& v1, float scale) const
	{
		return { v1.x * scale, v1.y * scale, v1.z * scale };
	}

	Vector3 Vector3_Div(Vector3& v1, float scale) const
	{
		return { v1.x / scale, v1.y / scale, v1.z / scale };
	}

	float Vector3_DotProduct(Vector3& v1, Vector3& v2) const
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	float Vector3_Length(Vector3& v) const
	{
		return sqrtf(Vector3_DotProduct(v, v));
	}

	Vector3 Vector3_Normalize(Vector3& v) const
	{
		float l = Vector3_Length(v);
		return { v.x / l, v.y / l, v.z / l };
	}

	Vector3 Vector3_CrossProduct(Vector3& v1, Vector3& v2) const
	{
		Vector3 cp;
		cp.x = v1.y * v2.z - v1.z * v2.y;
		cp.y = v1.z * v2.x - v1.x * v2.z;
		cp.z = v1.x * v2.y - v1.y * v2.x;
		return cp;
	}

	// clipping... yeah just treat it as a black box right now...
	Vector3 Vector_IntersectPlane(Vector3& plane_p, Vector3& plane_n, Vector3& lineStart, Vector3& lineEnd) const
	{
		plane_n = Vector3_Normalize(plane_n);
		float plane_d = -Vector3_DotProduct(plane_n, plane_p);
		float ad = Vector3_DotProduct(lineStart, plane_n);
		float bd = Vector3_DotProduct(lineEnd, plane_n);
		float t = (-plane_d - ad) / (bd - ad);
		Vector3 lineStartToEnd = Vector3_Sub(lineEnd, lineStart);
		Vector3 lineToIntersect = Vector3_Mul(lineStartToEnd, t);
		return Vector3_Add(lineStart, lineToIntersect);
	}

	int Triangle_ClipAgainstPlane(Vector3 plane_p, Vector3 plane_n, Triangle& in_tri, Triangle& out_tri1, Triangle& out_tri2) const
	{
		// make sure plane normal is indeed normal
		plane_n = Vector3_Normalize(plane_n);

		// return signed shortest distance from point to plane, plane normal must be normalised
		auto dist = [&](Vector3& p)
		{
			Vector3 n = Vector3_Normalize(p);
			return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - Vector3_DotProduct(plane_n, plane_p));
		};

		// create two temporary storage arrays to classify points either side of plane
		// if distance sign is positive, point lies on "inside" of plane
		Vector3* inside_points[3];  int nInsidePointCount = 0;
		Vector3* outside_points[3]; int nOutsidePointCount = 0;

		// get signed distance of each point in triangle to plane
		float d0 = dist(in_tri.vertices[0]);
		float d1 = dist(in_tri.vertices[1]);
		float d2 = dist(in_tri.vertices[2]);

		if (d0 >= 0) { inside_points[nInsidePointCount++] = &in_tri.vertices[0]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.vertices[0]; }
		if (d1 >= 0) { inside_points[nInsidePointCount++] = &in_tri.vertices[1]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.vertices[1]; }
		if (d2 >= 0) { inside_points[nInsidePointCount++] = &in_tri.vertices[2]; }
		else { outside_points[nOutsidePointCount++] = &in_tri.vertices[2]; }

		// now classify triangle points, and break the input triangle into 
		// smaller output triangles if required. There are four possible
		// outcomes...

		if (nInsidePointCount == 0)
		{
			// all points lie on the outside of plane, so clip whole triangle
			// It ceases to exist

			return 0; // No returned triangles are valid
		}

		if (nInsidePointCount == 3)
		{
			// all points lie on the inside of plane, so do nothing
			// and allow the triangle to simply pass through
			out_tri1 = in_tri;

			return 1; // Just the one returned original triangle is valid
		}

		if (nInsidePointCount == 1 && nOutsidePointCount == 2)
		{
			// triangle should be clipped. As two points lie outside
			// the plane, the triangle simply becomes a smaller triangle

			// topy appearance info to new triangle
			out_tri1.color = in_tri.color;
			out_tri1.symbol = in_tri.symbol;

			// the inside point is valid, so keep that...
			out_tri1.vertices[0] = *inside_points[0];

			// but the two new points are at the locations where the 
			// original sides of the triangle (lines) intersect with the plane
			out_tri1.vertices[1] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);
			out_tri1.vertices[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1]);

			return 1; // return the newly formed single triangle
		}

		if (nInsidePointCount == 2 && nOutsidePointCount == 1)
		{
			// triangle should be clipped. As two points lie inside the plane,
			// the clipped triangle becomes a "quad". Fortunately, we can
			// represent a quad with two new triangles

			// copy appearance info to new triangles
			out_tri1.color = in_tri.color;
			out_tri1.symbol = in_tri.symbol;

			out_tri2.color = in_tri.color;
			out_tri2.symbol = in_tri.symbol;

			// the first triangle consists of the two inside points and a new
			// point determined by the location where one side of the triangle
			// intersects with the plane
			out_tri1.vertices[0] = *inside_points[0];
			out_tri1.vertices[1] = *inside_points[1];
			out_tri1.vertices[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);

			// the second triangle is composed of one of he inside points, a
			// new point determined by the intersection of the other side of the 
			// triangle and the plane, and the newly created point above
			out_tri2.vertices[0] = *inside_points[1];
			out_tri2.vertices[1] = out_tri1.vertices[2];
			out_tri2.vertices[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0]);

			return 2; // return two newly formed triangles which form a quad
		}
	}


	// console bs
	CHAR_INFO GetColor(float lum) 
	{
		short bg_col, fg_col;
		wchar_t sym;
		int pixel_bw = (int)(13.0f * lum);
		switch (pixel_bw)
		{
		case 0: bg_col = BG_Black; fg_col = BG_Black; sym = Solid; break;

		case 1: bg_col = BG_Black; fg_col = FG_DarkGrey; sym = OneQuater; break;
		case 2: bg_col = BG_Black; fg_col = FG_DarkGrey; sym = TwoQuaters; break;
		case 3: bg_col = BG_Black; fg_col = FG_DarkGrey; sym = ThreeQuaters; break;
		case 4: bg_col = BG_Black; fg_col = FG_DarkGrey; sym = Solid; break;

		case 5: bg_col = BG_DarkGrey; fg_col = FG_Grey; sym = OneQuater; break;
		case 6: bg_col = BG_DarkGrey; fg_col = FG_Grey; sym = TwoQuaters; break;
		case 7: bg_col = BG_DarkGrey; fg_col = FG_Grey; sym = ThreeQuaters; break;
		case 8: bg_col = BG_DarkGrey; fg_col = FG_Grey; sym = Solid; break;

		case 9:  bg_col = BG_Grey; fg_col = FG_White; sym = OneQuater; break;
		case 10: bg_col = BG_Grey; fg_col = FG_White; sym = TwoQuaters; break;
		case 11: bg_col = BG_Grey; fg_col = FG_White; sym = ThreeQuaters; break;
		case 12: bg_col = BG_Grey; fg_col = FG_White; sym = Solid; break;
		default:
			bg_col = BG_Black; fg_col = FG_Black; sym = Solid;
		}

		CHAR_INFO c{};
		c.Attributes = bg_col | fg_col;
		c.Char.UnicodeChar = sym;
		return c;
	}
};