#include "Headers/VertexBuffer.h"

namespace Divide {
VertexBuffer::VertexBuffer(GFXDevice& context)
    : VertexDataInterface(context),
     _format(GFXDataFormat::UNSIGNED_SHORT),
     _primitiveRestartEnabled(false),
     _staticBuffer(false),
     _keepDataInMemory(false)
{
    reset();
}

VertexBuffer::~VertexBuffer()
{
    reset();
}

bool VertexBuffer::create(bool staticDraw) {
    _staticBuffer = staticDraw;
    return true;
}

bool VertexBuffer::createInternal() {
    return true;
}


void VertexBuffer::computeNormals() {
    vec3<F32> v1, v2, normal;
    // Code from
    // http://devmaster.net/forums/topic/1065-calculating-normals-of-a-mesh/

    size_t vertCount = getVertexCount();
    size_t indexCount = getIndexCount();

    typedef vectorImpl<vec3<F32>> normalVector;

    vectorImpl<normalVector> normalBuffer(vertCount);
    for (U32 i = 0; i < indexCount; i += 3) {

        U32 idx0 = getIndex(i + 0);
        U32 idx1 = getIndex(i + 1);
        U32 idx2 = getIndex(i + 2);
        // get the three vertices that make the faces
        const vec3<F32>& p1 = getPosition(idx0);
        const vec3<F32>& p2 = getPosition(idx1);
        const vec3<F32>& p3 = getPosition(idx2);

        v1.set(p2 - p1);
        v2.set(p3 - p1);
        normal.cross(v1, v2);
        normal.normalize();

        // Store the face's normal for each of the vertices that make up the face.
        normalBuffer[idx0].push_back(normal);
        normalBuffer[idx1].push_back(normal);
        normalBuffer[idx2].push_back(normal);
    }

    // Now loop through each vertex vector, and average out all the normals
    // stored.
    vec3<F32> currentNormal;
    for (U32 i = 0; i < vertCount; ++i) {
        currentNormal.reset();
        for (U32 j = 0; j < normalBuffer[i].size(); ++j) {
            currentNormal += normalBuffer[i][j];
        }
        currentNormal /= to_float(normalBuffer[i].size());

        modifyNormalValue(i, currentNormal);
    }

    normalBuffer.clear();
}

void VertexBuffer::computeTangents() {
    size_t indexCount = getIndexCount();

    // Code from:
    // http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/#header-1

    vec3<F32> deltaPos1, deltaPos2;
    vec2<F32> deltaUV1, deltaUV2;
    vec3<F32> tangent;

    for (U32 i = 0; i < indexCount; i += 3) {
        // get the three vertices that make the faces
        U32 idx0 = getIndex(i + 0);
        U32 idx1 = getIndex(i + 1);
        U32 idx2 = getIndex(i + 2);

        const vec3<F32>& v0 = getPosition(idx0);
        const vec3<F32>& v1 = getPosition(idx1);
        const vec3<F32>& v2 = getPosition(idx2);

        // Shortcuts for UVs
        const vec2<F32>& uv0 = getTexCoord(idx0);
        const vec2<F32>& uv1 = getTexCoord(idx1);
        const vec2<F32>& uv2 = getTexCoord(idx2);

        // Edges of the triangle : position delta
        deltaPos1.set(v1 - v0);
        deltaPos2.set(v2 - v0);

        // UV delta
        deltaUV1.set(uv1 - uv0);
        deltaUV2.set(uv2 - uv0);

        F32 r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        tangent.set((deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r);

        // Set the same tangent for all three vertices of the triangle.
        // They will be merged later, in vbindexer.cpp
        modifyTangentValue(idx0, tangent);
        modifyTangentValue(idx1, tangent);
        modifyTangentValue(idx2, tangent);
    }
}

void VertexBuffer::fromBuffer(VertexBuffer& other) {
    reset();
    _staticBuffer = other._staticBuffer;
    _format = other._format;
    _partitions = other._partitions;
    _hardwareIndicesL = other._hardwareIndicesL;
    _hardwareIndicesS = other._hardwareIndicesS;
    _data = other._data;
    _primitiveRestartEnabled = other._primitiveRestartEnabled;
    _attribDirty = other._attribDirty;
}

bool VertexBuffer::deserialize(ByteBuffer& dataIn) {
    assert(!dataIn.empty());
    stringImpl idString;
    dataIn >> idString;
    if (idString.compare("VB") != 0) {
        return false;
    }

    reset();
    U32 format, data1, data2, count;

    dataIn >> _staticBuffer;
    dataIn >> _keepDataInMemory;
    dataIn >> format;
    _format = static_cast<GFXDataFormat>(format);

    dataIn >> count;
    _partitions.reserve(count);
    for (U32 i = 0; i < count; ++i) {
        dataIn >> data1;
        dataIn >> data2;
        _partitions.push_back(std::make_pair(data1, data2));
    }
    dataIn >> _hardwareIndicesL;
    dataIn >> _hardwareIndicesS;

    dataIn >> count;
    _data.resize(count);
    for (U32 i = 0; i < count; ++i) {
        Vertex& vert = _data[i];
        dataIn >> vert._colour;
        dataIn >> vert._indices.i;
        dataIn >> vert._normal;
        dataIn >> vert._position;
        dataIn >> vert._tangent;
        dataIn >> vert._texcoord;
        dataIn >> vert._weights;
    }

    for (bool& state : _attribDirty) {
        dataIn >> state;
    }

    dataIn >> _primitiveRestartEnabled;

    return true;
}

bool VertexBuffer::serialize(ByteBuffer& dataOut) const {
    if (!_data.empty()) {
        dataOut << stringImpl("VB");
        dataOut << _staticBuffer;
        dataOut << _keepDataInMemory;
        dataOut << to_uint(_format);
        dataOut << to_uint(_partitions.size());
        for (const std::pair<U32, U32>& partition : _partitions) {
            dataOut << partition.first;
            dataOut << partition.second;
        }

        dataOut << _hardwareIndicesL;
        dataOut << _hardwareIndicesS;

        dataOut << to_uint(_data.size());
        for (Vertex vert : _data) {
            dataOut << vert._colour;
            dataOut << vert._indices.i;
            dataOut << vert._normal;
            dataOut << vert._position;
            dataOut << vert._tangent;
            dataOut << vert._texcoord;
            dataOut << vert._weights;
        }

        for (bool state : _attribDirty) {
            dataOut << state;
        }

        dataOut << _primitiveRestartEnabled;

        return true;
    }
    return false;
}
};