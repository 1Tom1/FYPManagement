#if !defined(MODELOBJ_H)
#define MODELOBJ_H

#include <cstdio>
#include <map>
#include <string>
#include <vector>

class ModelOBJ
{
public:
	struct Vertex
    {
        float position[3];
        float texCoord[2];
    };

	struct Mesh
    {
        int startIndex;
        int triangleCount;
    };

	ModelOBJ();
	~ModelOBJ();

	 bool import(const char *Filename);
	 void importOBJFirst(FILE *pFile);
	 void importOBJSecond(FILE *pFile);
	 void addTriangle(int index,
						int v0, int v1, int v2, 
						int vt0, int vt1, int vt2) ;
	 int addVertex(int hash, const Vertex *pVertex);
	 const int *getIndexBuffer() const;
	 const float *getVertexCoordBuffer() const;
	 const int getNbVertex();
	 const int getNbTriangles();

private:
	bool m_hasPositions;
    bool m_hasNormals;
    bool m_hasTextureCoords;

	int m_numberOfVertexCoords;
    int m_numberOfTextureCoords;
    int m_numberOfTriangles;
    int m_numberOfMeshes;

	std::vector<Vertex> m_vertexBuffer;
    std::vector<int> m_indexBuffer;
	std::vector<float> m_vertexCoords;
    std::vector<float> m_textureCoords;
	std::vector<float> m_normals;
};

inline const int *ModelOBJ::getIndexBuffer() const
{ return &m_indexBuffer[0]; }

inline const float *ModelOBJ::getVertexCoordBuffer() const
{ return &m_vertexCoords[0]; }

 

#endif