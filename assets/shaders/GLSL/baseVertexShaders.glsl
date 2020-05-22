-- Vertex.FullScreenQuad

void main(void)
{
    vec2 uv = vec2(0, 0);
    if ((gl_VertexID & 1) != 0)uv.x = 1;
    if ((gl_VertexID & 2) != 0)uv.y = 1;

    VAR._texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0, 1);
}

-- Vertex.Dummy

void main(void)
{
}

-- Vertex.BasicData

#include "vbInputData.vert"

void main(void)
{
    computeData();
    gl_Position = VAR._vertexWVP;
}

--Vertex.BasicLightData

#include "vbInputData.vert"
#include "lightingDefaults.vert"

void main() {

    computeData();
    computeLightVectors(dvd_NormalMatrixW(DATA_IDX), dvd_NormalMatrixWV(DATA_IDX));

    gl_Position = VAR._vertexWVP;
}
