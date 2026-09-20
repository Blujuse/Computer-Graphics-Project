#pragma once

#include <vector>
#include <Eigen/Dense>

struct VertexIndices
{
	int vert, tex, norm;
};


/// <summary>
/// A Model stores mesh data and can load this data from an obj file.
/// </summary>
class Model {
private:
	std::vector<Eigen::Vector3f> verts_, vns_; // Vertices and vertex normals
	std::vector<Eigen::Vector2f> vts_; // Texture coordinates
	std::vector<std::vector<VertexIndices>> faces_;  // Face indices of the vertices
public:
	Model(const char *filename);
	~Model();
	int nverts() const;
	int nfaces() const;
	Eigen::Vector3f vert(int i) const;
	Eigen::Vector2f texCoord(int i) const;
	Eigen::Vector3f normal(int i) const;
	std::vector<VertexIndices> face(int idx) const;
	bool hasNormals() const;
};

