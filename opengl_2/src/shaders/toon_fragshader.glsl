#version 330 core

// Define constants
#define M_PI 3.141593
#define NUM_STEPS 4.0

// Specify the inputs to the fragment shader
// These must have the same type and name!
in vec3 worldPosition;
in vec3 vertNormal;
in vec2 textureCoordinates;

// Specify the Uniforms of the fragment shaders
uniform vec3 viewPosition;
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 materialColor;
uniform sampler2D samplerUniform;
uniform int specular;

// Specify the output of the fragment shader
// Usually a vec4 describing a color (Red, Green, Blue, Alpha/Transparency)
out vec4 fColor;

void main() {
  vec3 N = normalize(vertNormal);
  vec3 L = normalize(lightPosition - worldPosition);
  vec3 V = normalize(viewPosition - worldPosition);
  vec3 textureColor = texture(samplerUniform, textureCoordinates).rgb;
  // vec3 textureColor = materialColor; // Textures not displaying the way I imagined they would :(

  // Diffuse quantization
  float diffuseIntensity = max(dot(N, L), 0.0F);
  float steppedIntensity = floor(diffuseIntensity * NUM_STEPS);
  float quantizedDiffuse = steppedIntensity / NUM_STEPS;

  // Specular quantization
  vec3 H = normalize(L + V);
  float rawSpecular = max(dot(N, H), 0.0F);
  float specularIntensity = pow(rawSpecular, specular);
  float quantizedSpecular = step(0.9, specularIntensity);

  // Color components
  vec3 shadowColor = textureColor * 0.3F;
  vec3 litColor = textureColor * lightColor;

  // Final calculations
  vec3 finalColor = mix(shadowColor, litColor, quantizedDiffuse);
  if(quantizedDiffuse > 0.0) {
    finalColor += lightColor * quantizedSpecular;
  }
  fColor = vec4(finalColor, 1.0F);

}