#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QKeyEvent>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QOpenGLDebugLogger>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QTimer>
#include <QVector3D>

#include "model.h"
#include "shadingmode.h"

/**
 * @brief The MainView class is resonsible for the actual content of the main
 * window.
 */
class MainView : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
  Q_OBJECT

 public:
  MainView(QWidget *parent = nullptr);
  ~MainView() override;

  // Functions for widget input events
  void setRotation(int rotateX, int rotateY, int rotateZ);
  void setScale(float scale);
  void setShadingMode(ShadingMode shading);

 protected:
  void initializeGL() override;
  void resizeGL(int newWidth, int newHeight) override;
  void paintGL() override;

  // Functions for keyboard input events
  void keyPressEvent(QKeyEvent *ev) override;
  void keyReleaseEvent(QKeyEvent *ev) override;

  // Function for mouse input events
  void mouseDoubleClickEvent(QMouseEvent *ev) override;
  void mouseMoveEvent(QMouseEvent *ev) override;
  void mousePressEvent(QMouseEvent *ev) override;
  void mouseReleaseEvent(QMouseEvent *ev) override;
  void wheelEvent(QWheelEvent *ev) override;

 private slots:
  void onMessageLogged(QOpenGLDebugMessage Message);

 private:
  void createShaderProgram();
  void loadMesh(const QString &filename);
  void destroyModelBuffers();
  void updateProjectionTransform();
  void updateModelTransforms();
  QVector<quint8> imageToBytes(const QImage &image);

  QOpenGLDebugLogger debugLogger;
  QTimer timer;  // timer used for animation

  QOpenGLShaderProgram toonShaderProgram;
  QOpenGLShaderProgram outlineShaderProgram;
  QVector3D viewPosition = QVector3D(0.0F, 0.0F, 0.0F);
  QVector3D lightPosition = QVector3D(100.0F, 50.0F, 0.0F);
  QVector3D lightColor = QVector3D(1.0F, 1.0F, 1.0F);
  QVector3D materialColor = QVector3D(1.0F, 1.0F, 1.0F);
  QVector<quint8> textureImage;
  QImage textureQimage;
  unsigned specular = 32;
  float outlineWidth = 0.06F;

  // Mesh values
  GLuint meshVAO;
  GLuint meshPositionVBO, meshColorVBO, meshTextureVBO;
  GLuint meshSize;
  GLuint textureName;
  QMatrix4x4 meshTransform;
  QMatrix3x3 normalTransform; 
  GLint texPtr;

  // Transforms
  float scale = 1.0F;
  QVector3D rotation;
  QMatrix4x4 projectionTransform;
};

#endif  // MAINVIEW_H
