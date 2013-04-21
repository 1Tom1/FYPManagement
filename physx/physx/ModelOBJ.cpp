#include "ModelOBJ.h"
#include <iostream>

ModelOBJ::ModelOBJ()
{
    m_hasPositions = false;
    m_hasNormals = false;
    m_hasTextureCoords = false;
  
    m_numberOfVertexCoords = 0;
    m_numberOfTextureCoords = 0;
    m_numberOfTriangles = 0;
    m_numberOfMeshes = 0;
}

ModelOBJ::~ModelOBJ()
{
}

void ModelOBJ::addTriangle(int index,
						int v0, int v1, int v2, 
						int vt0, int vt1, int vt2)
{
	
	Vertex vertex = {
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f,
	};

	vertex.position[0] = m_vertexCoords[v0 * 3];
    vertex.position[1] = m_vertexCoords[v0 * 3 + 1];
    vertex.position[2] = m_vertexCoords[v0 * 3 + 2];
    vertex.texCoord[0] = m_textureCoords[vt0 * 2];
    vertex.texCoord[1] = m_textureCoords[vt0 * 2 + 1];
	m_indexBuffer[index * 3] = v0;

	vertex.position[0] = m_vertexCoords[v1 * 3];
    vertex.position[1] = m_vertexCoords[v1 * 3 + 1];
    vertex.position[2] = m_vertexCoords[v1 * 3 + 2];
    vertex.texCoord[0] = m_textureCoords[vt1 * 2];
    vertex.texCoord[1] = m_textureCoords[vt1 * 2 + 1];
	m_indexBuffer[index * 3 + 1] = v1;

	vertex.position[0] = m_vertexCoords[v2 * 3];
    vertex.position[1] = m_vertexCoords[v2 * 3 + 1];
    vertex.position[2] = m_vertexCoords[v2 * 3 + 2];
    vertex.texCoord[0] = m_textureCoords[vt2 * 2];
    vertex.texCoord[1] = m_textureCoords[vt2 * 2 + 1];
	m_indexBuffer[index * 3 + 1] = v2;

}


bool ModelOBJ::import(const char *PathOfFile)
{
	// open file
	FILE *pFile = fopen(PathOfFile,"r");
	if (!pFile){
		std::cout<<"load fail";
        return false;
	}

	std::string fileName = PathOfFile;
	std::string::size_type offset = fileName.find_last_of('\\');
	char buffer[256] = {0};
	importOBJFirst(pFile);
	rewind(pFile); //reset file header
	importOBJSecond(pFile);
	
	
	
	fclose(pFile);

	return true;
}

void ModelOBJ::importOBJFirst(FILE *pFile)
{
	int v = 0;
	int vt = 0;
	int vn = 0;
	char buffer[256] = {0};
	

	while(fscanf(pFile,"%s",buffer) != EOF)
	{
		switch(buffer[0])
		{
		case 'f': // v/vt/vn
			fscanf(pFile,"%s",buffer);
			 if (strstr(buffer, "//")) // v//vn
            {
                sscanf(buffer, "%d//%d", &v, &vn);
                fscanf(pFile, "%d//%d", &v, &vn);
                fscanf(pFile, "%d//%d", &v, &vn);
                ++m_numberOfTriangles;

                while (fscanf(pFile, "%d//%d", &v, &vn) > 0)
                    ++m_numberOfTriangles;
            }
			else if(sscanf(buffer, "%d/%d/%d", &v, &vt, &vn) == 3)
			{
				fscanf(pFile, "%d/%d/%d", &v, &vt, &vn);
                fscanf(pFile, "%d/%d/%d", &v, &vt, &vn);
				++m_numberOfTriangles;

                while (fscanf(pFile, "%d/%d/%d", &v, &vt, &vn) > 0)
                    ++m_numberOfTriangles;
			}
			else if (sscanf(buffer, "%d/%d", &v, &vt) == 2) // v/vt
            {
                fscanf(pFile, "%d/%d", &v, &vt);
                fscanf(pFile, "%d/%d", &v, &vt);
                ++m_numberOfTriangles;

                while (fscanf(pFile, "%d/%d", &v, &vt) > 0)
                    ++m_numberOfTriangles;
            }
			break;

		case 'v': //v,vt
			switch(buffer[1])
			{
			case '\0':
                fgets(buffer, sizeof(buffer), pFile);
                ++m_numberOfVertexCoords;
                break;

            case 't':
                fgets(buffer, sizeof(buffer), pFile);
                ++m_numberOfTextureCoords;

            default:
                break;
			}
			break;

		default:
			fgets(buffer,sizeof(buffer),pFile);
			break;
		}
	}

	m_vertexCoords.resize(m_numberOfVertexCoords * 3);
	m_textureCoords.resize(m_numberOfTextureCoords*2);
	m_indexBuffer.resize(m_numberOfTriangles * 3);
	
}

