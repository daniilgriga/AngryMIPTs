#include "render/particles.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace angry
{
namespace
{

float rand_float ( float lo, float hi )
{
    float t = static_cast<float> ( std::rand() ) / static_cast<float> ( RAND_MAX );
    return lo + t * ( hi - lo );
}

}  // namespace

void ParticleSystem::emit ( sf::Vector2f pos, int count, sf::Color color,
                            float speed, float lifetime, float size )
{
    for ( int i = 0; i < count; ++i )
    {
        float angle = rand_float ( 0.f, 6.2832f );
        float spd = rand_float ( speed * 0.3f, speed );
        Particle p;
        p.position = pos;
        p.velocity = {std::cos ( angle ) * spd, std::sin ( angle ) * spd};
        p.color = color;
        p.lifetime = rand_float ( lifetime * 0.5f, lifetime );
        p.size = rand_float ( size * 0.5f, size );
        p.drag = 0.6f;
        particles_.push_back ( p );
    }
}

void ParticleSystem::emit_ring ( sf::Vector2f pos, int count, sf::Color color,
                                 float speed, float lifetime, float size )
{
    for ( int i = 0; i < count; ++i )
    {
        float angle = 6.2832f * static_cast<float> ( i ) / static_cast<float> ( count );
        Particle p;
        p.position = pos;
        p.velocity = {std::cos ( angle ) * speed, std::sin ( angle ) * speed};
        p.color = color;
        p.lifetime = lifetime;
        p.size = size;
        p.drag = 0.25f;
        particles_.push_back ( p );
    }
}

void ParticleSystem::emit_shards ( sf::Vector2f pos, int count, sf::Color color,
                                   float speed, float lifetime, float size,
                                   float angular_speed )
{
    for ( int i = 0; i < count; ++i )
    {
        float angle = rand_float ( 0.f, 6.2832f );
        float spd = rand_float ( speed * 0.35f, speed );
        Particle p;
        p.position = pos;
        p.velocity = {std::cos ( angle ) * spd, std::sin ( angle ) * spd};
        p.color = color;
        p.lifetime = rand_float ( lifetime * 0.6f, lifetime );
        p.size = rand_float ( size * 0.6f, size );
        p.shape = ParticleShape::Shard;
        p.rotationDeg = rand_float ( 0.f, 360.f );
        p.angularVelocityDeg = rand_float ( -angular_speed, angular_speed );
        p.gravityScale = 1.15f;
        p.drag = 1.2f;
        particles_.push_back ( p );
    }
}

void ParticleSystem::update ( float dt )
{
    for ( auto& p : particles_ )
    {
        p.age += dt;
        const float damping = std::max ( 0.f, 1.f - p.drag * dt );
        p.velocity *= damping;
        p.velocity.y += 200.f * p.gravityScale * dt;  // gravity on particles
        p.position += p.velocity * dt;
        p.rotationDeg += p.angularVelocityDeg * dt;
    }

    // remove dead particles
    particles_.erase (
        std::remove_if ( particles_.begin(), particles_.end(),
                         [] ( const Particle& p ) { return p.age >= p.lifetime; } ),
        particles_.end() );
}

void ParticleSystem::render ( sf::RenderTarget& target )
{
    for ( const auto& p : particles_ )
    {
        float alpha = 1.f - ( p.age / p.lifetime );
        float cur_size = p.size * alpha;

        sf::Color c = p.color;
        c.a = static_cast<uint8_t> ( 255.f * alpha * alpha );

        if ( p.shape == ParticleShape::Shard )
        {
            sf::RectangleShape shard ( {cur_size * 1.6f, cur_size * 0.8f} );
            shard.setOrigin ( {cur_size * 0.8f, cur_size * 0.4f} );
            shard.setPosition ( p.position );
            shard.setRotation ( sf::degrees ( p.rotationDeg ) );
            shard.setFillColor ( c );
            target.draw ( shard );
        }
        else
        {
            sf::CircleShape dot ( cur_size );
            dot.setOrigin ( {cur_size, cur_size} );
            dot.setPosition ( p.position );
            dot.setFillColor ( c );
            target.draw ( dot );
        }
    }
}

}  // namespace angry
