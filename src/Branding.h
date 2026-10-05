#pragma once
// Copyright (c) 2026, mikosu contributors, All rights reserved.
//
// mikosu's identity, defined in one place (with AC_INIT in configure.ac, which sets PACKAGE_NAME, PACKAGE_VERSION
// and PACKAGE_URL). Everything user-visible or sent to a server that names the product derives from here, so a
// later rename means editing AC_INIT, this file and the assets it names.
//
// Not identity, so deliberately NOT derived from PACKAGE_NAME: protocol details of neomod's private-server API
// that servers implement under that name (endpoint paths, form field names, neomod's own server domain). Those are
// spelled out as literals at their call sites, with a comment.

#include "config.h"

// product name: window title, data directory names, file names, logs ("mikosu")
#define BRAND_NAME PACKAGE_NAME

// where the project lives (from AC_INIT)
#define BRAND_REPO_URL PACKAGE_URL
#define BRAND_RELEASES_URL PACKAGE_URL "/releases"
#define BRAND_ISSUES_URL PACKAGE_URL "/issues"

// the project mikosu is forked from (credits, and links to neomod's own release history)
#define BRAND_UPSTREAM_REPO_URL "https://github.com/neomodnet/neomod"

// reverse-DNS application id (SDL app metadata, Linux .desktop/MIME integration, Windows app user model id)
#define BRAND_APP_ID "io.github.taizaki69." PACKAGE_NAME

// credits shown in app metadata
#define BRAND_CREATOR "mikosu contributors; based on neomod (kiwec, spectator) and McOsu (McKay)"

// HTTP user agent, sent to every server mikosu talks to: honest about what it is, with a link back.
// The full string is BRAND_USER_AGENT_PREFIX + <version> + BRAND_USER_AGENT_SUFFIX.
#define BRAND_USER_AGENT_PREFIX "Mozilla/5.0 (compatible; " PACKAGE_NAME "/"
#define BRAND_USER_AGENT_SUFFIX "; +" PACKAGE_URL ")"

// URL scheme mikosu registers for itself ("mikosu://")
#define BRAND_URL_SCHEME PACKAGE_NAME "://"

// main menu logo image in assets/materials (a placeholder until the real logo is chosen)
#define BRAND_LOGO_IMAGE PACKAGE_NAME ".png"

// Discord application for rich presence. Empty until mikosu has its own Discord application: presence must never
// run under another client's application (it would show that client's name).
#define BRAND_DISCORD_CLIENT_ID ""
