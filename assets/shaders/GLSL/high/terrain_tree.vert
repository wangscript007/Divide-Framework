varying vec4 vPixToLightTBN[1];
varying vec3 vPixToEyeTBN;
varying vec3 vVertexMV;
varying vec3 vPixToLightMV;
varying vec3 vLightDirMV;
varying vec4 texCoord[2];

uniform int enable_shadow_mapping;

uniform float time;
uniform vec3  scale;
uniform float windDirectionX;
uniform float windDirectionZ;
uniform float windSpeed;

uniform mat4 modelViewInvMatrix;
uniform mat4 lightProjectionMatrix;
uniform mat4 modelViewProjectionMatrix;

#define LIGHT_DIRECTIONAL		0.0
#define LIGHT_OMNIDIRECTIONAL	1.0
#define LIGHT_SPOT				2.0
				
void main(void){

	
	
	vec4 vertex = gl_Vertex;
	vec4 vertexMV = gl_ModelViewMatrix * vertex;
	vec3 normalMV = gl_NormalMatrix * gl_Normal;
	
	vec4 vertexM = gl_TextureMatrix[0] * gl_ModelViewMatrix * vec4(0.0, 0.0, 0.0, 1.0);
	vertex.x += 0.01 * cos(time*windSpeed) * cos(vertex.x) * sin(vertex.x) *windDirectionX;
	vertex.z += 0.01 * sin(time*windSpeed) * cos(vertex.x) * sin(vertex.x) *windDirectionZ;

	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	
	texCoord[0].st = gl_MultiTexCoord0.xy;
	
	vec3 vTangent = gl_MultiTexCoord0.xyz;
	vec3 n = normalize(gl_NormalMatrix * gl_Normal);
	vec3 t = normalize(gl_NormalMatrix * vTangent);
	vec3 b = cross(t, n);
	
	vec4 vLightPosMV = gl_LightSource[0].position;	
	vVertexMV = vec3(gl_ModelViewMatrix * gl_Vertex);	
	
	vec3 tmpVec;

	if(vLightPosMV.w == LIGHT_DIRECTIONAL)
		tmpVec = -vLightPosMV.xyz;					
	else
		tmpVec = vLightPosMV.xyz - vVertexMV.xyz;	

	vPixToLightMV = tmpVec;

	vPixToLightTBN[0].x = dot(tmpVec, t);
	vPixToLightTBN[0].y = dot(tmpVec, b);
	vPixToLightTBN[0].z = dot(tmpVec, n);
	// Point or directional?
	vPixToLightTBN[0].w = vLightPosMV.w;	
		
	//View vector
	tmpVec = -vVertexMV;
	vPixToEyeTBN.x = dot(tmpVec, t);
	vPixToEyeTBN.y = dot(tmpVec, b);
	vPixToEyeTBN.z = dot(tmpVec, n);
	
	
	if(length(gl_LightSource[0].spotDirection) > 0.001)	{
		// Spot light
		vLightDirMV = normalize(gl_LightSource[0].spotDirection);
		vPixToLightTBN[0].w = LIGHT_SPOT;
	}else{
		// Not a spot light
		vLightDirMV = gl_LightSource[0].spotDirection;
	}
	

	if(enable_shadow_mapping != 0) {
		// Transformed position 
		vec4 pos = gl_ModelViewMatrix * gl_Vertex;
		// position multiplied by the inverse of the camera matrix
		pos = modelViewInvMatrix * pos;
		// position multiplied by the light matrix. The vertex's position from the light's perspective
		texCoord[1] = lightProjectionMatrix * pos;
	}	
}