// ============================================================
// session_manager.cpp — Persistent auth session implementation.
// Part of: angry::data
//
// Implements session file IO and validation logic:
//   * Reads/writes session JSON with token/username fields
//   * Validates schema and gracefully handles corrupt data
//   * Clears persisted state on explicit logout
//   * Emits diagnostic logs for load/save/clear operations
//
// Web build: token/username are persisted in browser localStorage
//   via EM_ASM instead of the local filesystem.
// ============================================================

#include "data/session_manager.hpp"

#include "data/logger.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <cstdlib>
#else
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#endif

namespace angry
{

// #=# Construction #=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

SessionManager::SessionManager( std::string filepath )
    : filepath_( std::move( filepath ) )
{
}

// #=# Session Persistence API #=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

#ifdef __EMSCRIPTEN__  // Web — localStorage

namespace
{

constexpr const char* kTokenKey    = "angry_session_token";
constexpr const char* kUsernameKey = "angry_session_username";

std::string ls_get( const char* key )
{
    char* raw = reinterpret_cast<char*>( EM_ASM_PTR(
    {
        var val = localStorage.getItem( UTF8ToString( $0 ) );
        if ( val === null || val === undefined ) return 0;
        var len = lengthBytesUTF8( val ) + 1;
        var buf = _malloc( len );
        stringToUTF8( val, buf, len );
        return buf;
    }, key ) );

    if ( !raw ) return {};
    std::string result( raw );
    std::free( raw );
    return result;
}

}  // namespace

void SessionManager::load_session()
{
    token_.clear();
    username_.clear();

    token_    = ls_get( kTokenKey );
    username_ = ls_get( kUsernameKey );

    if ( token_.empty() || username_.empty() )
    {
        token_.clear();
        username_.clear();
        Logger::info( "Session loaded: empty (no localStorage data)" );
        return;
    }

    Logger::info( "Session loaded: user={}", username_ );
}

void SessionManager::save_session() const
{
    EM_ASM(
    {
        try { localStorage.setItem( UTF8ToString( $0 ), UTF8ToString( $1 ) ); } catch(e) {}
        try { localStorage.setItem( UTF8ToString( $2 ), UTF8ToString( $3 ) ); } catch(e) {}
    },
    kTokenKey, token_.c_str(), kUsernameKey, username_.c_str() );

    Logger::info( "Session saved: user={}", username_ );
}

void SessionManager::clear_session()
{
    token_.clear();
    username_.clear();

    EM_ASM(
    {
        try { localStorage.removeItem( UTF8ToString( $0 ) ); } catch(e) {}
        try { localStorage.removeItem( UTF8ToString( $1 ) ); } catch(e) {}
    },
    kTokenKey, kUsernameKey );

    Logger::info( "Session cleared" );
}

#else  // Native — filesystem

namespace
{

using Json = nlohmann::json;

}  // namespace

void SessionManager::load_session()
{
    token_.clear();
    username_.clear();

    if ( filepath_.empty() )
    {
        Logger::info( "Session load skipped: empty filepath" );
        return;
    }

    const std::filesystem::path path( filepath_ );
    if ( !std::filesystem::exists( path ) )
    {
        Logger::info( "Session loaded: empty (file not found: {})", filepath_ );
        return;
    }

    try
    {
        std::ifstream input( path );
        if ( !input.is_open() )
        {
            Logger::error( "Session load failed: cannot open {}", filepath_ );
            return;
        }

        Json root;
        input >> root;
        if ( !root.is_object() )
        {
            Logger::error( "Session load failed: invalid JSON root in {}", filepath_ );
            return;
        }

        if ( !root.contains( "token" ) || !root.contains( "username" )
             || !root["token"].is_string() || !root["username"].is_string() )
        {
            Logger::error( "Session load failed: missing token/username in {}", filepath_ );
            return;
        }

        token_ = root["token"].get<std::string>();
        username_ = root["username"].get<std::string>();

        if ( token_.empty() || username_.empty() )
        {
            token_.clear();
            username_.clear();
            Logger::info( "Session loaded: empty" );
            return;
        }

        Logger::info( "Session loaded: user={}", username_ );
    }
    catch ( const std::exception& e )
    {
        token_.clear();
        username_.clear();
        Logger::error( "Session load failed for {}: {}", filepath_, e.what() );
    }
}

void SessionManager::save_session() const
{
    if ( filepath_.empty() )
    {
        Logger::info( "Session save skipped: empty filepath" );
        return;
    }

    try
    {
        const std::filesystem::path path( filepath_ );
        const std::filesystem::path parent = path.parent_path();
        if ( !parent.empty() )
        {
            std::filesystem::create_directories( parent );
        }

        std::ofstream output( path );
        if ( !output.is_open() )
        {
            Logger::error( "Session save failed: cannot open {}", filepath_ );
            return;
        }

        const Json root = {
            {"token", token_},
            {"username", username_},
        };

        output << root.dump( 2 ) << '\n';
        if ( !output.good() )
        {
            Logger::error( "Session save failed: write error {}", filepath_ );
            return;
        }

        Logger::info( "Session saved: user={}", username_ );
    }
    catch ( const std::exception& e )
    {
        Logger::error( "Session save failed for {}: {}", filepath_, e.what() );
    }
}

void SessionManager::clear_session()
{
    token_.clear();
    username_.clear();

    if ( filepath_.empty() )
    {
        Logger::info( "Session clear skipped: empty filepath" );
        return;
    }

    try
    {
        const std::filesystem::path path( filepath_ );
        if ( std::filesystem::exists( path ) )
        {
            std::filesystem::remove( path );
        }
        Logger::info( "Session cleared" );
    }
    catch ( const std::exception& e )
    {
        Logger::error( "Session clear failed for {}: {}", filepath_, e.what() );
    }
}

#endif  // __EMSCRIPTEN__

// #=# Accessors / Mutators #=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

bool SessionManager::is_logged_in() const
{
    return !token_.empty() && !username_.empty();
}

const std::string& SessionManager::token() const
{
    return token_;
}

const std::string& SessionManager::username() const
{
    return username_;
}

void SessionManager::set_session( std::string token, std::string username )
{
    token_ = std::move( token );
    username_ = std::move( username );
}

}  // namespace angry
