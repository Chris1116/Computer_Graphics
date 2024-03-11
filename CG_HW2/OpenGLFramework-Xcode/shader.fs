#version 330 core

out vec4 FragColor;
in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 vertex_pos;

const float PI = 3.14159265359;

uniform vec3 cameraPosition;

struct PhongMaterial
{
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
};
uniform PhongMaterial material;

struct LightInfo
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};
uniform LightInfo lightSources;

struct SpotLightInfo
{
    vec3 spotDirection;
    float spotExponent;
    float spotCutOff;
};
uniform SpotLightInfo spotLightInfo;

uniform int curLightMode;
uniform float shininess;
uniform int isPerPixLighting;

vec3 directionalLight(vec3 vertexPosition, vec3 vertexNormal)
{
    vec3 L = normalize(lightSources.position);
    vec3 V = normalize(cameraPosition - vertexPosition);
    vec3 H = normalize(L + V);
    vec3 N = normalize(vertexNormal);

    vec3 ambient = lightSources.ambient * material.Ka;
    vec3 diffuse = max(dot(L, N), 0.0f) * lightSources.diffuse * material.Kd;
    vec3 specular = pow(max(dot(H, N), 0.0f), shininess) * lightSources.specular * material.Ks;
    return ambient + diffuse + specular;
}

vec3 positionLight(vec3 vertexPosition, vec3 vertexNormal)
{
    vec3 L = normalize(lightSources.position - vertexPosition);
    vec3 V = normalize(cameraPosition - vertexPosition);
    vec3 H = normalize(L + V);
    vec3 N = normalize(vertexNormal);

    vec3 ambient = lightSources.ambient * material.Ka;
    vec3 diffuse = max(dot(L, N), 0.0f) * lightSources.diffuse * material.Kd;
    vec3 specular = pow(max(dot(H, N), 0.0f), shininess) * lightSources.specular * material.Ks;

    float dist = length(lightSources.position - vertexPosition);
    float attenuation = lightSources.constantAttenuation +
    lightSources.linearAttenuation * dist +
    lightSources.quadraticAttenuation * dist * dist;
    float f_att = min(1.0f / attenuation, 1.0f);
    return ambient + f_att * (diffuse + specular);
}

vec3 spotLight(vec3 vertexPosition, vec3 vertexNormal)
{
    vec3 L = normalize(lightSources.position - vertexPosition);
    vec3 V = normalize(cameraPosition - vertexPosition);
    vec3 H = normalize(L + V);
    vec3 N = normalize(vertexNormal);

    vec3 ambient = lightSources.ambient * material.Ka;
    vec3 diffuse = max(dot(L, N), 0.0f) * lightSources.diffuse * material.Kd;
    vec3 specular = pow(max(dot(H, N), 0.0f), shininess) * lightSources.specular * material.Ks;

    float dist = length(lightSources.position - vertexPosition);
    float attenuation = lightSources.constantAttenuation +
    lightSources.linearAttenuation * dist +
    lightSources.quadraticAttenuation * dist * dist;
    float f_att = min(1.0f / attenuation, 1.0f);

    vec3 v = normalize(vertexPosition - lightSources.position);
    vec3 d = normalize(spotLightInfo.spotDirection);
    float vd = dot(v, d);
    float spotEffect = 0;
    if (vd > cos(spotLightInfo.spotCutOff * PI / 180.0f))
        spotEffect = pow(max(vd, 0.0f), spotLightInfo.spotExponent);
    return ambient + spotEffect * f_att * (diffuse + specular);
}

void main() {
    // [TODO]
    vec3 color;
    if (curLightMode == 0)
       color = directionalLight(vertex_pos, vertex_normal);
    else if (curLightMode == 1)
       color = positionLight(vertex_pos, vertex_normal);
    else if (curLightMode == 2)
       color = spotLight(vertex_pos, vertex_normal);
    FragColor = (isPerPixLighting == 1) ? vec4(color, 1.0f) : vec4(vertex_color, 1.0f);
    //FragColor = vec4(vertex_normal, 1.0f);
}
