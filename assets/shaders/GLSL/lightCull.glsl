-- Compute

uniform uint numDirLights;
uniform uint numPointLights;
uniform uint numSpotLights;

#include "lightInput.cmn"

#define NUM_THREADS_PER_TILE (FORWARD_PLUS_TILE_RES * FORWARD_PLUS_TILE_RES)
#define FLT_MAX 3.402823466e+38F

// shared = The value of such variables are shared between all invocations within a work group.
shared uint ldsLightIdxCounter;
shared uint ldsLightIdx[MAX_NUM_LIGHTS_PER_TILE];
shared uint ldsZMax;
shared uint ldsZMin;

layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D HiZBuffer;

vec4 ConvertProjToView(in mat4 invProjection, vec4 p)
{
    p = invProjection * p;
    p /= p.w;
    // FIXME: Added the following line, not needed in D3D11 or TiledForwardShading11 example. [2014-07-07]
    p.y = -p.y;
    return p;
}

// this creates the standard Hessian-normal-form plane equation from three points, 
// except it is simplified for the case where the first point is the origin
vec4 CreatePlaneEquation(vec4 b, vec4 c)
{
    vec4 n;

    // normalize(cross( b.xyz-a.xyz, c.xyz-a.xyz )), except we know "a" is the origin
    n.xyz = normalize(cross(b.xyz, c.xyz));

    // -(n dot a), except we know "a" is the origin
    n.w = 0;

    return n;
}

// point-plane distance, simplified for the case where the plane passes through the origin
float GetSignedDistanceFromPlane(in vec4 p, in vec4 eqn)
{
    // dot( eqn.xyz, p.xyz ) + eqn.w, , except we know eqn.w is zero (see CreatePlaneEquation above)
    return dot(eqn.xyz, p.xyz);
}

void CalculateMinMaxDepthInLds(uvec3 globalThreadIdx)
{
    float depth = textureLod(HiZBuffer, globalThreadIdx.xy, 0).r;
    uint z = floatBitsToUint(depth);

    if (depth != 0.f)
    {
        atomicMax(ldsZMax, z);
        atomicMin(ldsZMin, z);
    }
}

layout(local_size_x = FORWARD_PLUS_TILE_RES, local_size_y = FORWARD_PLUS_TILE_RES) in;

