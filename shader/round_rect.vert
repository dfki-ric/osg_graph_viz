#version 120

varying vec4 modelVertex;

void main() {
  vec4 vModelPos = gl_Vertex;
  vec4 vViewPos = gl_ModelViewMatrix * vModelPos ;
  gl_Position = gl_ModelViewProjectionMatrix * vModelPos;
  gl_ClipVertex = vViewPos;
  modelVertex = vModelPos;
}
