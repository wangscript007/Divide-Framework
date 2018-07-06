-- Vertex
in vec3  inVertexData;

uniform mat4 dvd_WorldViewProjectionMatrix;
uniform mat4 dvd_WorldMatrix;

layout(std140) uniform dvd_MatrixBlock
{
    mat4 dvd_ProjectionMatrix;
    mat4 dvd_ViewMatrix;
	mat4 dvd_ViewProjectionMatrix;
};

out vec3 _vertex;

void main(void){
    vec4 dvd_Vertex = vec4(inVertexData,1.0);
    _vertex = normalize(dvd_Vertex.xyz);
    gl_Position = dvd_ViewProjectionMatrix * dvd_WorldMatrix * dvd_Vertex;
}

-- Fragment

in vec3 _vertex;
out vec4 _skyColor;

uniform bool enable_sun;
uniform vec3 sun_vector;

uniform samplerCube texSky;


void main (void){

    vec4 sky_color = texture(texSky, _vertex.xyz);
    _skyColor = sky_color;

    if(enable_sun){

        vec3 vert = normalize(_vertex);
        vec3 sun = normalize(sun_vector);
        
        float day_factor = max(-sun.y, 0.0);
        
        float dotv = max(dot(vert, -sun), 0.0);
        vec4 sun_color = clamp(gl_LightSource[0].diffuse*1.5, 0.0, 1.0);
    
        float pow_factor = day_factor * 175.0 + 25.0;
        float sun_factor = clamp(pow(dotv, pow_factor), 0.0, 1.0);
        
        _skyColor *= day_factor + sun_color * sun_factor;
    
    }

    _skyColor.a = 1.0;
}