void main(void)
{
    uvec3 globalIdx = gl_GlobalInvocationID;
    uvec3 localIdx = gl_LocalInvocationID;
    uvec3 groupIdx = gl_WorkGroupID;

    uint localIdxFlattened = localIdx.x + localIdx.y * FORWARD_PLUS_TILE_RES;
    uint tileIdxFlattened = groupIdx.x + groupIdx.y * GetNumTilesX();

    if (localIdxFlattened == 0)
    {
        ldsZMin = 0xffffffff;
        ldsZMax = 0;
        ldsLightIdxCounter = 0;
    }
    barrier();

    vec4 frustumEqn[4];
    {
        // construct frustum for this tile
        uint pxm = FORWARD_PLUS_TILE_RES * groupIdx.x;
        uint pym = FORWARD_PLUS_TILE_RES * groupIdx.y;
        uint pxp = FORWARD_PLUS_TILE_RES * (groupIdx.x + 1);
        uint pyp = FORWARD_PLUS_TILE_RES * (groupIdx.y + 1);

        uint uWindowWidthEvenlyDivisibleByTileRes = FORWARD_PLUS_TILE_RES * GetNumTilesX();
        uint uWindowHeightEvenlyDivisibleByTileRes = FORWARD_PLUS_TILE_RES * GetNumTilesY();

        // four corners of the tile, clockwise from top-left
        mat4 invProjection = inverse(dvd_ProjectionMatrix);
        vec4 frustum[4];
        frustum[0] = ConvertProjToView(invProjection, vec4(pxm / float(uWindowWidthEvenlyDivisibleByTileRes) * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pym) / float(uWindowHeightEvenlyDivisibleByTileRes) * 2.0f - 1.0f, 1.0f, 1.0f));
        frustum[1] = ConvertProjToView(invProjection, vec4(pxp / float(uWindowWidthEvenlyDivisibleByTileRes) * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pym) / float(uWindowHeightEvenlyDivisibleByTileRes) * 2.0f - 1.0f, 1.0f, 1.0f));
        frustum[2] = ConvertProjToView(invProjection, vec4(pxp / float(uWindowWidthEvenlyDivisibleByTileRes) * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pyp) / float(uWindowHeightEvenlyDivisibleByTileRes) * 2.0f - 1.0f, 1.0f, 1.0f));
        frustum[3] = ConvertProjToView(invProjection, vec4(pxm / float(uWindowWidthEvenlyDivisibleByTileRes) * 2.0f - 1.0f, (uWindowHeightEvenlyDivisibleByTileRes - pyp) / float(uWindowHeightEvenlyDivisibleByTileRes) * 2.0f - 1.0f, 1.0f, 1.0f));

        // create plane equations for the four sides of the frustum, 
        // with the positive half-space outside the frustum (and remember, 
        // view space is left handed, so use the left-hand rule to determine 
        // cross product direction)
        for (uint i = 0; i < 4; i++)
        {
            frustumEqn[i] = CreatePlaneEquation(frustum[i], frustum[(i + 1) & 3]);
        }
    }

    barrier();

    // calculate the min and max depth for this tile, 
    // to form the front and back of the frustum

    float minZ = FLT_MAX;
    float maxZ = 0.0f;

    CalculateMinMaxDepthInLds(globalIdx);

    barrier();

    minZ = uintBitsToFloat(ldsZMin);
    maxZ = uintBitsToFloat(ldsZMax);

    // loop over the point lights and do a sphere vs. frustum intersection test

    uint lightIDXOffset = numDirLights;
    for (uint i = 0; i < numPointLights; i += NUM_THREADS_PER_TILE)
    {
        uint il = localIdxFlattened + i;

        if (il < numPointLights)
        {
            vec4 center = dvd_LightSource[il + lightIDXOffset]._positionWV;
            float r = center.w;
            // test if sphere is intersecting or inside frustum
            if (center.z + minZ < r && center.z - maxZ < r)
            {
                if ((GetSignedDistanceFromPlane(center, frustumEqn[0]) < r) &&
                    (GetSignedDistanceFromPlane(center, frustumEqn[1]) < r) &&
                    (GetSignedDistanceFromPlane(center, frustumEqn[2]) < r) &&
                    (GetSignedDistanceFromPlane(center, frustumEqn[3]) < r))
                {
                    // do a thread-safe increment of the list counter 
                    // and put the index of this light into the list
                    uint dstIdx = atomicAdd(ldsLightIdxCounter, 1);
                    ldsLightIdx[dstIdx] = il + 1;
                }
            }
        }
    }

    barrier();

    // Spot lights.
    lightIDXOffset += numPointLights;
    uint uNumPointLightsInThisTile = ldsLightIdxCounter;

    for (uint i = 0; i < numSpotLights; i += NUM_THREADS_PER_TILE)
    {
        uint il = localIdxFlattened + i;

        if (il < numSpotLights)
        {
            vec4 center = dvd_LightSource[il + lightIDXOffset]._positionWV;
            vec3 direction = dvd_LightSource[il + lightIDXOffset]._directionWV.xyz;
            float r = center.w * 0.5;
            center.xyz += direction * r;
            // test if sphere is intersecting or inside frustum
            if (center.z + minZ < r && center.z - maxZ < r)
            {
                if ((GetSignedDistanceFromPlane(center, frustumEqn[0]) < r) &&
                    (GetSignedDistanceFromPlane(center, frustumEqn[1]) < r) &&
                    (GetSignedDistanceFromPlane(center, frustumEqn[2]) < r) &&
                    (GetSignedDistanceFromPlane(center, frustumEqn[3]) < r))
                {
                    // do a thread-safe increment of the list counter 
                    // and put the index of this light into the list
                    uint dstIdx = atomicAdd(ldsLightIdxCounter, 1);
                    ldsLightIdx[dstIdx] = il + 1;
                }
            }
        }
    }
    
    barrier();

    {   // write back
        uint startOffset = dvd_numLightsPerTile * tileIdxFlattened;

        for (uint i = localIdxFlattened; i < uNumPointLightsInThisTile; i += NUM_THREADS_PER_TILE)
        {
            // per-tile list of light indices
            perTileLightIndices[startOffset + i] = ldsLightIdx[i];
        }

        for (uint j = (localIdxFlattened + uNumPointLightsInThisTile); j < ldsLightIdxCounter; j += NUM_THREADS_PER_TILE)
        {
            // per-tile list of light indices
            perTileLightIndices[startOffset + j + 1] = ldsLightIdx[j];
        }

        if (localIdxFlattened == 0)
        {
            // mark the end of each per-tile list with a sentinel (point lights)
            perTileLightIndices[startOffset + uNumPointLightsInThisTile] = LIGHT_INDEX_BUFFER_SENTINEL;

            // mark the end of each per-tile list with a sentinel (spot lights)
            perTileLightIndices[startOffset + ldsLightIdxCounter + 1] = LIGHT_INDEX_BUFFER_SENTINEL;
        }
    }
}