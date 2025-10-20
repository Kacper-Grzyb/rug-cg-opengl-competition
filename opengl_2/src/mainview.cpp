#include "mainview.h"

#include <QDateTime>

/**
 * @brief MainView::MainView Constructs a new main view.
 *
 * @param parent Parent widget.
 */
MainView::MainView(QWidget *parent) : QOpenGLWidget(parent) {
  qDebug() << "MainView constructor";

  materialColor = QVector3D(1.0f, 1.0f, 1.0f);
  lightColor= QVector3D(1.0f, 1.0f, 1.0f);
  lightPosition = QVector3D(100.0f, 50.0f, 0.0f);

  connect(&timer, SIGNAL(timeout()), this, SLOT(update()));

  // Loading the texture image
  textureQimage = QImage(":/textures/cat_diff.png");
  textureImage = imageToBytes(textureQimage);
}

/**
 * @brief MainView::~MainView
 *
 * Destructor of MainView
 * This is the last function called, before exit of the program
 * Use this to clean up your variables, buffers etc.
 */
MainView::~MainView() {
  qDebug() << "MainView destructor";

  makeCurrent();
  destroyModelBuffers();
  glDeleteTextures(1, &textureName);
}

// --- OpenGL initialization

/**
 * @brief MainView::initializeGL Called upon OpenGL initialization
 * Attaches a debugger and calls other init functions.
 */
void MainView::initializeGL() {
  qDebug() << ":: Initializing OpenGL";
  initializeOpenGLFunctions();

  connect(&debugLogger, SIGNAL(messageLogged(QOpenGLDebugMessage)), this,
          SLOT(onMessageLogged(QOpenGLDebugMessage)), Qt::DirectConnection);

  if (debugLogger.initialize()) {
    qDebug() << ":: Logging initialized";
    debugLogger.startLogging(QOpenGLDebugLogger::SynchronousLogging);
  }

  QString glVersion{reinterpret_cast<const char *>(glGetString(GL_VERSION))};
  qDebug() << ":: Using OpenGL" << qPrintable(glVersion);

  // Enable depth buffer
  glEnable(GL_DEPTH_TEST);

  // Enable backface culling
  glEnable(GL_CULL_FACE);

  // Default is GL_LESS
  glDepthFunc(GL_LEQUAL);

  // Set the color to be used by glClear. This is, effectively, the background
  // color.
  glClearColor(0.37f, 0.42f, 0.45f, 0.0f);

  // Texture setup
  glGenTextures(1, &textureName);
  glBindTexture(GL_TEXTURE_2D, textureName);
  // Setting texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Uploading texture data
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA8,
    textureQimage.width(),
    textureQimage.height(),
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    textureImage.data()
  );

  createShaderProgram();
  loadMesh(":/models/cat.obj");

  // Initialize transformations
  updateProjectionTransform();
  updateModelTransforms();
}

/**
 * @brief MainView::createShaderProgram Creates a new shader program with a
 * vertex and fragment shader.
 */
void MainView::createShaderProgram() {
  // Create toon shader program
  toonShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                        ":/shaders/toon_vertshader.glsl");
  toonShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                        ":/shaders/toon_fragshader.glsl");
  toonShaderProgram.link();
  
  // Create outline shader program
  outlineShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                        ":/shaders/outline_vertshader.glsl");
  outlineShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                        ":/shaders/outline_fragshader.glsl");
  outlineShaderProgram.link();
}

/**
 * @brief MainView::loadMesh Loads a mesh from a given file. Note that only one
 * mesh can be loaded at a time.
 * @param filename Filename of where the mesh is located.
 */
void MainView::loadMesh(const QString &filename) {
  Model model(filename);
  QVector<QVector3D> meshCoords = model.getCoords();
  QVector<QVector3D> meshNormals = model.getNormals();
  QVector<QVector2D> textureCoords = model.getTextureCoords();


  meshSize = meshCoords.size();

  // Generate VAO
  glGenVertexArrays(1, &meshVAO);
  glBindVertexArray(meshVAO);

  // Generate VBO
  glGenBuffers(1, &meshPositionVBO);
  glGenBuffers(1, &meshColorVBO);
  glGenBuffers(1, &meshTextureVBO);

  // Copy the vertex coordinates into the position VBO
  glBindBuffer(GL_ARRAY_BUFFER, meshPositionVBO);
  glBufferData(GL_ARRAY_BUFFER, meshCoords.size() * sizeof(QVector3D),
               meshCoords.data(), GL_STATIC_DRAW);

  // Set vertex coordinates to location 0
  // Note: glVertexAttribPointer implicitly reference the VBO currently bound to GL_ARRAY_BUFFER
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D),
                        reinterpret_cast<GLvoid *>(0));
  glEnableVertexAttribArray(0);

  // Copy the colors into the color VBO
  glBindBuffer(GL_ARRAY_BUFFER, meshColorVBO);
  glBufferData(GL_ARRAY_BUFFER, meshNormals.size() * sizeof(QVector3D),
               meshNormals.data(), GL_STATIC_DRAW);

  // Set color values to location 1
  // Note: glVertexAttribPointer implicitly reference the VBO currently bound to GL_ARRAY_BUFFER
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D),
                        reinterpret_cast<GLvoid *>(0));
  glEnableVertexAttribArray(1);

  // Copy the texture coordinates into the position VBO
  glBindBuffer(GL_ARRAY_BUFFER, meshTextureVBO);
  glBufferData(GL_ARRAY_BUFFER, textureCoords.size() * sizeof(QVector2D),
              textureCoords.data(), GL_STATIC_DRAW);

  // Set texture coordiantes to location 2
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(QVector2D),
                        reinterpret_cast<GLvoid *>(0));
  glEnableVertexAttribArray(2);

  // Generally good practice to unbind the buffers to prevent anything after
  // this from accidentally modifying it.
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

