#include <gtest/gtest.h>

#include "utils/UrlUtils.hpp"

using chainx::UrlUtils;

TEST(UrlUtilsTest, ResolvesDomains)
{
    const QUrl url = UrlUtils::resolveInput(QStringLiteral("example.com"));
    EXPECT_EQ(url.toString(), QStringLiteral("https://example.com"));
}

TEST(UrlUtilsTest, ResolvesDomainsWithPortAndPath)
{
    const QUrl url = UrlUtils::resolveInput(QStringLiteral("example.com:8080/path?q=1"));
    EXPECT_EQ(url.scheme(), QStringLiteral("https"));
    EXPECT_EQ(url.host(), QStringLiteral("example.com"));
    EXPECT_EQ(url.port(), 8080);
}

TEST(UrlUtilsTest, KeepsExplicitHttps)
{
    const QUrl url = UrlUtils::resolveInput(QStringLiteral("https://example.com/page"));
    EXPECT_EQ(url.toString(), QStringLiteral("https://example.com/page"));
}

TEST(UrlUtilsTest, ResolvesLocalhost)
{
    const QUrl url = UrlUtils::resolveInput(QStringLiteral("localhost:3000"));
    EXPECT_EQ(url.toString(), QStringLiteral("https://localhost:3000"));
}

TEST(UrlUtilsTest, SearchesPlainText)
{
    const QUrl url = UrlUtils::resolveInput(QStringLiteral("hello world"));
    EXPECT_TRUE(url.toString().startsWith(QStringLiteral("https://duckduckgo.com/")));
    EXPECT_TRUE(url.toEncoded().contains(QByteArrayLiteral("hello%20world")));
}

TEST(UrlUtilsTest, RejectsEmptyInput)
{
    EXPECT_FALSE(UrlUtils::resolveInput(QString()).isValid());
    EXPECT_FALSE(UrlUtils::resolveInput(QStringLiteral("   ")).isValid());
}

TEST(UrlUtilsTest, RecognisesUrls)
{
    EXPECT_TRUE(UrlUtils::looksLikeUrl(QStringLiteral("example.com")));
    EXPECT_TRUE(UrlUtils::looksLikeUrl(QStringLiteral("sub.example.co.uk/path")));
    EXPECT_TRUE(UrlUtils::looksLikeUrl(QStringLiteral("192.168.1.1")));
    EXPECT_TRUE(UrlUtils::looksLikeUrl(QStringLiteral("localhost")));

    EXPECT_FALSE(UrlUtils::looksLikeUrl(QStringLiteral("hello world")));
    EXPECT_FALSE(UrlUtils::looksLikeUrl(QStringLiteral("3.14")));
    EXPECT_FALSE(UrlUtils::looksLikeUrl(QString()));
}

TEST(UrlUtilsTest, IdentifiesNewTabPage)
{
    EXPECT_TRUE(UrlUtils::isNewTabPage(UrlUtils::newTabUrl()));
    EXPECT_FALSE(UrlUtils::isNewTabPage(QUrl(QStringLiteral("https://example.com"))));
}

TEST(UrlUtilsTest, DisplayStringHidesNewTabPage)
{
    EXPECT_TRUE(UrlUtils::displayString(UrlUtils::newTabUrl()).isEmpty());
    EXPECT_EQ(UrlUtils::displayString(QUrl(QStringLiteral("https://example.com/x"))),
              QStringLiteral("https://example.com/x"));
}

TEST(UrlUtilsTest, RecordableSchemes)
{
    EXPECT_TRUE(UrlUtils::isRecordable(QUrl(QStringLiteral("https://example.com"))));
    EXPECT_TRUE(UrlUtils::isRecordable(QUrl(QStringLiteral("http://example.com"))));
    EXPECT_TRUE(UrlUtils::isRecordable(QUrl(QStringLiteral("file:///tmp/index.html"))));
    EXPECT_FALSE(UrlUtils::isRecordable(UrlUtils::newTabUrl()));
    EXPECT_FALSE(UrlUtils::isRecordable(QUrl()));
}
