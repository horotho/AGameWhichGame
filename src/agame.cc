//======================================================================================================================
// Overview
//======================================================================================================================
// The main file for A GAME, which contains the entry point to all things kewl.
// A lot of the stuff in this file can be extracted to another file, but for
// early prototyping it's all here.

//======================================================================================================================
// Includes
//======================================================================================================================
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
#include "resources.h"

//======================================================================================================================
// Namespaces
//======================================================================================================================
using namespace ci;
using namespace ci::app;
using namespace std;

//======================================================================================================================
class cKeyHandler {
public:
  void Handle(KeyEvent event, bool isDown) {
    auto const k_code = event.getCode();

    // Escape is only on key up
    Escape = k_code == KeyEvent::KEY_ESCAPE && !isDown;

    // All of these are really 'is key being held down'
    if (k_code == KeyEvent::KEY_UP) { Up = isDown; }
    if (k_code == KeyEvent::KEY_LEFT) { Left = isDown; }
    if (k_code == KeyEvent::KEY_RIGHT) { Right = isDown; }
    if (k_code == KeyEvent::KEY_DOWN) { Down = isDown; }
    if (k_code == KeyEvent::KEY_SPACE) { Space = isDown; }
  }

  bool Escape{false};
  bool Left{false};
  bool Right{false};
  bool Up{false};
  bool Down{false};
  bool Space{false};
};

//======================================================================================================================
class cEntity {
public:
  virtual ~cEntity() = default;

  vec2 Velocity;
  vec2 Position;
  Color8u Color;
  float Size;
};

//======================================================================================================================
// The default entity, right now it's always a circle and has no hit detection.
//======================================================================================================================
class cPlayer : public cEntity {
public:
  static const uint32_t k_DefaultSpeed = 6;

  //======================================================================================================================
  cPlayer() = default;

  //======================================================================================================================
  void Draw() const {
    gl::ScopedModelMatrix m;
    gl::ScopedColor c(Color);

    // Draw a simple colored rectangle, don't actully use the textures (yet)
    gl::translate(Position);
    gl::drawSolidRect(Rectf(0, 0, Size, Size));
  }

  //======================================================================================================================
  void Update(cKeyHandler const & keys) {
    auto const k_WindowSize = getWindowSize();

    if (keys.Right) { Position.x += k_DefaultSpeed; }
    if (keys.Left) { Position.x -= k_DefaultSpeed; }
    if (keys.Up) { Position.y -= k_DefaultSpeed; }
    if (keys.Down) { Position.y += k_DefaultSpeed; }

    auto const k_xMax = k_WindowSize.x - Size;
    auto const k_yMax = k_WindowSize.y - Size;
    Position.x        = (Position.x <= 0) ? 0 : ((Position.x >= k_xMax) ? k_xMax : Position.x);
    Position.y        = (Position.y <= 0) ? 0 : ((Position.y >= k_yMax) ? k_yMax : Position.y);
  }
};

//======================================================================================================================
class cBullet : public cEntity {
public:
  //======================================================================================================================
  void Draw() const {
    gl::ScopedModelMatrix m;
    gl::ScopedColor c(Color);

    // Draw a simple colored rectangle, don't actully use the textures (yet)
    gl::translate(Position);
    gl::drawSolidRect(Rectf(0, 0, Size, Size));
  }

  //======================================================================================================================
  void Update() {
    Position += Velocity;
    if (Position.y < 0) { Dead = true; }
  }

  bool Dead {false};
};

//======================================================================================================================
class cEnemy : public cEntity {
public:
  //======================================================================================================================
  void Draw() const {
    gl::ScopedModelMatrix m;
    gl::ScopedColor c(Color);

    // Draw a simple colored rectangle, don't actully use the textures (yet)
    gl::translate(Position);
    gl::drawSolidRect(Rectf(0, 0, Size, Size));
  }

