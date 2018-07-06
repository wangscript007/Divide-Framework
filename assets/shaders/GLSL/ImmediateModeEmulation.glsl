-- Vertex

in vec2 inTexCoordData;
in vec4 inColorData;
in vec3 inVertexData;

out vec2 _texCoord;
out vec4 _color;

uniform mat4 dvd_WorldViewProjectionMatrix;

void main(){
  _texCoord = inTexCoordData;
  _color = inColorData;
  gl_Position = dvd_WorldViewProjectionMatrix * vec4(inVertexData,1.0);
} 

-- Vertex.GUI

in vec2 inTexCoordData;
in vec4 inColorData;
in vec3 inVertexData;

out vec2 _texCoord;
out vec3 _color;

layout(std140) uniform dvd_MatrixBlock
{
    mat4 dvd_ProjectionMatrix;
    mat4 dvd_ViewMatrix;
	mat4 dvd_ViewProjectionMatrix;
};

void main(){
  _texCoord = inTexCoordData;
  _color = inColorData.rgb;
  gl_Position = dvd_ProjectionMatrix * vec4(inVertexData,1.0);
} 

-- Fragment

uniform sampler2D tex;
uniform bool useTexture;

in  vec2 _texCoord;
in  vec4 _color;
out vec4 _colorOut;

void main(){
	if(!useTexture){
		_colorOut = _color;
	}else{
		_colorOut = texture(tex, _texCoord);
		_colorOut.rgb += _color.rgb;
	}
}

-- Fragment.GUI

uniform sampler2D tex;

in  vec2 _texCoord;
in  vec3 _color;

out vec4 _colorOut;

void main(){
	_colorOut = vec4(_color, texture(tex, _texCoord).r);
}