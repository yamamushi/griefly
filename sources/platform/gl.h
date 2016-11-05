#if defined(_WIN32) || defined(_WIN64)
 #include <gl\GL.h>
 #include <gl\GLU.h>
#endif // Windows

#if defined(__linux__)
 #include <GL/gl.h>
 #include <GL/glu.h>
#endif // Linux

#if defined(__APPLE__)
 #include <OpenGL/gl.h>
 #include <OpenGL/glu.h>
// #include <GLUT/glut.h>
#endif // Apple