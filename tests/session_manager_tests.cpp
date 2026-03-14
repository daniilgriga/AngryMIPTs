// ============================================================
// session_manager_tests.cpp — SessionManager unit tests.
// Part of: angry::tests
//
// Verifies persistent session management behavior:
//   * Save/load roundtrip for token and username
//   * Missing file and malformed JSON handling
//   * Logout/clear semantics for session storage
//   * In-memory state invariants after operations
// ============================================================

#include "data/session_manager.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{

// #=# Test Helpers #=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

std::filesystem::path make_temp_session_path()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path()
           / ( "angry_mipts_session_test_" + std::to_string( now ) + ".json" );
}

class TempSessionFile
{
public:
    TempSessionFile()
        : path_( make_temp_session_path() )
    {
    }

    ~TempSessionFile()
    {
        std::error_code ec;
        std::filesystem::remove( path_, ec );
    }

    const std::string path_string() const
    {
        return path_.string();
    }

    const std::filesystem::path& path() const
    {
        return path_;
    }

private:
    std::filesystem::path path_;
};

}  // namespace

// #=# Test Cases #=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

TEST( SessionManager, SaveThenLoadRestoresSession )
{
    TempSessionFile temp_file;

    angry::SessionManager writer( temp_file.path_string() );
    writer.setSession( "token-abc", "alex" );
    writer.saveSession();

    angry::SessionManager reader( temp_file.path_string() );
    reader.loadSession();

    EXPECT_TRUE( reader.isLoggedIn() );
    EXPECT_EQ( reader.token(), "token-abc" );
    EXPECT_EQ( reader.username(), "alex" );
}

TEST( SessionManager, MissingFileMeansEmptySession )
{
    TempSessionFile temp_file;

    angry::SessionManager sm( temp_file.path_string() );
    sm.loadSession();

    EXPECT_FALSE( sm.isLoggedIn() );
    EXPECT_TRUE( sm.token().empty() );
    EXPECT_TRUE( sm.username().empty() );
}

TEST( SessionManager, BrokenJsonResultsInEmptySession )
{
    TempSessionFile temp_file;
    {
        std::ofstream out( temp_file.path() );
        out << "{ not valid json";
    }

    angry::SessionManager sm( temp_file.path_string() );
    sm.loadSession();

    EXPECT_FALSE( sm.isLoggedIn() );
    EXPECT_TRUE( sm.token().empty() );
    EXPECT_TRUE( sm.username().empty() );
}

TEST( SessionManager, ClearSessionRemovesFileAndState )
{
    TempSessionFile temp_file;

    angry::SessionManager sm( temp_file.path_string() );
    sm.setSession( "token-xyz", "mipt" );
    sm.saveSession();

    ASSERT_TRUE( std::filesystem::exists( temp_file.path() ) );
    ASSERT_TRUE( sm.isLoggedIn() );

    sm.clearSession();

    EXPECT_FALSE( std::filesystem::exists( temp_file.path() ) );
    EXPECT_FALSE( sm.isLoggedIn() );
    EXPECT_TRUE( sm.token().empty() );
    EXPECT_TRUE( sm.username().empty() );
}
