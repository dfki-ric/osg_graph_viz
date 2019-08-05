#version 120

varying vec4 modelVertex;
uniform vec3 start;
uniform vec3 end;

void main() {
  vec4 vModelPos = gl_Vertex;
  vec4 vViewPos = gl_ModelViewMatrix * vModelPos ;
  gl_Position = gl_ModelViewProjectionMatrix * vModelPos;
  gl_ClipVertex = vViewPos;
  modelVertex = vModelPos;
  modelVertex -= vec4(start, 0.0);
  vec4 foo = vec4(end-start, 0.0);
  modelVertex /= abs(foo.y);
  modelVertex.y = abs(modelVertex.y);
  modelVertex.x = abs(modelVertex.x);
  modelVertex.z = abs(1/(end.y - start.y));
  modelVertex.w = abs(foo.y)/abs(foo.x);
}
