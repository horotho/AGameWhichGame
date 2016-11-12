#include "resources.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Text.h"
#include "cinder/Timer.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

//======================================================================================================================
class cKeyHandler {
public:
  void Handle(KeyEvent event, bool isDown) {
    if (event.getCode() == KeyEvent::KEY_UP) {
      Up = isDown;
    }
    if (event.getCode() == KeyEvent::KEY_LEFT) {
      Left = isDown;
    }
    if (event.getCode() == KeyEvent::KEY_RIGHT) {
      Right = isDown;
    }
    if (event.getCode() == KeyEvent::KEY_DOWN) {
      Down = isDown;
    }
  }

  bool Left{false};
  bool Right{false};
  bool Up{false};
  bool Down{false};
};

//======================================================================================================================
class cEntity {
public:
  static const uint32_t k_DefaultSpeed = 8;

  //======================================================================================================================
  cEntity() = default;

  //======================================================================================================================
  void Draw(gl::BatchRef batch) {
    gl::pushModelMatrix();
    gl::ScopedColor color(CurrentColor);
    gl::translate(Position);
    batch->draw();
    gl::popModelMatrix();
  }

  //======================================================================================================================
  void Update(cKeyHandler const &keys) {
    auto const k_WindowSize = getWindowSize();
    if (keys.Right) {
      Position.x = (Position.x + Radius >= k_WindowSize.x) ? Position.x : (Position.x + k_DefaultSpeed);
    }

    if (keys.Left) {
      Position.x = (Position.x - Radius <= 0) ? Position.x : (Position.x - k_DefaultSpeed);
    }

    if (keys.Up) {
      Position.y = (Position.y - Radius <= 0) ? Position.y : (Position.y - k_DefaultSpeed);
    }

    if (keys.Down) {
      Position.y = (Position.y + Radius >= k_WindowSize.y) ? Position.y : (Position.y + k_DefaultSpeed);
    }
  }

  vec2 Position{0, 0};
  float Radius{30.0f};
  Color8u CurrentColor{239, 225, 71};
};

//======================================================================================================================
class cGame : public App {
public:
  static const Color k_BackgroundColor;
  void setup() override;
  void update() override;
  void draw() override;
  void keyDown(KeyEvent) override;
  void keyUp(KeyEvent) override;

private:
  // mesh and texture
  gl::VboMeshRef Mesh;
  gl::BatchRef Batch;
  gl::TextureRef Texture;

  // Default shader
  gl::GlslProgRef Shader;

  cKeyHandler Keys;
  cEntity Player;
};

const Color cGame::k_BackgroundColor = Color(0.11f, 0.112f, 0.11f);

//======================================================================================================================
void cGame::keyDown(KeyEvent event) { Keys.Handle(event, true); }

//======================================================================================================================
void cGame::keyUp(KeyEvent event) { Keys.Handle(event, false); }

//======================================================================================================================
void cGame::setup() {
  getWindow()->setTitle("A GAME");
  // allow maximum frame rate
  disableFrameRate();
  gl::enableVerticalSync(false);

  // BEGIN CODE I KNOW NOTHING ABOUT
  
  // create a default shader with color and texture support
  Shader = gl::context()->getStockShader(gl::ShaderDef().color().texture());

  // create ball mesh ( much faster than using gl::drawSolidCircle() )
  size_t const slices = 20;

  std::vector<vec3> positions;
  std::vector<vec2> texcoords;
  std::vector<uint8_t> indices;

  texcoords.push_back(vec2(0.5f, 0.5f));
  positions.push_back(vec3(0));

  for (size_t i = 0; i <= slices; ++i) {
    float angle = i / (float)slices * 2.0f * (float)M_PI;
    vec2 v(sinf(angle), cosf(angle));

    texcoords.push_back(vec2(0.5f, 0.5f) + 0.5f * v);
    positions.push_back(Player.Radius * vec3(v, 0.0f));
  }

  gl::VboMesh::Layout layout;
  layout.usage(GL_STATIC_DRAW);
  layout.attrib(geom::Attrib::POSITION, 3);
  layout.attrib(geom::Attrib::TEX_COORD_0, 2);

  Mesh = gl::VboMesh::create(positions.size(), GL_TRIANGLE_FAN, {layout});
  Mesh->bufferAttrib(geom::POSITION, positions.size() * sizeof(vec3),
                     positions.data());
  Mesh->bufferAttrib(geom::TEX_COORD_0, texcoords.size() * sizeof(vec2),
                     texcoords.data());

  // combine mesh and shader into batch for much better performance
  Batch = gl::Batch::create(Mesh, Shader);

  // END CODE I KNOW NOTHING ABOUT
  
  // load texture
  Texture = gl::Texture::create(loadImage(loadAsset(BALL_TEXTURE)));

  Player.Position = getWindowCenter();
}

//======================================================================================================================
void cGame::update() {
  // Use a fixed time step for a steady 60 updates per second.
  static const double timestep = 1.0 / 60.0;

  // Keep track of time.
  static double time = getElapsedSeconds();
  static double accumulator = 0.0;

  // Calculate elapsed time since last frame.
  double elapsed = getElapsedSeconds() - time;
  time += elapsed;

  // Update the simulation.
  accumulator += math<double>::min(elapsed, 0.1); // prevents 'spiral of death'
  while (accumulator >= timestep) {
    accumulator -= timestep;
    Player.Update(Keys);
  }
}

//======================================================================================================================
void cGame::draw() {
  gl::clear(k_BackgroundColor);

  gl::ScopedBlendAdditive blend;
  gl::ScopedGlslProg shader(Shader);
  Shader->uniform("uTex0", 0);

  gl::ScopedTextureBind tex0(Texture);

  Player.Draw(Batch);
}


//======================================================================================================================
// This is a magical macro that sets up our 2D world and runs our app!
//======================================================================================================================
CINDER_APP(cGame, RendererGl)

