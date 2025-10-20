#version 330 core

// Specify the input locations of attributes
layout(location = 0) in vec3 vertCoordinates_in;
layout(location = 1) in vec3 vertNormal_in;

uniform mat4 modelViewTransform;
uniform mat3 normalTransform;
uniform mat4 projectionTransform;
uniform float outlineWidth;

void main() {
  vec3 normal = normalize(normalTransform * vertNormal_in);
  vec3 displacedPosition = vertCoordinates_in + normal * outlineWidth;
  gl_Position = projectionTransform * modelViewTransform * vec4(displacedPosition, 1.0F);
}