void ModelOBJ::importOBJSecond(FILE *pFile)
{
	int v[3] = {0};
	int vt[3] = {0};
	int vn[3] = {0};

	int numVertices = 0;
    int numTexCoords = 0;
	int numNormals = 0;
	int numTriangles = 0;
	char buffer[256] = {0};

	while(fscanf(pFile, "%s", buffer) != EOF)
	{

		switch(buffer[0])
		{
			
		case 'f':
			v[0] = v[1] = v[2] = 0;
			vt[0] = vt[1] = vt[2] = 0;
			vn[0] = vn[1] = vn[2] = 0;

			fscanf(pFile, "%s", buffer);
		if (strstr(buffer, "//"))
		{
				sscanf(buffer, "%d//%d", &v[0], &vn[0]);
                fscanf(pFile, "%d//%d", &v[1], &vn[1]);
                fscanf(pFile, "%d//%d", &v[2], &vn[2]);

				v[0] = v[0] - 1;
                v[1] = v[1] - 1;
                v[2] = v[2] - 1;

                vn[0] = vn[0] - 1;
                vn[1] = vn[1] - 1;
                vn[2] = vn[2] - 1;

				m_indexBuffer[numTriangles * 3] = v[0];
				m_indexBuffer[numTriangles * 3 + 1] = v[1];
				m_indexBuffer[numTriangles * 3 + 2] = v[2];
				numTriangles++;

				v[1] = v[2];
                vn[1] = vn[2];

				 while (fscanf(pFile, "%d//%d", &v[2], &vn[2]) > 0)
                {
                    v[2] = v[2] - 1;
                    vn[2] = vn[2] - 1;

					m_indexBuffer[numTriangles * 3] = v[0];
					m_indexBuffer[numTriangles * 3 + 1] = v[1];
					m_indexBuffer[numTriangles * 3 + 2] = v[2];
                    numTriangles++;

                    v[1] = v[2];
                    vn[1] = vn[2];
                }
			}
			else if(sscanf(buffer,"%d/%d/%d", &v[0], &vt[0], &vn[0]) == 3)
			{
				fscanf(pFile, "%d/%d/%d", &v[1], &vt[1], &vn[1]);
                fscanf(pFile, "%d/%d/%d", &v[2], &vt[2], &vn[2]);
				
				v[0] =  v[0] - 1;
                v[1] =  v[1] - 1;
                v[2] =  v[2] - 1;

                vt[0] =  vt[0] - 1;
                vt[1] =  vt[1] - 1;
                vt[2] =  vt[2] - 1;
				
				m_indexBuffer[numTriangles * 3] = v[0];
				m_indexBuffer[numTriangles * 3 + 1] = v[1];
				m_indexBuffer[numTriangles * 3 + 2] = v[2];
				numTriangles++;

				v[1] = v[2];
                vt[1] = vt[2];

				 while (fscanf(pFile, "%d/%d", &v[2], &vt[2]) > 0)
                {
                    v[2] = (v[2] < 0) ? v[2] + numVertices - 1 : v[2] - 1;
                    vt[2] = (vt[2] < 0) ? vt[2] + numTexCoords - 1 : vt[2] - 1;

                    m_indexBuffer[numTriangles * 3] = v[0];
					m_indexBuffer[numTriangles * 3 + 1] = v[1];
					m_indexBuffer[numTriangles * 3 + 2] = v[2];
					numTriangles++;

                    v[1] = v[2];
                    vt[1] = vt[2];

					 while (fscanf(pFile, "%d/%d/%d", &v[2], &vt[2], &vn[2]) > 0)
					{
                    v[2] = (v[2] < 0) ? v[2] + numVertices - 1 : v[2] - 1;
                    vt[2] = (vt[2] < 0) ? vt[2] + numTexCoords - 1 : vt[2] - 1;

					m_indexBuffer[numTriangles * 3] = v[0];
					m_indexBuffer[numTriangles * 3 + 1] = v[1];
					m_indexBuffer[numTriangles * 3 + 2] = v[2];
					numTriangles++;

                    v[1] = v[2];
                    vt[1] = vt[2];
      
					}
                }
			} 
			else if (sscanf(buffer, "%d/%d", &v[0], &vt[0]) == 2) // v/vt
            {
                fscanf(pFile, "%d/%d", &v[1], &vt[1]);
                fscanf(pFile, "%d/%d", &v[2], &vt[2]);

                v[0] = v[0] - 1;
                v[1] = v[1] - 1;
                v[2] = v[2] - 1;

                vt[0] = vt[0] - 1;
                vt[1] = vt[1] - 1;
                vt[2] = vt[2] - 1;

                m_indexBuffer[numTriangles * 3] = v[0];
				m_indexBuffer[numTriangles * 3 + 1] = v[1];
				m_indexBuffer[numTriangles * 3 + 2] = v[2];
				numTriangles++;

                v[1] = v[2];
                vt[1] = vt[2];

                while (fscanf(pFile, "%d/%d", &v[2], &vt[2]) > 0)
                {
                    v[2] = (v[2] < 0) ? v[2] + numVertices - 1 : v[2] - 1;
                    vt[2] = (vt[2] < 0) ? vt[2] + numTexCoords - 1 : vt[2] - 1;

                    m_indexBuffer[numTriangles * 3] = v[0];
					m_indexBuffer[numTriangles * 3 + 1] = v[1];
					m_indexBuffer[numTriangles * 3 + 2] = v[2];
					numTriangles++;

                    v[1] = v[2];
                    vt[1] = vt[2];
                }
            }
			break;

		case 'v': //v,vn,vt
			switch (buffer[1])
			{
			case '\0': //v
				
				fscanf(pFile,"%f %f %f",
					&m_vertexCoords[3 * numVertices],
                    &m_vertexCoords[3 * numVertices + 1],
                    &m_vertexCoords[3 * numVertices + 2]);
                ++numVertices;
				break;

			case 't':
				fscanf(pFile, "%f %f",
                    &m_textureCoords[2 * numTexCoords],
                    &m_textureCoords[2 * numTexCoords + 1]);
                ++numTexCoords;
				break;

			default:
				break;
			}
			break;

		default:
			fgets(buffer,sizeof(buffer),pFile);
			break;
		}
	}
	
}

const int ModelOBJ::getNbVertex() 
{ 
	return m_numberOfVertexCoords;
}

 const int ModelOBJ::getNbTriangles() 
{
	return m_numberOfTriangles;
 }