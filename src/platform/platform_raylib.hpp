#pragma once

// ============================================================
// platform_raylib.hpp — Raylib backend (Emscripten/web builds).
// Defines platform:: types matching the SFML backend API
// so that platform-agnostic code compiles on both targets.
// ============================================================

#include <raylib.h>
#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace platform
{

// ── Geometry ────────────────────────────────────────────────

struct Vec2f
{
    float x = 0, y = 0;
    Vec2f() = default;
    Vec2f( float x, float y ) : x(x), y(y) {}
    Vec2f  operator+( Vec2f o ) const { return { x + o.x, y + o.y }; }
    Vec2f  operator-( Vec2f o ) const { return { x - o.x, y - o.y }; }
    Vec2f  operator*( float s ) const { return { x * s,   y * s   }; }
    Vec2f& operator+=( Vec2f o ) { x += o.x; y += o.y; return *this; }
    Vec2f& operator-=( Vec2f o ) { x -= o.x; y -= o.y; return *this; }
};
struct Vec2u { unsigned x = 0, y = 0; };
struct Vec2i { int x = 0, y = 0; };

struct Rect
{
    float left = 0, top = 0, width = 0, height = 0;
    Rect() = default;
    Rect( Vec2f pos, Vec2f size )
        : left(pos.x), top(pos.y), width(size.x), height(size.y) {}
    Rect( float l, float t, float w, float h )
        : left(l), top(t), width(w), height(h) {}
    bool contains( Vec2f p ) const
    {
        return p.x >= left && p.x < left + width
            && p.y >= top  && p.y < top  + height;
    }
};
using FloatRect = Rect;

struct Transform
{
    // 3×3 affine matrix (row-major, identity by default)
    float m[9] = { 1,0,0, 0,1,0, 0,0,1 };
};

// ── Color ───────────────────────────────────────────────────

struct Color
{
    uint8_t r = 0, g = 0, b = 0, a = 255;
    Color() = default;
    Color( uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255 )
        : r(r), g(g), b(b), a(a) {}

    static const Color White;
    static const Color Black;
    static const Color Transparent;

    ::Color to_rl() const { return { r, g, b, a }; }
};

inline const Color Color::White       { 255, 255, 255, 255 };
inline const Color Color::Black       {   0,   0,   0, 255 };
inline const Color Color::Transparent {   0,   0,   0,   0 };

// ── Time / Clock ────────────────────────────────────────────

struct Time
{
    float seconds = 0.f;
    float asSeconds()      const { return seconds; }
    int   asMilliseconds() const { return static_cast<int>(seconds * 1000.f); }
};

inline Time seconds( float s ) { return { s }; }
inline Time milliseconds( int ms ) { return { ms / 1000.f }; }

struct Clock
{
    double start_ = GetTime();
    Time getElapsedTime() const { return { float(GetTime() - start_) }; }
    Time restart() { float e = float(GetTime() - start_); start_ = GetTime(); return { e }; }
};

// ── Font / Text ─────────────────────────────────────────────

struct Font
{
    ::Font rl {};
    bool   loaded = false;
    bool openFromFile( const std::string& path )
    {
        rl = ::LoadFont( path.c_str() );
        loaded = IsFontReady( rl );
        return loaded;
    }
};

// Text is not a stored object in Raylib — it's drawn immediately.
// We keep a data struct matching sf::Text fields so scenes compile unchanged.
struct Text
{
    const Font*  font_     = nullptr;
    std::string  string_;
    unsigned     char_size_ = 18;
    Color        fill_color_    { 255, 255, 255, 255 };
    Color        outline_color_ { 0, 0, 0, 0 };
    float        outline_thickness_ = 0.f;
    Vec2f        position_;
    Vec2f        origin_;
    bool         bold_ = false;

    Text() = default;
    Text( const Font& f, const std::string& s, unsigned size )
        : font_(&f), string_(s), char_size_(size) {}

    void setString( const std::string& s )  { string_ = s; }
    void setCharacterSize( unsigned s )      { char_size_ = s; }
    void setFillColor( Color c )             { fill_color_ = c; }
    void setOutlineColor( Color c )          { outline_color_ = c; }
    void setOutlineThickness( float t )      { outline_thickness_ = t; }
    void setStyle( int /*flags*/ )           {}  // Bold handled separately
    void setPosition( Vec2f p )              { position_ = p; }
    void setOrigin( Vec2f o )                { origin_ = o; }

    struct Bounds { float left, top, width, height; };
    Bounds getLocalBounds() const
    {
        if ( !font_ || !font_->loaded ) return {};
        ::Vector2 sz = MeasureTextEx( font_->rl, string_.c_str(),
                                       float(char_size_), 1.f );
        return { 0.f, 0.f, sz.x, sz.y };
    }
    Bounds getGlobalBounds() const
    {
        auto lb = getLocalBounds();
        return { position_.x - origin_.x + lb.left,
                 position_.y - origin_.y + lb.top,
                 lb.width, lb.height };
    }
};

// ── Texture / Image ─────────────────────────────────────────

struct Image
{
    ::Image rl {};
    bool    ready = false;

    void create( unsigned w, unsigned h, Color fill = {} )
    {
        rl = GenImageColor( int(w), int(h), fill.to_rl() );
        ready = true;
    }
    void setPixel( unsigned x, unsigned y, Color c )
    {
        ImageDrawPixel( &rl, int(x), int(y), c.to_rl() );
    }
    void saveToFile( const std::string& /*path*/ ) const
    {
        // On web: no disk — skip silently.
    }
    Vec2u getSize() const
    {
        return { unsigned(rl.width), unsigned(rl.height) };
    }
};

struct Texture
{
    ::Texture2D rl {};
    bool        loaded = false;

    void loadFromImage( const Image& img )
    {
        rl     = LoadTextureFromImage( img.rl );
        loaded = IsTextureReady( rl );
    }
    void setSmooth( bool /*s*/ ) {}  // use SetTextureFilter if needed
    Vec2u getSize() const { return { unsigned(rl.width), unsigned(rl.height) }; }
};

// ── Shader ──────────────────────────────────────────────────

struct Shader
{
    ::Shader rl {};
    bool     loaded = false;

    enum class Type { Fragment, Vertex };
    static bool isAvailable() { return true; }

    bool loadFromMemory( const std::string& src, Type type )
    {
        const char* vs = ( type == Type::Fragment ) ? nullptr : src.c_str();
        const char* fs = ( type == Type::Fragment ) ? src.c_str() : nullptr;
        rl     = LoadShaderFromMemory( vs, fs );
        loaded = IsShaderReady( rl );
        return loaded;
    }

    void setUniform( const std::string& name, float v )
    {
        int loc = GetShaderLocation( rl, name.c_str() );
        SetShaderValue( rl, loc, &v, SHADER_UNIFORM_FLOAT );
    }
    void setUniform( const std::string& name, Vec2f v )
    {
        int loc = GetShaderLocation( rl, name.c_str() );
        float vals[2] = { v.x, v.y };
        SetShaderValue( rl, loc, vals, SHADER_UNIFORM_VEC2 );
    }
};

// ── View (camera / letterbox) ────────────────────────────────

struct View
{
    Vec2f center_;
    Vec2f size_;
    Rect  viewport_ { 0.f, 0.f, 1.f, 1.f };  // normalized 0..1

    View() = default;
    View( Rect r ) : center_{ r.left + r.width*0.5f, r.top + r.height*0.5f },
                     size_{ r.width, r.height } {}

    void setCenter( Vec2f c )   { center_ = c; }
    void setSize( Vec2f s )     { size_   = s; }
    void setViewport( Rect vp ) { viewport_ = vp; }
    void reset( Rect r )
    {
        center_ = { r.left + r.width*0.5f, r.top + r.height*0.5f };
        size_   = { r.width, r.height };
    }
    Vec2f getSize()   const { return size_; }
    Vec2f getCenter() const { return center_; }
};

// ── Event ────────────────────────────────────────────────────

struct KeyEvent   { int key; bool alt = false; bool shift = false; bool ctrl = false; };
struct MouseBtnEvent  { float x, y; int button; };  // button: 0=left,1=right,2=mid
struct MouseMoveEvent { float x, y; };
struct MouseWheelEvent{ float delta; float x, y; };
struct TextEvent  { uint32_t unicode; };
struct ResizedEvent   { unsigned w, h; };
struct ClosedEvent    {};
struct FocusEvent     {};

using Event = std::variant<
    KeyEvent,
    MouseBtnEvent,
    MouseMoveEvent,
    MouseWheelEvent,
    TextEvent,
    ResizedEvent,
    ClosedEvent,
    FocusEvent
>;

// ── Window ──────────────────────────────────────────────────
// Wraps Raylib window + provides sf::RenderWindow-compatible API.

struct RenderTexture; // forward
struct Window
{
    bool open_ = false;
    int  w_ = 1280, h_ = 720;

    void create( unsigned w, unsigned h, const std::string& title )
    {
        InitWindow( int(w), int(h), title.c_str() );
        open_ = true;
        w_ = int(w); h_ = int(h);
    }
    bool isOpen() const { return open_ && !WindowShouldClose(); }
    void close()        { open_ = false; CloseWindow(); }
    void display()      { EndDrawing(); }
    void clear( Color c = {} ) { BeginDrawing(); ClearBackground( c.to_rl() ); }
    void setFramerateLimit( unsigned fps )  { SetTargetFPS( int(fps) ); }
    void setVerticalSyncEnabled( bool /*v*/ ) {}

    Vec2u getSize() const { return { unsigned(GetScreenWidth()), unsigned(GetScreenHeight()) }; }

    // Events are polled separately via poll_events()
    std::vector<Event> pollEvents();
};

// ── Poll events (Raylib input → Event stream) ────────────────

std::vector<Event> poll_events( Window& w );

// ── Angle helper (matches sf::degrees) ──────────────────────
inline float degrees( float v ) { return v; }  // Raylib uses degrees natively

// ── RenderTarget stub ────────────────────────────────────────
// On Raylib there's no separate RenderTarget type at this level —
// drawing goes directly to the screen or a RenderTexture.
// Scene render() takes Window& and we add a RenderTexture variant later.
using RenderTarget = Window;

// ── Primitive types stubs (used by particles / renderer) ─────

struct Vertex
{
    Vec2f    position;
    Color    color { 255,255,255,255 };
};

struct VertexArray
{
    int                prim_type = 0;  // matches Raylib DrawPrimitive types
    std::vector<Vertex> verts;
    void resize( std::size_t n ) { verts.resize(n); }
    Vertex& operator[]( std::size_t i ) { return verts[i]; }
    const Vertex& operator[]( std::size_t i ) const { return verts[i]; }
    std::size_t getVertexCount() const { return verts.size(); }
};

struct RectShape
{
    Vec2f pos_, size_, origin_;
    Color fill_, outline_;
    float outline_t_ = 0.f;
    float rotation_  = 0.f;

    void setPosition( Vec2f p )            { pos_       = p; }
    void setSize( Vec2f s )                { size_      = s; }
    void setFillColor( Color c )           { fill_      = c; }
    void setOutlineColor( Color c )        { outline_   = c; }
    void setOutlineThickness( float t )    { outline_t_ = t; }
    void setOrigin( Vec2f o )              { origin_    = o; }
    void setRotation( float deg )          { rotation_  = deg; }
    Rect getGlobalBounds() const
    {
        return { pos_.x - origin_.x, pos_.y - origin_.y, size_.x, size_.y };
    }
};

struct CircleShape
{
    Vec2f  pos_, origin_;
    Color  fill_, outline_;
    float  outline_t_ = 0.f;
    float  radius_    = 0.f;
    int    point_count_ = 32;

    void setPosition( Vec2f p )         { pos_         = p; }
    void setRadius( float r )           { radius_      = r; }
    void setFillColor( Color c )        { fill_        = c; }
    void setOutlineColor( Color c )     { outline_     = c; }
    void setOutlineThickness( float t ) { outline_t_   = t; }
    void setOrigin( Vec2f o )           { origin_      = o; }
    void setPointCount( int n )         { point_count_ = n; }
    float getRadius() const             { return radius_; }
};

struct ConvexShape
{
    std::vector<Vec2f> points_;
    Vec2f   pos_, origin_;
    Color   fill_, outline_;
    float   outline_t_ = 0.f;
    const Texture* tex_ = nullptr;

    void setPointCount( int n )             { points_.resize(n); }
    void setPoint( int i, Vec2f p )         { points_[i] = p; }
    void setPosition( Vec2f p )             { pos_       = p; }
    void setOrigin( Vec2f o )               { origin_    = o; }
    void setFillColor( Color c )            { fill_      = c; }
    void setOutlineColor( Color c )         { outline_   = c; }
    void setOutlineThickness( float t )     { outline_t_ = t; }
    void setTexture( const Texture* t )     { tex_       = t; }
};

struct Sprite
{
    const Texture* tex_      = nullptr;
    Vec2f          pos_, origin_, scale_ { 1.f, 1.f };
    float          rotation_ = 0.f;
    Color          color_    { 255,255,255,255 };

    void setTexture( const Texture& t )     { tex_      = &t; }
    void setPosition( Vec2f p )             { pos_      = p; }
    void setOrigin( Vec2f o )               { origin_   = o; }
    void setScale( Vec2f s )                { scale_    = s; }
    void setRotation( float deg )           { rotation_ = deg; }
    void setColor( Color c )                { color_    = c; }
};

// ── Off-screen render texture ────────────────────────────────

struct RenderTexture
{
    ::RenderTexture2D rl {};
    bool ready = false;
    Texture color_tex;

    bool create( unsigned w, unsigned h )
    {
        rl    = LoadRenderTexture( int(w), int(h) );
        ready = IsRenderTextureReady( rl );
        color_tex.rl     = rl.texture;
        color_tex.loaded = ready;
        return ready;
    }
    void clear( Color c = {} )
    {
        BeginTextureMode( rl );
        ClearBackground( c.to_rl() );
    }
    void display() { EndTextureMode(); }
    const Texture& getTexture() const { return color_tex; }
};

// ── Audio ────────────────────────────────────────────────────

struct SoundBuffer
{
    ::Wave    wave {};
    bool      loaded = false;

    bool loadFromSamples( const int16_t* samples, std::size_t count,
                          unsigned channels, unsigned sampleRate )
    {
        wave.frameCount = unsigned(count / channels);
        wave.sampleRate = sampleRate;
        wave.sampleSize = 16;
        wave.channels   = channels;
        // Copy samples
        std::size_t bytes = count * sizeof(int16_t);
        wave.data = malloc( bytes );
        if ( wave.data ) { memcpy( wave.data, samples, bytes ); loaded = true; }
        return loaded;
    }
    std::size_t getSampleCount() const
    {
        return wave.frameCount * wave.channels;
    }
};

struct Sound
{
    ::Sound rl {};
    bool    loaded = false;

    void load( const SoundBuffer& buf )
    {
        rl     = LoadSoundFromWave( buf.wave );
        loaded = IsSoundReady( rl );
    }

    enum class Status { Stopped, Playing, Paused };
    Status getStatus() const
    {
        return IsSoundPlaying( rl ) ? Status::Playing : Status::Stopped;
    }

    void setVolume( float v )  { SetSoundVolume( rl, v / 100.f ); }
    void setPitch( float p )   { SetSoundPitch( rl, p ); }
    void play()                { PlaySound( rl ); }
    void stop()                { StopSound( rl ); }
};

}  // namespace platform