  //======================================================================================================================
  void Update() {
    auto const k_ws = getWindowSize();
    Position += Velocity;
  }

  bool Dead;
};

//======================================================================================================================
// The A GAME App!
// Does meaningful work...
//======================================================================================================================
class cGame : public App {
public:
  static const Color k_BackgroundColor;
  void setup() override;
  void update() override;
  void draw() override;
  void keyDown(KeyEvent) override;
  void keyUp(KeyEvent) override;
  static void prepare(Settings *);

private:
  // mesh and texture
  gl::BatchRef Circle;
  gl::TextureRef Texture;

  // Default shader
  gl::GlslProgRef Shader;

  cKeyHandler Keys;
  cPlayer Player;
  std::vector< cBullet > Bullets;
};

const Color cGame::k_BackgroundColor = Color(0.11f, 0.112f, 0.11f);

//======================================================================================================================
void cGame::prepare(Settings * settings) {
  settings->setTitle("A GAME");
  settings->setWindowSize(500, 500);
}

//======================================================================================================================
void cGame::keyDown(KeyEvent event) { Keys.Handle(event, true); }

//======================================================================================================================
void cGame::keyUp(KeyEvent event) { Keys.Handle(event, false); }

//======================================================================================================================
// Our main 'do a thing' function, where all of our setup and updates are
// actually seen!
//======================================================================================================================
void cGame::draw() {
  gl::clear(k_BackgroundColor);
  gl::setMatricesWindow(getWindowSize());

  for (auto const & b : Bullets) {
    b.Draw();
  }
  Player.Draw();
}

//======================================================================================================================
void cGame::setup() {
  // allow maximum frame rate
  disableFrameRate();
  gl::enableVerticalSync(false);

  // Player config
  Player.Position = getWindowCenter();
  Player.Size     = 15;
  Player.Color    = Color8u(255, 235, 59);

  // load texture
  Texture = gl::Texture::create(loadImage(loadAsset(TEXTURE_PLAYER)));
  Texture->bind();

  // create a default shader with color and texture support
  auto shader = gl::ShaderDef().color().texture();
  Shader      = getStockShader(shader);

  // combine mesh and shader into batch for much better performance
  // auto circle = geom::Circle().subdivisions(40).radius(15.0f);
  auto shape = geom::Rect().rect(Rectf(0, 0, Player.Size, Player.Size));
  Circle     = gl::Batch::create(shape, Shader);
}

//======================================================================================================================
void cGame::update() {
  // Use a fixed time step for a steady 60 updates per second.
  static const double timestep = 1.0 / 60.0;

  // Keep track of time.
  static double time        = getElapsedSeconds();
  static double lastBullet  = 0.0;
  static double accumulator = 0.0;

  // Calculate elapsed time since last frame.
  double elapsed = getElapsedSeconds() - time;
  time += elapsed;

  // Update the simulation.
  accumulator += math< double >::min(elapsed, 0.1); // prevents 'spiral of death'
  while (accumulator >= timestep) {
    accumulator -= timestep;
    Player.Update(Keys);
    for (auto itr = std::begin(Bullets); itr != std::end(Bullets);) {
      if (itr->Dead) { itr = Bullets.erase(itr); }
      else {
        itr->Update();
        itr++;
      }
    }

    console() << Bullets.size() << " bullets" << std::endl;

    if (Keys.Escape) { quit(); }

    if (Keys.Space && (time - lastBullet) > (5 * timestep)) {
      cBullet bullet;
      bullet.Position = Player.Position + vec2(Player.Size * 0.5, 0);
      bullet.Size     = 2;
      bullet.Velocity = {0, -7};
      bullet.Color    = Color(1, 0, 0);
      Bullets.push_back(bullet);
      lastBullet = time;
    }
  }
}

//======================================================================================================================
// This is a magical macro that sets up our 2D world and runs our app!
//======================================================================================================================
CINDER_APP(cGame, RendererGl, &cGame::prepare)