// --- OpenGL drawing

/**
 * @brief MainView::paintGL Actual function used for drawing to the screen.
 *
 */
void MainView::paintGL() {
  // Clear the screen before rendering
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Bind the VAO once
  glBindVertexArray(meshVAO);

  // Pass 1: Draw the outline
  outlineShaderProgram.bind();
  
  glCullFace(GL_FRONT); 

  // Set Uniforms for the outline pass
  outlineShaderProgram.setUniformValue("modelViewTransform", meshTransform);
  outlineShaderProgram.setUniformValue("projectionTransform", projectionTransform);
  outlineShaderProgram.setUniformValue("normalTransform", normalTransform);
  outlineShaderProgram.setUniformValue("outlineWidth", outlineWidth); 

  glDrawArrays(GL_TRIANGLES, 0, meshSize);
  outlineShaderProgram.release();
  
  // Pass 2: Draw the model
  toonShaderProgram.bind();
  
  glCullFace(GL_BACK); 
  
  // Set Uniforms for the main shading pass
  toonShaderProgram.setUniformValue("modelViewTransform", meshTransform);
  toonShaderProgram.setUniformValue("normalTransform", normalTransform);
  toonShaderProgram.setUniformValue("projectionTransform", projectionTransform);
  toonShaderProgram.setUniformValue("viewPosition", viewPosition);
  toonShaderProgram.setUniformValue("lightPosition", lightPosition);
  toonShaderProgram.setUniformValue("lightColor", lightColor);
  toonShaderProgram.setUniformValue("materialColor", materialColor);
  toonShaderProgram.setUniformValue("samplerUniform", 0);
  toonShaderProgram.setUniformValue("specular", specular);

  // Bind the texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureName);

  glDrawArrays(GL_TRIANGLES, 0, meshSize);

  toonShaderProgram.release();
  
  // Unbind VAO
  glBindVertexArray(0);
}

/**
 * @brief MainView::resizeGL Called upon resizing of the screen.
 *
 * @param newWidth The new width of the screen in pixels.
 * @param newHeight The new height of the screen in pixels.
 */
void MainView::resizeGL(int newWidth, int newHeight) {
  Q_UNUSED(newWidth)
  Q_UNUSED(newHeight)
  updateProjectionTransform();
}

/**
 * @brief MainView::updateProjectionTransform Updates the projection transform
 * matrix taking into consideration the current aspect ratio.
 */
void MainView::updateProjectionTransform() {
  float aspectRatio =
      static_cast<float>(width()) / static_cast<float>(height());
  projectionTransform.setToIdentity();
  projectionTransform.perspective(60.0F, aspectRatio, 0.2F, 20.0F);
}

/**
 * @brief MainView::updateModelTransforms Updates the model transform matrix of
 * the mesh to reflect the current rotation and scale values.
 */
void MainView::updateModelTransforms() {
  meshTransform.setToIdentity();
  meshTransform.translate(0.0F, 0.0F, -10.0F);
  meshTransform.rotate(QQuaternion::fromEulerAngles(rotation));
  meshTransform.scale(scale);
  normalTransform = meshTransform.normalMatrix();
  update();
}

/**
 * @brief MainView::destroyModelBuffers Cleans up the memory used by OpenGL.
 */
void MainView::destroyModelBuffers() {
  glDeleteBuffers(1, &meshPositionVBO);
  glDeleteBuffers(1, &meshColorVBO);
  glDeleteVertexArrays(1, &meshVAO);
}

/**
 * @brief MainView::setRotation Changes the rotation of the displayed objects.
 * @param rotateX Number of degrees to rotate around the x axis.
 * @param rotateY Number of degrees to rotate around the y axis.
 * @param rotateZ Number of degrees to rotate around the z axis.
 */
void MainView::setRotation(int rotateX, int rotateY, int rotateZ) {
  rotation = {static_cast<float>(rotateX), static_cast<float>(rotateY),
              static_cast<float>(rotateZ)};
  updateModelTransforms();
}

/**
 * @brief MainView::setScale Changes the scale of the displayed objects.
 * @param scale The new scale factor. A scale factor of 1.0 should scale the
 * mesh to its original size.
 */
void MainView::setScale(float newScale) {
  scale = newScale;
  updateModelTransforms();
}

void MainView::setShadingMode(ShadingMode shading) {
  qDebug() << "Changed shading to" << shading;
  Q_UNIMPLEMENTED();
}

/**
 * @brief MainView::onMessageLogged OpenGL logging function, do not change.
 *
 * @param Message The message to be logged.
 */
void MainView::onMessageLogged(QOpenGLDebugMessage Message) {
  qDebug() << " → Log:" << Message;
}
